#ifndef TUPLE_HELPERS_HPP_5PDXFRA6
#define TUPLE_HELPERS_HPP_5PDXFRA6

#include <tuple>
#include <type_traits>

// Apply a functor template to all members on an std::tuple.
template<template <typename T> class Function, std::size_t i = 0, typename... Tuple>
inline typename std::enable_if<i == sizeof...(Tuple), void>::type
forEach(const std::tuple<Tuple...>&) {}

template<template <typename T> class Function, std::size_t i = 0, typename... Tuple>
inline typename std::enable_if<i < sizeof...(Tuple), void>::type
forEach(const std::tuple<Tuple...>& t)
{
   Function<typename std::tuple_element<i, std::tuple<Tuple...>>::type> f{};
   f(std::get<i>(t));
   forEach<Function, i + 1>(t);
   //forEach<Function, i + 1>(t, Function<typename std::tuple_element<i + 1,
      //std::tuple<Tuple...>>::type>{});
}

template <typename T>
struct Print {
   void operator()(const T& t) {
      std::cout << "'" << t << "', ";
   }
};

template <>
struct Print<int> {
   void operator()(const int& i) {
      std::cout << i << ", ";
   }
};

/*
// http://stackoverflow.com/q/1198260/iterate-over-tuple
// http://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error
template<std::size_t i = 0, typename... Ts>
inline typename std::enable_if<i == sizeof...(Ts) - 1, void>::type
printCreatureType(const std::tuple<Ts...>& t)
{
   std::cout << '(' << std::get<i>(t) << ')' << std::endl;
}

template<std::size_t i = 0, typename... Ts>
inline typename std::enable_if<i < sizeof...(Ts) - 1, void>::type
printCreatureType(const std::tuple<Ts...>& t)
{
   std::cout << '(' << std::get<i>(t) << "), ";
   printCreatureType<i + 1, Ts...>(t);
}
*/

#endif // TUPLE_HELPERS_HPP_5PDXFRA6

// vim: tw=90 sts=-1 sw=3 et
