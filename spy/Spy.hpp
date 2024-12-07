#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace detail {
template <class T>
concept is_nothrow_equality_comparable_v = requires(T a, T b) {
  { a == b } noexcept -> std::same_as<bool>;
};

template <class T>
concept is_nothrow_three_way_comparable_v = requires(T a, T b) {
  { a <=> b } noexcept -> std::same_as<bool>;
};

struct PureLoggerHolder {
  void operator()(unsigned int i) { this->Log(i); }

  virtual void Log(unsigned int) = 0;

  virtual ~PureLoggerHolder() noexcept = default;
};

template <class Logger>
struct LoggerStorage : public PureLoggerHolder {
 private:
  Logger logger;

 public:
  constexpr LoggerStorage() noexcept(
      std::is_nothrow_default_constructible_v<Logger>)
    requires(std::is_default_constructible_v<Logger>)
  = default;

  constexpr LoggerStorage(const LoggerStorage&) noexcept(
      std::is_nothrow_copy_constructible_v<Logger>)
    requires(std::is_copy_constructible_v<Logger>)
  = default;

  constexpr LoggerStorage(LoggerStorage&&) noexcept(
      std::is_nothrow_move_constructible_v<Logger>)
    requires(std::is_move_constructible_v<Logger>)
  = default;

  template <class Func>
  LoggerStorage(Func&& f) : logger(std::forward<Func>(f)) {}

  void Log(unsigned int cnt) noexcept(
      std::is_nothrow_invocable_v<Logger, unsigned int>) override {
    static_assert(std::invocable<decltype(logger), unsigned int>);
    this->logger(cnt);
  }

  ~LoggerStorage() noexcept(std::is_nothrow_destructible_v<Logger>) override =
      default;
};
}  // namespace detail

// Allocator is only used in bonus SBO tests,
// ignore if you don't need bonus points.
template <class T, class Allocator = std::allocator<std::byte>>
class Spy {
 public:
  using value_type = T;
  using allocator_type = Allocator;
  using pure_logger_type = detail::PureLoggerHolder;
  using pure_logger_ptr = detail::PureLoggerHolder*;
  template <std::invocable<unsigned int> _log>
  using logger_storage = detail::LoggerStorage<_log>;

 private:
  value_type value_;

  allocator_type alloc_;

  pure_logger_ptr storage_ = nullptr;

  template <std::invocable<unsigned int> Logger>
  static consteval bool is_same_constructible_v() {
    bool result = true;
    if (std::is_move_constructible_v<Spy<T>>)
      result &= std::is_move_constructible_v<Logger>;
    if (std::is_copy_constructible_v<Spy<T>>)
      result &= std::is_copy_constructible_v<Logger>;
    if (std::is_move_assignable_v<Spy<T>>)
      result &= std::is_move_assignable_v<Logger>;
    if (std::is_copy_assignable_v<Spy<T>>)
      result &= std::is_copy_assignable_v<Logger>;
    if (std::is_nothrow_destructible_v<Spy<T>>)
      result &= std::is_nothrow_destructible_v<Logger>;
    return result;
  }

  void destroy_storage() {
    if (storage_ != nullptr) {
      delete storage_;
      storage_ = nullptr;
    }
  }

 public:
  static_assert(!std::is_reference_v<value_type>);

  constexpr Spy() noexcept(std::is_nothrow_default_constructible_v<value_type>)
    requires(std::is_default_constructible_v<value_type>)
  = default;

  constexpr Spy(const Spy&) noexcept(
      std::is_nothrow_copy_constructible_v<value_type>)
    requires(std::is_copy_constructible_v<value_type>)
  = default;

  constexpr Spy(Spy&&) noexcept(
      std::is_nothrow_move_constructible_v<value_type>)
    requires(std::is_move_constructible_v<value_type>)
  = default;

  constexpr Spy& operator=(const Spy&) noexcept(
      std::is_nothrow_copy_assignable_v<value_type>)
    requires(std::is_copy_assignable_v<value_type>)
  = default;

  constexpr Spy& operator=(Spy&&) noexcept(
      std::is_nothrow_move_assignable_v<value_type>)
    requires(std::is_move_constructible_v<value_type>)
  = default;

  constexpr Spy(const allocator_type& alloc) noexcept(
      std::is_nothrow_copy_constructible_v<allocator_type> &&
      std::is_nothrow_default_constructible_v<value_type>)
      : alloc_(alloc) {}

  constexpr Spy(allocator_type&& alloc) noexcept(
      std::is_nothrow_move_constructible_v<allocator_type> &&
      std::is_nothrow_default_constructible_v<value_type>)
    requires(std::is_move_constructible_v<allocator_type> &&
             std::is_default_constructible_v<value_type>)
      : alloc_(alloc) {}

  template <typename U = value_type>
    requires(std::is_constructible_v<value_type, U> &&
             std::is_convertible_v<U, value_type>)
  constexpr Spy(value_type&& value) noexcept(
      std::is_nothrow_constructible_v<T, value_type>)
      : value_(std::forward<value_type>(value)) {}

  ~Spy() noexcept(std::is_nothrow_destructible_v<value_type>) {
    destroy_storage();
    value_.~value_type();
  }

  constexpr bool operator<=>(const Spy& other) const
      noexcept(detail::is_nothrow_three_way_comparable_v<value_type>)
    requires(std::three_way_comparable<value_type>)
  {
    return other.value_ == value_;
  }

  constexpr bool operator==(const Spy& other) const
      noexcept(detail::is_nothrow_equality_comparable_v<value_type>)
    requires(std::equality_comparable<value_type>)
  {
    return other.value_ == value_;
  }

  T& operator*() noexcept { return std::addressof(value_); }

  const T& operator*() const noexcept { return std::addressof(value_); }

  T* operator->() noexcept {
    static unsigned int counter = 0;
    storage_->Log(counter);
    return std::addressof(value_);
  }

  const T* operator->() const noexcept { return std::addressof(value_); }

  void setLogger() { destroy_storage(); }

  template <std::invocable<unsigned int> Logger>
    requires(is_same_constructible_v<Logger>())
  void setLogger(Logger&& logger) noexcept {
    destroy_storage();
    storage_ = new logger_storage<Logger>(std::forward<Logger>(logger));
  }
};