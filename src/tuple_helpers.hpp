#ifndef TUPLE_HELPERS_HPP_5PDXFRA6
#define TUPLE_HELPERS_HPP_5PDXFRA6

#include <tuple>
#include <type_traits> // enable_if, underlying_type_t

// Apply a functor template to all members of an std::tuple.
// http://stackoverflow.com/q/1198260/iterate-over-tuple
// http://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error
template <template <typename T> class Function, std::size_t i = 0, typename... Ts>
inline typename std::enable_if<i == sizeof...(Ts), void>::type
forEach(const std::tuple<Ts...>&) {}

template <template <typename T> class Function, std::size_t i = 0, typename... Ts>
inline typename std::enable_if<i < sizeof...(Ts), void>::type
forEach(const std::tuple<Ts...>& t)
{
   constexpr Function<typename std::tuple_element<i, std::tuple<Ts...>>::type> f{};
   f(std::get<i>(t));
   forEach<Function, i + 1>(t);
}

template <typename T>
struct Print {
   void operator()(const T& t) const {
      std::cout << "'" << t << "', ";
   }
};

template <>
struct Print<int> {
   void operator()(const int& i) const {
      std::cout << i << ", ";
   }
};

template <typename... Ts>
inline void printTuple(const std::tuple<Ts...>& t) {
   forEach<Print>(t);
}

// Get the underlying type of an enumerator.  Useful to access tuple elements with
// std::get and a symbolic name with reduced verbosity and without using an unscoped enum
// for the symbolic names (e.g., `std::get<toUT(EnumType::enumerator)>(enumObject)`).  See
// item 10 from "Effective Modern C++ (Scott Meyers).  Requires C++14.
template <typename E>
constexpr auto toUT(E enumerator) noexcept {
   return static_cast<std::underlying_type_t<E>>(enumerator);
}

#endif // TUPLE_HELPERS_HPP_5PDXFRA6

// vim: tw=90 sts=-1 sw=3 et
