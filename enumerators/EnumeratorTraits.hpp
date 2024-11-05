#pragma once

#include <cstddef>
#include <type_traits>
#include <limits>
#include <string_view>
#include <cstdint>


namespace detail {

template<class Enum, Enum Name>
consteval auto name() {
    return __PRETTY_FUNCTION__;
}

template<class Enum, Enum value>
constexpr bool isEnumPart() {
    constexpr std::string_view name = detail::name<Enum, value>();
    constexpr auto pos = name.find("::", sizeof("auto detail::name() [Enum ="));

    return pos != std::string_view::npos;
}
template<class Enum, Enum value>
constexpr std::string_view bind() {
    constexpr std::string_view name = detail::name<Enum, value>();

    constexpr auto begin = name.find("::", sizeof("auto detail::name() [Enum =")) + 2;
    constexpr auto end = name.find(']', begin);

    return name.substr(begin, end - begin);
}

template <class Enum, std::int64_t Index, std::int64_t End>
struct loop {
  constexpr loop(std::int32_t& count) {
    if(isEnumPart<Enum, static_cast<Enum>(Index)>())
        count += 1;

    if(isEnumPart<Enum, static_cast<Enum>(Index + 1)>())
        count += 1;

    if(isEnumPart<Enum, static_cast<Enum>(Index + 2)>())
        count += 1;

    if(isEnumPart<Enum, static_cast<Enum>(Index + 3)>())
        count += 1;

    loop<Enum, Index + 2, End> cycle(count);
  }
};

template <class Enum, std::int64_t End>
struct loop<Enum, End + 1, End> {
  constexpr loop(std::int32_t& count) {
    if(isEnumPart<Enum, static_cast<Enum>(End)>())
        count += 1;
  }
};

template <class Enum, std::int64_t End>
struct loop<Enum, End, End> {
  constexpr loop(std::int32_t& count) {
    if(isEnumPart<Enum, static_cast<Enum>(End)>())
        count += 1;
  }
};
}

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr const auto NUMERIC_MAX = static_cast<const int>(std::numeric_limits<std::underlying_type_t<Enum>>().max());
    static constexpr const auto NUMERIC_MIN = static_cast<const int>(std::numeric_limits<std::underlying_type_t<Enum>>().min());

    static consteval std::size_t sizeEnum() {
        std::size_t count = 0;

        std::int16_t MIN = -static_cast<std::int16_t>(MAXN);
        std::int16_t MAX = static_cast<std::int16_t>(MAXN);

        const std::size_t shift = sizeof("auto detail::name() [Enum =");
        for(std::int16_t i = MIN; i != MAX; ++i) {
            std::string_view name = detail::name<Enum, static_cast<Enum>(i)>();
            std::size_t pos = name.find("::", shift);
            if(pos != std::string_view::npos)
                count += 1;
        }
        return count;
    }

    static constexpr std::size_t size() noexcept {
       return sizeEnum();
    }

    static constexpr Enum at(std::size_t i) noexcept {
        return {};
    }

    static constexpr std::string_view nameAt(std::size_t i) noexcept {
        return detail::bind<Enum, at(i)>();
    }
};