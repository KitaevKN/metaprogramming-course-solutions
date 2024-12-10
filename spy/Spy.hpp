#pragma once

#include <concepts>
#include <functional>
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

struct PureLogger {
  auto operator()(unsigned int i) { this->Log(i); }

  virtual void Log(unsigned int) = 0;

  virtual ~PureLogger() noexcept = default;

  virtual PureLogger* copy() = 0;
};

template <class Logger>
struct LoggerStorage : public PureLogger {
 private:
  Logger logger;

 public:
  constexpr LoggerStorage() noexcept(
      std::is_nothrow_default_constructible_v<Logger>)
    requires(std::is_default_constructible_v<Logger>)
  = default;

  LoggerStorage& operator=(const LoggerStorage&) = delete;

  LoggerStorage& operator=(LoggerStorage&&) = delete;

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

  PureLogger* copy() override {
    if constexpr (std::is_copy_constructible_v<Logger>) {
      return new LoggerStorage(logger);
    }
    return nullptr;
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
  using pure_logger_type = detail::PureLogger;
  template <std::invocable<unsigned int> _log>
  using logger_type = detail::LoggerStorage<_log>;
  using value_type = T;
  using allocator_type = Allocator;

 private:
  class SpyRefer {
    Spy& spy_;

   public:
    SpyRefer(Spy& spy) : spy_(spy) {}

    ~SpyRefer() {
      if (--spy_.reffer_cnt == 0) {
        if (spy_.storage_) std::invoke(*spy_.storage_, spy_.call_cnt);
        spy_.call_cnt = 0;
      }
    }

    value_type* operator->() { return &spy_.value_; }
  };

  mutable unsigned int reffer_cnt = 0;

  mutable unsigned int call_cnt = 0;

  value_type value_;

  allocator_type alloc_;

  std::shared_ptr<pure_logger_type> storage_ = nullptr;

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
    if (std::is_nothrow_move_constructible_v<Spy<T>>)
      result &= std::is_nothrow_move_constructible_v<Logger>;
    if (std::is_nothrow_copy_constructible_v<Spy<T>>)
      result &= std::is_nothrow_copy_constructible_v<Logger>;
    if (std::is_nothrow_destructible_v<Spy<T>>)
      result &= std::is_nothrow_destructible_v<Logger>;
    return result;
  }

 public:
  static_assert(!std::is_reference_v<value_type>);

  constexpr Spy() noexcept(std::is_nothrow_default_constructible_v<value_type>)
    requires(std::is_default_constructible_v<value_type>)
  = default;

  constexpr Spy(const Spy& other) noexcept(
      std::is_nothrow_copy_constructible_v<value_type>)
    requires(std::is_copy_constructible_v<value_type>)
      : value_(other.value_), alloc_(other.alloc_) {
    if (other.storage_) storage_.reset(other.storage_->copy());
  }

  constexpr Spy(Spy&& other) noexcept(
      std::is_nothrow_move_constructible_v<value_type>)
    requires(std::is_move_constructible_v<value_type>)
      : value_(std::move(other.value_)),
        alloc_(std::move(other.alloc_)),
        storage_(std::move(other.storage_)) {}

  constexpr Spy& operator=(const Spy& other) noexcept(
      std::is_nothrow_copy_assignable_v<value_type>)
    requires(std::is_copy_assignable_v<value_type>)
  {
    call_cnt = 0;
    reffer_cnt = 0;
    value_ = other.value_;

    if (other.storage_)
      storage_.reset(other.storage_->copy());
    else
      storage_.reset();

    if constexpr (std::is_copy_assignable_v<allocator_type>)
      alloc_ = other.alloc_;
    return *this;
  }

  constexpr Spy& operator=(Spy&& other) noexcept(
      std::is_nothrow_move_assignable_v<value_type>)
    requires(std::is_move_assignable_v<value_type>)
  {
    call_cnt = 0;
    reffer_cnt = 0;
    value_ = std::move(other.value_);

    if (other.storage_)
      storage_ = std::move(other.storage_);
    else
      storage_.reset();

    if constexpr (std::is_move_assignable_v<allocator_type>)
      alloc_ = std::move(other.alloc_);
    return *this;
  }

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
    value_.~value_type();
  }

  constexpr auto operator<=>(const Spy& other) const
      noexcept(detail::is_nothrow_three_way_comparable_v<value_type>)
    requires(std::three_way_comparable<value_type>)
  {
    return other.value_ == value_;
  }

  constexpr auto operator==(const Spy& other) const
      noexcept(detail::is_nothrow_equality_comparable_v<value_type>)
    requires(std::equality_comparable<value_type>)
  {
    return other.value_ == value_;
  }

  auto& operator*() noexcept { return std::addressof(value_); }

  const auto& operator*() const noexcept { return std::addressof(value_); }

  auto operator->() noexcept {
    call_cnt++;
    reffer_cnt++;
    return SpyRefer(*this);
  }

  auto operator->() const noexcept {
    call_cnt++;
    reffer_cnt++;
    return std::addressof(value_);
  }

  void setLogger() { storage_.reset(); }

  template <std::invocable<unsigned int> Logger>
    requires(is_same_constructible_v<Logger>())
  void setLogger(Logger&& logger) noexcept {
    storage_.reset(new logger_type<Logger>(std::forward<Logger>(logger)));
  }
};