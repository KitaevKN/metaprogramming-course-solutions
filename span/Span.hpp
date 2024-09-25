#ifndef SPAN_HPP
#define SPAN_HPP

#include <cstdlib>
#include <iterator>
#include <span>
#include <type_traits>
#include <vector>

template <class T, std::size_t extent_ = std::dynamic_extent>
class Span {
 public:
  template <typename To, typename From>
  using is_array_convertible = std::is_convertible<From (*)[], To (*)[]>;

  using element_type = T;
  using value_type = typename std::remove_cv<T>::type;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = pointer;
  using const_iterator = const iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;

  static constexpr std::size_t extent = extent_;

  constexpr Span() noexcept : data_(nullptr) {}

  template <class It>
  constexpr Span(It first, std::size_t count)
      : size_(size_type(count)), data_(std::to_address(first)) {
    MPC_VERIFY(extent == std::dynamic_extent || count == extent);
  }

  template <class Begin, class End>
    requires std::is_convertible_v<Begin, pointer> &&
                 std::is_convertible_v<End, pointer>
  constexpr Span(Begin first, End last)
      : data_(std::to_address(first)), size_(size_type(last - first)) {}

  template <std::size_t N>
  constexpr Span(element_type (&arr)[N]) noexcept : Span(std::data(arr), N) {}

  template <class U, std::size_t N>
    requires is_array_convertible<T, U>::value
  constexpr Span(std::array<U, N>& arr) noexcept : Span(std::data(arr), N) {}

  template <class U, std::size_t N>
    requires is_array_convertible<T, U>::value
  constexpr Span(const std::array<U, N>& arr) noexcept
      : Span(std::data(arr), N) {}

  template <class R>
  constexpr Span(R&& range)
      : Span(std::ranges::data(range), std::ranges::size(range)) {}

  constexpr Span(std::initializer_list<value_type> il) noexcept
      : Span(il.begin(), il.size()) {}

  template <class U, std::size_t N>
  constexpr Span(const Span& source) noexcept
      : data_(source.data()), size_(N) {}

  ~Span() noexcept = default;

  constexpr Span& operator=(const Span&) noexcept = default;

  constexpr Span(const Span& other) noexcept = default;

  constexpr bool empty() const { return Size() == 0; }

  constexpr iterator begin() const { return iterator(data_); }

  constexpr iterator end() const { return iterator(data_ + Size()); }

  constexpr const_iterator cbegin() const noexcept {
    return const_iterator(data_);
  }

  constexpr const_iterator cend() const noexcept {
    return const_iterator(data_ + Size());
  }

  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }

  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }

  constexpr std::size_t Size() const { return size_.value(); }

  constexpr pointer Data() const noexcept { return data_; }

  template <size_type N>
  constexpr Span<element_type, N> First() const {
    MPC_VERIFY(N <= Size());
    return {Data(), N};
  }

  constexpr Span<element_type, std::dynamic_extent> First(
      const size_type N) const {
    MPC_VERIFY(N <= Size());
    return {Data(), N};
  }

  template <size_type N>
  constexpr Span<element_type, N> Last() const {
    MPC_VERIFY(N <= Size());
    return {Data() + (Size() - N), N};
  }

  constexpr Span<element_type, std::dynamic_extent> Last(
      const size_type N) const {
    MPC_VERIFY(N <= Size());
    return {Data() + (Size() - N), N};
  }

  constexpr reference operator[](size_type index) const {
    MPC_VERIFY(index < Size());
    return reference(*(data_ + index));
  }

  constexpr reference Front() const {
    MPC_VERIFY(!empty());
    return reference(*(data_));
  }

  constexpr reference Back() const {
    MPC_VERIFY(!empty());
    return reference(*(data_ + Size() - 1));
  }

 private:
  template <std::size_t size>
  struct storage {
    constexpr storage(std::size_t) {}

    static constexpr std::size_t value() { return size; }
  };

  template <>
  struct storage<std::dynamic_extent> {
    constexpr storage(std::size_t S) : S(S) {}

    constexpr std::size_t value() const { return S; }

   private:
    std::size_t S;
  };

  [[no_unique_address]] storage<extent_> size_;
  T* data_;
};

template <typename _Type, size_t _ArrayExtent>
Span(_Type (&)[_ArrayExtent]) -> Span<_Type, _ArrayExtent>;

template <typename _Type, size_t _ArrayExtent>
Span(std::array<_Type, _ArrayExtent>&) -> Span<_Type, _ArrayExtent>;

template <typename _Type, size_t _ArrayExtent>
Span(const std::array<_Type, _ArrayExtent>&) -> Span<const _Type, _ArrayExtent>;

template <std::contiguous_iterator _Iter, typename _End>
Span(_Iter,
     _End) -> Span<std::remove_reference_t<std::iter_reference_t<_Iter>>>;

template <std::ranges::contiguous_range _Range>
Span(_Range&&)
    -> Span<std::remove_reference_t<std::ranges::range_reference_t<_Range&>>>;
#endif  // SPAN_HPP