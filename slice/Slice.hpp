#include <array>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>

inline constexpr std::ptrdiff_t dynamic_stride = -1;

namespace detail {
template <std::size_t size>
struct storage_extent {
  constexpr storage_extent(std::size_t) noexcept {}

  static constexpr std::size_t value_extent() { return size; }
};

template <>
struct storage_extent<std::dynamic_extent> {
  constexpr storage_extent(std::size_t S) noexcept : S(S) {}

  constexpr std::size_t value_extent() const { return S; }

 private:
  std::size_t S;
};

template <std::ptrdiff_t stride>
struct storage_stride {
  constexpr storage_stride(std::ptrdiff_t) {}

  static constexpr std::size_t value_stride() { return stride; }
};

template <>
struct storage_stride<dynamic_stride> {
  constexpr storage_stride(std::ptrdiff_t S) noexcept : S(S) {}

  constexpr std::size_t value_stride() const { return S; }

 private:
  std::ptrdiff_t S;
};

template<typename T>
class iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;

  iterator() = default;

  iterator(iterator&& other) = default;

  iterator(const iterator& other) = default;

  iterator(const pointer ptr, const difference_type skip = 1) : ptr_(ptr), skip_(skip) {}

  iterator& operator=(const iterator& other) = default;

  iterator& operator=(iterator&& other) = default;


  reference operator*() const {
    return *ptr_;
  }

  reference operator[](const difference_type i) const {
    return ptr_[skip_ * i];
  }

  iterator& operator++() {
    ptr_ += skip_;
    return *this;
  }

  iterator operator++(int) {
    ptr_ += skip_;
    return *this;
  }

  iterator& operator--() {
    ptr_ -= skip_;
    return *this;
  }

  iterator operator--(int) {
    ptr_ -= skip_;
    return *this;
  }

  iterator& operator+=(int i) {
    ptr_ += skip_ * i;
    return *this;
  }

  iterator& operator-=(int i) {
    ptr_ -= skip_ * i;
    return *this;
  }

  bool operator<=>(const iterator& other) const {
    return ptr_ <=> other.ptr_;
  }

  bool operator==(const iterator& other) const {
    return ptr_ == other.ptr_;
  }

  iterator operator+(const difference_type offset) const {
    iterator res = *this;
    res += offset;
    return res;
  }

  iterator operator-(const difference_type offset) const {
    iterator res = *this;
    res -= offset;
    return res;
  }

  difference_type operator+(const iterator& other) const {
    return (ptr_ + other.ptr_) / skip_;
  }

  difference_type operator-(const iterator& other) const {
    return (ptr_ - other.ptr_) / skip_;
  }

  friend iterator operator+(const difference_type offset, const iterator& it) {
    iterator copy = it;
    copy += offset;
    return copy;
  }

  friend iterator operator-(const difference_type offset, const iterator& it) {
    iterator copy = it;
    copy -= offset;
    return copy;
  }

private:
  difference_type skip_;
  pointer ptr_;
};
}

template <class T, std::size_t extent = std::dynamic_extent,
          std::ptrdiff_t stride = 1>
class Slice : public detail::storage_extent<extent>, public detail::storage_stride<stride> {
private:
  using ExtentT = detail::storage_extent<extent>;
  using StrideT = detail::storage_stride<stride>;
public:
  using element_type = T;
  using value_type = typename std::remove_cv<T>::type;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = detail::iterator<T>;
  using const_iterator = const iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;

  constexpr Slice() : data_(nullptr), ExtentT(0), StrideT(1) {}

  template<class U, size_type E, difference_type S>
  requires(std::is_convertible<element_type, U>::value)
  constexpr Slice(const Slice<U, E, S>& lhs) noexcept : Slice(lhs.Data(), lhs.Size(), lhs.Stride()) {}

  template <class U>
  Slice(U& container)
      : ExtentT(std::size(container)),
        StrideT(1),
        data_(std::data(container)){};

  template <std::contiguous_iterator It>
  Slice(It first, std::size_t count, std::ptrdiff_t skip)
      : ExtentT(count),
        StrideT(skip),
        data_(std::to_address(first)) {}

  ~Slice() noexcept = default;

  constexpr pointer Data() const noexcept { return data_; }

  constexpr size_type Size() const noexcept { return this->value_extent(); }

  constexpr difference_type Stride() const noexcept {
    return this->value_stride();
  }

  constexpr iterator begin() const noexcept { return iterator(Data(), Stride()); }

  constexpr iterator end() const noexcept { return iterator(Data() + (Size() * Stride()), Stride()); }

  constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }

  constexpr reverse_iterator rend() const noexcept { return begin(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }

  constexpr const_iterator cend() const noexcept { return end(); }

  constexpr reference At(const size_type i) const {
    return begin()[i];
  }

  constexpr reference operator[](const size_type i) const {
    return At(i);
  }

  template<class U, size_type e, difference_type s>
  bool operator==(const Slice<U, e, s>& other) const {
    return other.Data() == Data() && other.Size() == Size() && other.Stride() == Stride();
  }
  // // Data, Size, Stride, begin, end, casts, etc...

  // Slice<T, std::dynamic_extent, stride>
  //   First(std::size_t count) const;

  // template <std::size_t count>
  // Slice<T, /*?*/, stride>
  //   First() const;

  // Slice<T, std::dynamic_extent, stride>
  //   Last(std::size_t count) const;

  // template <std::size_t count>
  // Slice<T, /*?*/, stride>
  //   Last() const;

  // Slice<T, std::dynamic_extent, stride>
  //   DropFirst(std::size_t count) const;

  // template <std::size_t count>
  // Slice<T, /*?*/, stride>
  //   DropFirst() const;

  // Slice<T, std::dynamic_extent, stride>
  //   DropLast(std::size_t count) const;

  // template <std::size_t count>
  // Slice<T, /*?*/, stride>
  //   DropLast() const;

  // Slice<T, /*?*/, /*?*/>
  //   Skip(std::ptrdiff_t skip) const;

  // template <std::ptrdiff_t skip>
  // Slice<T, /*?*/, /*?*/>
  //   Skip() const;
 private:
  T* data_;
};


template
  < std::contiguous_iterator It
  >
Slice(It, std::size_t, std::ptrdiff_t) -> Slice<std::remove_reference_t<std::iter_reference_t<It>>, std::dynamic_extent, dynamic_stride>;

template
  < class T
  , std::size_t N
  >
Slice(std::array<T, N>&) -> Slice<T, N>;

template
  < class U
  >
Slice(U&) -> Slice<typename U::value_type>;

template
  < class U
  >
Slice(const U&) -> Slice<const typename U::value_type>;