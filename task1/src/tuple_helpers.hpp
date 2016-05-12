#ifndef TUPLE_HELPERS_HPP_5PDXFRA6
#define TUPLE_HELPERS_HPP_5PDXFRA6

#include <tuple>
#include <type_traits> // enable_if

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

#endif // TUPLE_HELPERS_HPP_5PDXFRA6

// vim: tw=90 sts=-1 sw=3 et
