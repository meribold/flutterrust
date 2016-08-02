#include "creature_parser.hpp"

#include <algorithm>  // std::transform
#include <cctype>     // std::tolower (TODO: use the one from <locale>?)
#include <set>        // std::set
#include <sstream>    // std::stringstream
#include <string>     // std::string
#include <tuple>      // std::make_tuple
#include <utility>    // std::move
#include <vector>     // std::vector

#ifdef DEBUG
#include <iostream>
#endif

auto getAttrs = [](std::string s) -> CreatureAttrs {
   // Assert we get exactly one string out of each of these sets: {Landbewohner,
   // Wasserbewohner}, {Pflanze, Tier} and, only in case we got "Tier", {Pflanzenfresser,
   // Fleischfresser}.  Disregard capitalization.
   std::transform(s.begin(), s.end(), s.begin(), [](char c) { return std::tolower(c); });
   std::stringstream stream{s};
   std::set<std::string> words;
   {
      std::string attribute;
      while (stream >> attribute) {
         words.insert(attribute);
      }
   }
   std::bitset<3> bitset;
   if (words.find("landbewohner") != words.end()) {
      bitset.flip(0);
      if (words.find("wasserbewohner") != words.end()) {
         throw std::string{
             "\"Wasserbewohner\" and \"Landbewohner\" are mutually exclusive"};
      }
   } else if (words.find("wasserbewohner") == words.end()) {
      throw std::string{"expected \"Wasserbewohner\" or \"Landbewohner\" in attributes"};
   }
   if (words.find("tier") != words.end()) {
      bitset.flip(1);
      if (words.find("pflanze") != words.end()) {
         throw std::string{"\"Pflanze\" and \"Tier\" are mutually exclusive"};
      }
   } else if (words.find("pflanze") == words.end()) {
      throw std::string{"expected \"Pflanze\" or \"Tier\" in attributes"};
   }
   if (!bitset.test(1)) {
      // Plants should have exactly two attributes.
      if (words.size() > 2) {
         throw std::string{"unexpected attributes encountered"};
      }
   } else {
      if (words.find("fleischfresser") != words.end()) {
         bitset.flip(2);
         if (words.find("pflanzenfresser") != words.end()) {
            throw std::string{
                "\"Pflanzenfresser\" and \"Fleischfresser\" are mutually exclusive"};
         }
      } else if (words.find("pflanzenfresser") == words.end()) {
         throw std::string{
             "expected \"Pflanzenfresser\" or \"Fleischfresser\" in attributes"};
      }
      if (words.size() > 3) {
         throw std::string{"unexpected attributes encountered"};
      }
   }
   /*
   if (s.find("landbewohner") != std::string::npos) {
      // It's terrestrial.
      bitset.flip(0);
   }
   if (s.find("wasserbewohner") != std::string::npos) {
      // It's aquatic.
      if (bitset.test(0)) {
         // But it's already a land thing.
         throw std::string{
             "\"Landbewohner\" and \"Wasserbewohner\" are mutually exclusive"};
      }
   } else if (!bitset.test(0)) {
      // It's neither a land nor a water thing.
      throw std::string{"expected \"Landbewohner\" or \"Wasserbewohner\" in attributes"};
   }
   */
   return CreatureAttrs{bitset};
};

namespace {
auto extractors = std::make_tuple(getName, getInt, getInt, getInt, getAttrs, getPath);
}

std::vector<CreatureType> loadCreatureTypes(std::istream&& iS,
                                            std::vector<std::string>& errors) {
   auto tuples = loadResources<CreatureType::Tuple>(std::move(iS), extractors, errors);
   return *reinterpret_cast<std::vector<CreatureType>*>(&tuples);  // Don't judge me!
}

// vim: tw=90 sts=-1 sw=3 et
