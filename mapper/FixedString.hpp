#pragma once

#include <cstddef>
#include <string_view>

namespace detail {
template <std::size_t Index, std::size_t End>
struct loop {
  constexpr loop(const char* from, char to[]) {
    to[Index] = from[Index];

    if (from[Index] != '\0') loop<Index + 1, End> call(from, to);
  }
};

template <std::size_t End>
struct loop<End, End> {
  constexpr loop(const char* from, char to[]) { to[End] = from[End]; }
};
}  // namespace detail
template <std::size_t max_length>
struct FixedString {
  constexpr FixedString(const char* string, std::size_t size) : len(size) {
    detail::loop</*Begin*/ 0, /*End*/ max_length>(string, data);
  }

  constexpr operator std::string_view() const {
    return std::string_view(data, len);
  }

  char data[max_length]{};
  const std::size_t len;
};

constexpr FixedString<256> operator"" _cstr(const char* string,
                                            std::size_t size) {
  return FixedString<256>(string, size);
}
