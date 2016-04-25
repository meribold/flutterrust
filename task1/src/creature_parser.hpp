#ifndef CREATURE_PARSER_HPP_BCCMAXUG
#define CREATURE_PARSER_HPP_BCCMAXUG

#include <istream>
#include <string>
#include <tuple>
#include <vector>

#include "resource_parser.hpp"

typedef std::tuple<std::string, int, int, int, std::string, std::string> CreatureType;

std::vector<CreatureType> loadCreatureTypes(std::istream&&);

std::vector<CreatureType>
loadCreatureTypes(std::istream&&, std::vector<std::string>& errors);

#include "creature_parser.ipp"

#endif // CREATURE_TABLE_HPP_AMPCHIJX

// vim: tw=90 sts=-1 sw=3 et
