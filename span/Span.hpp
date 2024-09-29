#ifndef SPAN_HPP
#define SPAN_HPP

#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <span>
#include <testing/assert.hpp>
#include <type_traits>
#include <vector>

#include "lib/assert.hpp"

template <class T, std::size_t Extent = std::dynamic_extent>
class Span;

template <typename T>
struct is_span : std::false_type {};

template <typename T>
struct is_span<Span<T>> : std::true_type {};

template <typename T>
constexpr bool is_span_v = is_span<T>::value;

template <class T, std::size_t Extent>
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

  static constexpr size_type extent = Extent;

  constexpr Span() noexcept
    requires(extent == 0 || extent == std::dynamic_extent)
      : data_(nullptr) {}

  template <class It>
  explicit(extent != std::dynamic_extent)
  constexpr Span(It first, size_type count)
    requires(std::contiguous_iterator<It>)
      : size_(size_type(count)), data_(std::to_address(first)) {
    MPC_VERIFY(extent == std::dynamic_extent || count == extent);
  }

  template <class Begin, class End>
  explicit(extent != std::dynamic_extent)
  constexpr Span(Begin first, End last)
    requires(std::contiguous_iterator<Begin> && std::sentinel_for<Begin, End> &&
             !std::is_convertible_v<End, size_type> &&
             std::is_convertible_v<std::iter_reference_t<Begin>, element_type>)
      : data_(std::to_address(first)), size_(size_type(last - first)) {}

  template <size_type N>
  constexpr Span(element_type (&arr)[N]) noexcept
    requires((extent == std::dynamic_extent || N == extent) &&
             std::is_convertible_v<
                 std::remove_pointer_t<decltype(std::data(arr))>, element_type>)
      : Span(std::data(arr), N) {}

  template <class U, size_type N>
  constexpr Span(std::array<U, N>& arr) noexcept
    requires((extent == std::dynamic_extent || N == extent) &&
             std::is_convertible_v<
                 std::remove_pointer_t<decltype(std::data(arr))>, element_type>)
      : Span(std::data(arr), N) {}

  template <class U, size_type N>
  constexpr Span(const std::array<U, N>& arr) noexcept
    requires((extent == std::dynamic_extent || N == extent) &&
             std::is_convertible_v<
                 std::remove_pointer_t<decltype(std::data(arr))>, element_type>)

      : Span(std::data(arr), N) {}

  template <class R>
  explicit(extent != std::dynamic_extent)
  constexpr Span(R&& range)
    requires(
        ((std::ranges::contiguous_range<R> && std::ranges::sized_range<R>) ||
         (std::ranges::borrowed_range<R> || std::is_const_v<element_type>)) &&
        (!is_span_v<std::remove_cvref_t<R>> &&
         !std::is_array_v<std::remove_cvref_t<R>>))
      : Span(std::ranges::data(range), std::ranges::size(range)) {}

  explicit(extent != std::dynamic_extent)
  constexpr Span(std::initializer_list<value_type> il) noexcept
    requires(std::is_const_v<element_type>)
      : Span(il.begin(), il.size()) {}

  template <class U, size_type N>
  explicit(extent != std::dynamic_extent &&
           N == std::dynamic_extent)
  constexpr Span(const Span& source) noexcept
    requires((extent == std::dynamic_extent || N == std::dynamic_extent ||
              N == extent) &&
             std::is_convertible_v<U, element_type>)
      : Span(std::data(source), N) {}

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

  constexpr size_type Size() const { return size_.value(); }

  constexpr pointer Data() const noexcept { return data_; }

  template <size_type N>
  constexpr Span<element_type, N> First() const {
    MPC_VERIFY(N <= Size());
    return Span<element_type, N>{Data(), N};
  }

  constexpr Span<element_type, std::dynamic_extent> First(
      const size_type N) const {
    MPC_VERIFY(N <= Size());
    return {Data(), N};
  }

  template <size_type N>
  constexpr Span<element_type, N> Last() const {
    MPC_VERIFY(N <= Size());
    return Span<element_type, N>{(Data() + (Size() - N)), N};
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

  constexpr size_type Size_Bytes() const noexcept {
    return Size() * sizeof(element_type);
  }

  template <size_type Offset, size_type Count = std::dynamic_extent>
  constexpr Span<element_type, Count != std::dynamic_extent ? Count
                               : extent != std::dynamic_extent
                                   ? extent - Offset
                                   : std::dynamic_extent>
  Subspan() const {
    MPC_VERIFY(Offset <= Size());
    MPC_VERIFY(Count == std::dynamic_extent || Count <= (Size() - Offset));

    if constexpr (Count != std::dynamic_extent)
      return {pointer(Data() + Offset), Count};
    else if constexpr (extent != std::dynamic_extent)
      return {pointer(Data() + Offset), extent - Offset};
    else
      return {pointer(Data() + Offset), std::dynamic_extent};
  };

  constexpr Span<element_type, std::dynamic_extent> Subspan(
      const size_type Offset, const size_type Count) const {
    MPC_VERIFY(Offset <= Size());
    MPC_VERIFY(Count <= (Size() - Offset));
    return {pointer(Data() + Offset), Count};
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

  [[no_unique_address]] storage<extent> size_;
  pointer data_;
};

template <typename T, size_t Extent>
Span(T (&)[Extent]) -> Span<T, Extent>;

template <typename T, size_t Extent>
Span(std::array<T, Extent>&) -> Span<T, Extent>;

template <typename T, size_t Extent>
Span(const std::array<T, Extent>&) -> Span<const T, Extent>;

template <std::contiguous_iterator Begin, typename End>
Span(Begin, End) -> Span<std::remove_reference_t<std::iter_reference_t<Begin>>>;

template <std::ranges::contiguous_range Range>
Span(Range&&)
    -> Span<std::remove_reference_t<std::ranges::range_reference_t<Range&>>>;
#endif  // SPAN_HPP