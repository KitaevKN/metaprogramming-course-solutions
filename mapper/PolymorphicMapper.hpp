#include <optional>
#include <type_traits>

template <class From, auto target>
struct Mapping {
  using Type = const From*;

  static constexpr auto Value() { return target; }
};

template <class Base, class Target, class Object, class... Objects>
static std::optional<Target> Finder(const Base& object) {
  if (const auto* S = dynamic_cast<typename Object::Type>(&object); S) {
    return Object::Value();
  }
  return Finder<Base, Target, Objects...>(object);
}

template <class Base, class Target>
static std::optional<Target> Finder(const Base&) {
  return std::nullopt;
}

template <class Base, class Target, class... Mappings>
struct PolymorphicMapper {
  static std::optional<Target> map(const Base& object) {
    return Finder<Base, Target, Mappings...>(object);
  }
};