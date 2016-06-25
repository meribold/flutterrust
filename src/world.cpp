#include <fstream>    // ifstream
#include <stdexcept>  // runtime_error
#include <string>
#include <vector>

#include "creature_parser.hpp"
#include "world.hpp"

World::World() : creatureTypes{} {
   std::string filePath = u8"CreatureTable.txt";
   std::ifstream iStream{filePath};
   iStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

   if (!iStream.is_open()) {
      throw std::runtime_error{u8"couldn't open file " + filePath};
   }

   std::vector<std::string> errors;
   creatureTypes = loadCreatureTypes(std::move(iStream), errors);
}

// vim: tw=90 sts=-1 sw=3 et
