#ifndef CREATURE_PARSER_HPP_BCCMAXUG
#define CREATURE_PARSER_HPP_BCCMAXUG

#include <istream>
#include <string>
#include <tuple>
#include <vector>

#include "resource_parser.hpp"

using CreatureType = std::tuple<std::string, int, int, int, std::string, std::string>;

/*
// Names for the tuple elements to avoid unhelpful code like `std::get<4>(typle).
enum class CTFields { name, strength, speed, lifetime, attributes, bitmap };
*/

// Names for the tuple elements to avoid unhelpful code like `std::get<4>(typle).
constexpr struct {
   std::size_t name = 0, strength = 1, speed = 2, lifetime = 3, attributes = 4,
               bitmap = 5;
} cTFields;

std::vector<CreatureType> loadCreatureTypes(std::istream&&);

std::vector<CreatureType> loadCreatureTypes(std::istream&&,
   std::vector<std::string>& errors);

#include "creature_parser.ipp"

#endif // CREATURE_PARSER_HPP_BCCMAXUG

// vim: tw=90 sts=-1 sw=3 et
