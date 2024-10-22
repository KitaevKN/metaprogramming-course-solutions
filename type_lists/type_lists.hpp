#pragma once

#include <concepts>
#include <type_tuples.hpp>

namespace type_lists {

template <class TL>
concept TypeSequence = requires {
  typename TL::Head;
  typename TL::Tail;
};

struct Nil {};

template <class TL>
concept Empty = std::derived_from<TL, Nil>;

template <class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

template <class T, TypeList TL>
struct Cons {
  using Head = T;
  using Tail = TL;
};

template <class T>
struct Repeat {
  using Head = T;
  using Tail = Repeat<T>;
};

namespace detail {
template <TypeList TL, class... Ts>
struct ToTuple {
  using Type =
      typename ToTuple<typename TL::Tail, Ts..., typename TL::Head>::Type;
};

template <Empty TL, class... Ts>
struct ToTuple<TL, Ts...> {
  using Type = type_tuples::TTuple<Ts...>;
};
}  // namespace detail

template <TypeList TL, class... Ts>
using ToTuple = detail::ToTuple<TL, Ts...>::Type;

namespace detail {
template <type_tuples::TypeTuple TT>
struct FromTuple;

template <class Head, class... Tail>
struct FromTuple<type_tuples::TTuple<Head, Tail...>> {
  using Type =
      Cons<Head, typename FromTuple<type_tuples::TTuple<Tail...>>::Type>;
};

template <>
struct FromTuple<type_tuples::TTuple<>> {
  using Type = Nil;
};
}  // namespace detail

template <type_tuples::TypeTuple Ts>
using FromTuple = typename detail::FromTuple<Ts>::Type;

namespace detail {
template <std::size_t size, TypeList TL>
struct Take {
  using Head = typename TL::Head;
  using Tail = Take<size - 1, typename TL::Tail>;
};

template <TypeList TL>
struct Take<0, TL> : Nil {};

template <std::size_t size, Empty TL>
struct Take<size, TL> : Nil {};
}  // namespace detail

template <std::size_t size, TypeList TL>
using Take = typename detail::Take<size, TL>;

namespace detail {
template <std::size_t size, TypeList TL>
struct Drop {
  using Type = Drop<size - 1, typename TL::Tail>::Type;
};

template <std::size_t size, Empty TL>
struct Drop<size, TL> {
  using Type = TL;
};

template <TypeList TL>
struct Drop<0, TL> {
  using Type = TL;
};
}  // namespace detail

template <std::size_t size, TypeList TL>
using Drop = typename detail::Drop<size, TL>::Type;

template <std::size_t size, typename T>
using Replicate = Take<size, Repeat<T>>;

template <template <class, class...> class Obj, class T>
struct Iterate {
  using Head = T;
  using Tail = Iterate<Obj, Obj<T>>;
};

namespace detail {
template <TypeList TL1, TypeList TL2>
struct Cycle {
  using Head = TL1::Head;
  using Tail = Cycle<typename TL1::Tail, TL2>;
};

template <Empty TL1, TypeList TL2>
struct Cycle<TL1, TL2> {
  using Head = TL2::Head;
  using Tail = Cycle<typename TL2::Tail, TL2>;
};

template <Empty TL1, Empty TL2>
struct Cycle<TL1, TL2> : Nil {};
}  // namespace detail

template <TypeList TL>
using Cycle = detail::Cycle<TL, TL>;

namespace detail {
template <template <class, class...> class Obj, TypeList TL>
struct Map {
  using Head = Obj<typename TL::Head>;
  using Tail = Map<Obj, typename TL::Tail>;
};

template <template <class, class...> class Obj, Empty TL>
struct Map<Obj, TL> : Nil {};
}  // namespace detail

template <template <class, class...> class Obj, TypeList TL>
using Map = detail::Map<Obj, TL>;

namespace detail {
template <template <class, class...> class P, TypeList TL>
struct Filter : Filter<P, typename TL::Tail> {};

template <template <class, class...> class P, TypeList TL>
  requires(P<typename TL::Head>::Value)
struct Filter<P, TL> {
  using Head = TL::Head;
  using Tail = Filter<P, typename TL::Tail>;
};

template <template <class, class...> class P, Empty TL>
struct Filter<P, TL> : Nil {};
}  // namespace detail

template <template <class, class...> class Obj, TypeList TL>
using Filter = detail::Filter<Obj, TL>;

namespace detail {
template <template <class, class> class Obj, class T, TypeList TL>
struct Scanl {
  using Head = Obj<T, typename TL::Head>;
  using Tail = Scanl<Obj, typename TL::Head, typename TL::Tail>;
};

template <template <class, class> class Obj, class T, Empty TL>
struct Scanl<Obj, T, TL> : Nil {};
}  // namespace detail

template <template <class, class> class Obj, class T, TypeList TL>
struct Scanl {
  using Head = T;
  using Tail = detail::Scanl<Obj, T, TL>;
};

namespace detail {
template <template <class, class> class Obj, class T, TypeList TL>
struct Foldl {
  using Type = Foldl<Obj, Obj<T, typename TL::Head>, typename TL::Tail>::Type;
};

template <template <class, class> class Obj, class T, Empty TL>
struct Foldl<Obj, T, TL> {
  using Type = T;
};
}  // namespace detail

template <template <class, class> class Obj, class T, TypeList TL>
using Foldl = detail::Foldl<Obj, T, TL>::Type;

namespace detail {
template <template <class, class> class Obj, class T, TypeList TL>
struct Foldr {
  using Type =
      Obj<typename TL::Head, typename Foldr<Obj, T, typename TL::Tail>::Type>;
};

template <template <class, class> class Obj, class T, Empty TL>
struct Foldr<Obj, T, TL> {
  using Type = T;
};
}  // namespace detail

template <template <class, class> class Obj, class T, TypeList TL>
using Foldr = detail::Foldr<Obj, T, TL>::Type;

namespace detail {
template <TypeList TL, typename... Ts>
struct Inits {
  using Head = type_lists::FromTuple<type_tuples::TTuple<Ts...>>;
  using Tail = Inits<typename TL::Tail, Ts..., typename TL::Head>;
};

template <Empty TL, typename... Ts>
struct Inits<TL, Ts...> {
  using Head = type_lists::FromTuple<type_tuples::TTuple<Ts...>>;
  using Tail = Nil;
};
}  // namespace detail

template <TypeList TL>
using Inits = detail::Inits<TL>;

namespace detail {
template <TypeList TL>
struct Tails {
  using Head = TL;
  using Tail = Tails<typename TL::Tail>;
};

template <Empty TL>
struct Tails<TL> {
  using Head = Nil;
  using Tail = Nil;
};
}  // namespace detail

template <TypeList TL>
using Tails = detail::Tails<TL>;

namespace detail {
template <TypeList TL1, TypeList TL2>
struct Zip2 {
  using Head = type_tuples::TTuple<typename TL1::Head, typename TL2::Head>;
  using Tail = Zip2<typename TL1::Tail, typename TL2::Tail>;
};

template <Empty TL1, TypeList TL2>
struct Zip2<TL1, TL2> : Nil {};

template <TypeList TL1, Empty TL2>
struct Zip2<TL1, TL2> : Nil {};

template <Empty TL1, Empty TL2>
struct Zip2<TL1, TL2> : Nil {};
}  // namespace detail

template <TypeList TL1, TypeList TL2>
using Zip2 = detail::Zip2<TL1, TL2>;

namespace detail {
template <TypeList... TL>
struct Zip {
  using Head = type_tuples::TTuple<typename TL::Head...>;
  using Tail = Zip<typename TL::Tail...>;
};
}  // namespace detail

template <TypeList... TL>
using Zip = detail::Zip<TL...>;

}  // namespace type_lists
