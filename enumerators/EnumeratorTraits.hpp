#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace detail {
template <class Enum, Enum Name>
consteval auto name() {
  return __PRETTY_FUNCTION__;
}

template <class Enum, std::size_t N>
consteval bool isEnum() {
  std::string_view name = detail::name<Enum, static_cast<Enum>(N)>();
  auto pos = name.find('(', sizeof("auto detail::name() [Enum ="));
  return pos == std::string_view::npos;
}

template <class Enum, std::size_t N>
  requires(!std::is_convertible_v<Enum, std::underlying_type_t<Enum>>)
consteval std::string_view bind() {
  std::string_view name = detail::name<Enum, static_cast<Enum>(N)>();

  auto begin = name.find("::", sizeof("auto detail::name() [Enum =")) + 2;
  auto end = name.find(']', begin);

  return (begin - 2) == std::string_view::npos
             ? std::string_view()
             : name.substr(begin, end - begin);
}

template <class Enum, std::size_t N>
  requires(std::is_convertible_v<Enum, std::underlying_type_t<Enum>>)
consteval std::string_view bind() {
  std::string_view name = detail::name<Enum, static_cast<Enum>(N)>();

  auto begin = name.find('=', sizeof("auto detail::name() [Enum =")) + 2;
  auto end = name.find(']', begin);
  return name.find('(', begin) != std::string_view::npos
             ? std::string_view()
             : name.substr(begin, end - begin);
}

template <class Enum, std::uint64_t MAXN, std::int64_t MINN>
consteval auto size() {
  std::size_t size = 0;
  if constexpr (std::is_signed_v<std::underlying_type_t<Enum>>) {
    [&size]<std::size_t... I>(std::index_sequence<I...>) {
      (
          [&size]() {
            if (isEnum<Enum, I + MINN>()) ++size;
          }(),
          ...);
    }(std::make_index_sequence<-MINN>());
  }

  [&size]<std::size_t... I>(std::index_sequence<I...>) {
    (
        [&size]() {
          if (isEnum<Enum, I>()) ++size;
        }(),
        ...);
  }(std::make_index_sequence<MAXN + 1>());

  return size;
}

template <class Enum, std::uint64_t ENUM_MAX, std::int64_t ENUM_MIN>
consteval auto builder() {
  constexpr std::size_t Size = detail::size<Enum, ENUM_MAX, ENUM_MIN>();
  std::size_t index = 0;
  std::array<Enum, Size> Enums;
  std::array<std::string_view, Size> Names;

  if constexpr (std::is_signed_v<std::underlying_type_t<Enum>>) {
    [&Enums, &Names, &index]<std::size_t... I>(std::index_sequence<I...>) {
      (
          [&Enums, &Names, &index]() {
            std::string_view name = detail::bind<Enum, I + ENUM_MIN>();
            if (!name.empty()) {
              Enums[index] = static_cast<Enum>(I + ENUM_MIN);
              Names[index] = name;
              index += 1;
            }
          }(),
          ...);
    }(std::make_index_sequence<-ENUM_MIN>());
  }

  [&Enums, &Names, &index]<std::size_t... I>(std::index_sequence<I...>) {
    (
        [&Enums, &Names, &index]() {
          std::string_view name = detail::bind<Enum, I>();
          if (!name.empty()) {
            Enums[index] = static_cast<Enum>(I);
            Names[index] = name;
            index += 1;
          }
        }(),
        ...);
  }(std::make_index_sequence<ENUM_MAX + 1>());
  return std::make_tuple(Size, Enums, Names);
}
}  // namespace detail

template <class Enum, std::size_t MAXN = 512>
struct EnumeratorTraits {
 public:
  static_assert(std::is_enum_v<Enum>);

  static constexpr std::uint64_t ENUM_MAX =
      std::min(static_cast<std::uint64_t>(
                   std::numeric_limits<std::underlying_type_t<Enum>>().max()),
               static_cast<std::uint64_t>(MAXN));

  static constexpr std::int64_t ENUM_MIN =
      std::max(static_cast<std::int64_t>(
                   std::numeric_limits<std::underlying_type_t<Enum>>().min()),
               -static_cast<std::int64_t>(MAXN));

 private:
  static constexpr auto storage = detail::builder<Enum, ENUM_MAX, ENUM_MIN>();

 public:
  static constexpr std::size_t size() noexcept { return std::get<0>(storage); }

  static constexpr Enum at(std::size_t i) noexcept {
    return std::get<1>(storage)[i];
  }

  static constexpr std::string_view nameAt(std::size_t i) noexcept {
    return std::get<2>(storage)[i];
  }
};
