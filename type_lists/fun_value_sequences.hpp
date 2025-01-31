#pragma once

#include <type_lists.hpp>
#include <value_types.hpp>

namespace detail {
template <int Val>
struct Nats {
  using Head = value_types::ValueTag<Val>;
  using Tail = Nats<Val + 1>;
};

template <>
struct Nats<1024> : type_lists::Nil {};

template <int First, int Second>
struct Fib {
  using Head = value_types::ValueTag<First>;
  using Tail = Fib<Second, First + Second>;
};

template <class Val>
struct Primes {
  static consteval bool Result() {
    for (int i = 2; i != Val::Value; ++i)
      if (Val::Value % i == 0) return false;
    return true;
  }

  static constexpr bool Value = Result();
};

template <>
struct Primes<value_types::ValueTag<0>> {
  static constexpr bool Value = false;
};

template <>
struct Primes<value_types::ValueTag<1>> {
  static constexpr bool Value = false;
};

template <>
struct Primes<value_types::ValueTag<2>> {
  static constexpr bool Value = true;
};
}  // namespace detail

using Nats = detail::Nats<0>;

using Fib = detail::Fib<0, 1>;

using Primes = type_lists::Filter<detail::Primes, Nats>;