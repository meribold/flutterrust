#ifndef RESOURCE_PARSER_HPP_E1ZJ3OTU
#define RESOURCE_PARSER_HPP_E1ZJ3OTU

#include <istream>
#include <string>
#include <tuple>
#include <vector>

template <typename CreatureType, typename Extractors>
std::vector<CreatureType>
loadResources(std::istream&&, Extractors, std::vector<std::string>& errors);

/*
template<std::size_t i = 0, typename... Ts>
inline typename std::enable_if<i < sizeof...(Ts) - 1, void>::type
printCreatureType(const std::tuple<Ts...>& t);
*/

#include "resource_parser.ipp"

#endif // RESOURCE_PARSER_HPP_E1ZJ3OTU
