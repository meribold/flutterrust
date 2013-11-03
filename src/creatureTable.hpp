#ifndef CREATURE_TABLE_HPP
#define CREATURE_TABLE_HPP

#include <istream>
#include <string>
#include <vector>

template <typename CreatureType, typename Extractors>
std::vector<CreatureType>
loadCreatureTypes(std::istream&&, Extractors, std::vector<std::string>& errors);

template <typename CreatureType>
void printCreatureTypes(const std::vector<CreatureType>& vector);

#include "creatureTable.ipp"

#endif

