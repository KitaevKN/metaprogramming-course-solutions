#include <array>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>

#include "lib/assert.hpp"

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

template <typename T>
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

  iterator(const pointer ptr, const difference_type skip = 1)
      : skip_(skip), ptr_(ptr) {}

  iterator& operator=(const iterator& other) = default;

  iterator& operator=(iterator&& other) = default;

  reference operator*() const {
    MPC_VERIFY(ptr_);
    return *ptr_;
  }

  reference operator[](const difference_type i) const {
    MPC_VERIFY(ptr_);
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

  bool operator<=>(const iterator& other) const { return ptr_ <=> other.ptr_; }

  bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }

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
}  // namespace detail

template <class T, std::size_t extent = std::dynamic_extent,
          std::ptrdiff_t stride = 1>
class Slice : public detail::storage_extent<extent>,
              public detail::storage_stride<stride> {
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

 public:
  constexpr Slice() : ExtentT(0), StrideT(1), data_(nullptr) {}

  template <class U, size_type E, difference_type S>
  constexpr Slice(const Slice<U, E, S>& lhs) noexcept
      : ExtentT(lhs.Size()), StrideT(lhs.Stride()), data_(lhs.Data()) {}

  template <class U, size_type E, difference_type S>
  constexpr Slice(Slice<U, E, S>& lhs) noexcept
      : ExtentT(lhs.Size()), StrideT(lhs.Stride()), data_(lhs.Data()) {}

  template <class U>
  constexpr Slice(U& container)
    requires(std::is_convertible_v<decltype(std::data(container)), pointer> &&
             std::is_convertible_v<decltype(std::size(container)), size_type>)
      : Slice(container, stride){};

  template <class U>
  constexpr Slice(U& container, difference_type skip)
    requires(std::is_convertible_v<decltype(std::data(container)), pointer> &&
             std::is_convertible_v<decltype(std::size(container)), size_type>)
      : ExtentT(std::size(container)),
        StrideT(skip),
        data_(std::data(container)) {
    MPC_VERIFY(extent == std::dynamic_extent ||
               extent == std::size(container) / skip);
    MPC_VERIFY(stride == dynamic_stride || stride == skip);
  };

  template <std::contiguous_iterator It>
    requires(extent != std::dynamic_extent)
  constexpr Slice(It first, std::size_t count, std::ptrdiff_t skip)
      : ExtentT(count), StrideT(skip), data_(std::to_address(first)) {
    MPC_VERIFY(extent == count / skip + (count % skip != 0));
    MPC_VERIFY(stride == dynamic_stride || stride == skip);
  }

  template <std::contiguous_iterator It>
    requires(extent == std::dynamic_extent)
  constexpr Slice(It first, std::size_t count, std::ptrdiff_t skip)
      : ExtentT(count / skip + (count % skip != 0)),
        StrideT(skip),
        data_(std::to_address(first)) {
    MPC_VERIFY(stride == dynamic_stride || stride == skip);
  }
  template <class U>
  constexpr Slice(U* ptr, size_type count) : Slice(ptr, count, 1) {}

  ~Slice() noexcept = default;

  constexpr pointer Data() const noexcept { return data_; }

  constexpr size_type Size() const noexcept { return this->value_extent(); }

  constexpr difference_type Stride() const noexcept {
    return this->value_stride();
  }

  constexpr bool Empty() const noexcept { return Size() == 0; }
  constexpr iterator begin() const noexcept {
    return iterator(Data(), Stride());
  }

  constexpr iterator end() const noexcept {
    return iterator(Data() + (Size() * Stride()), Stride());
  }

  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }

  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }

  constexpr const_iterator cbegin() const noexcept { return begin(); }

  constexpr const_iterator cend() const noexcept { return end(); }

  constexpr reference At(const size_type i) const {
    MPC_VERIFY(i < Size());
    return begin()[i];
  }

  constexpr reference operator[](const size_type i) const { return At(i); }

  template <class U, size_type e, difference_type s>
  bool operator==(const Slice<U, e, s>& other) const noexcept {
    return other.Data() == Data() && other.Size() == Size() &&
           other.Stride() == Stride();
  }

  constexpr auto First(std::size_t count) const {
    MPC_VERIFY(count <= Size());
    return Slice<T, std::dynamic_extent, stride>(Data(), count * Stride(),
                                                 Stride());
  }

  template <std::size_t count>
  constexpr auto First() const {
    MPC_VERIFY(count <= Size());
    return Slice<T, count, stride>(Data(), count * Stride(), Stride());
  }

  constexpr auto Last(std::size_t count) const {
    MPC_VERIFY(count <= Size());
    return Slice<T, std::dynamic_extent, stride>(
        Data() + ((Size() - count) * Stride()), count * Stride(), Stride());
  }

  template <std::size_t count>
  constexpr auto Last() const {
    MPC_VERIFY(count <= Size());
    return Slice<T, count, stride>(Data() + ((Size() - count) * Stride()),
                                   count * Stride(), Stride());
  }

  constexpr auto DropFirst(std::size_t count) const {
    MPC_VERIFY(count <= Size());
    return Last(Size() - count);
  }

  template <std::size_t count>
  constexpr auto DropFirst() const {
    MPC_VERIFY(count <= Size());
    if constexpr (extent == std::dynamic_extent)
      return Last(Size() - count);
    else
      return Last<extent - count>();
  }

  constexpr auto DropLast(std::size_t count) const {
    MPC_VERIFY(count <= Size());
    return First(Size() - count);
  }

  template <std::size_t count>
  constexpr auto DropLast() const {
    MPC_VERIFY(count <= Size());
    if constexpr (extent == std::dynamic_extent)
      return First(Size() - count);
    else
      return First<extent - count>();
  };

  constexpr auto Skip(std::ptrdiff_t skip) const {
    return Slice<T, std::dynamic_extent, dynamic_stride>(
        Data(), Size() * Stride(), skip * Stride());
  }

  template <std::ptrdiff_t skip>
  constexpr auto Skip() const {
    return Slice < T,
           extent == std::dynamic_extent ? std::dynamic_extent
                                         : extent / skip + (extent % skip != 0),
           stride == dynamic_stride
               ? dynamic_stride
               : skip * stride > (Data(), Size() * Stride(), skip * Stride());
  }

 private:
  pointer data_;
};

template <std::contiguous_iterator It>
Slice(It, std::size_t, std::ptrdiff_t)
    -> Slice<std::remove_reference_t<std::iter_reference_t<It>>,
             std::dynamic_extent, dynamic_stride>;

template <class T, std::size_t N>
Slice(std::array<T, N>&) -> Slice<T, N>;

template <class U>
Slice(U&) -> Slice<typename U::value_type>;

template <class U>
Slice(const U&) -> Slice<const typename U::value_type>;