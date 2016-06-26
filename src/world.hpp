#ifndef WORLD_HPP_L42R9DKX
#define WORLD_HPP_L42R9DKX

#include <string>
#include <vector>

#include "creature_type.hpp"

class World {
  public:
   World(std::string filePath);

   std::vector<CreatureType> creatureTypes;

  private:
   // ...
};

#endif  // WORLD_HPP_L42R9DKX

// vim: tw=90 sts=-1 sw=3 et
