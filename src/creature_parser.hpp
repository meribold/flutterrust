#ifndef CREATURE_PARSER_HPP_BCCMAXUG
#define CREATURE_PARSER_HPP_BCCMAXUG

#include <istream>
#include <string>
#include <vector>

#include "creature_type.hpp"
#include "resource_parser.hpp"

std::vector<CreatureType> loadCreatureTypes(std::istream&&,
                                            std::vector<std::string>& errors);

// For those who don't care.
std::vector<CreatureType> loadCreatureTypes(std::istream&&);

<<<<<<< HEAD
#include "creature_parser.ipp"

=======
>>>>>>> f1b64c48c4eb5420aea5792d47374f13945b73ac
#endif  // CREATURE_PARSER_HPP_BCCMAXUG

// vim: tw=90 sts=-1 sw=3 et
