#include "creature.hpp"

#include <fstream>    // ifstream
#include <random>     // std::default_random_engine, std::random_device, ...
#include <stdexcept>  // runtime_error
#include <vector>     // vector

#include "creature_parser.hpp"

void Creature::loadTypes(std::string filePath) {
   std::ifstream iStream{filePath};

   if (!iStream.is_open()) {
      throw std::runtime_error{u8"couldn't open file " + filePath};
   }

   iStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

   std::vector<std::string> errors;
   creatureTypes = loadCreatureTypes(std::move(iStream), errors);
}

namespace {
std::default_random_engine rNG(std::random_device{}());
// Distribution ranging from 0 to the highest representable value.
std::uniform_int_distribution<int> defaultRNDist{};
}

Creature::Creature(std::uint8_t typeIndex)
    : Creature{typeIndex, creatureTypes[typeIndex].getMaxLifetime()} {}

Creature::Creature(std::uint8_t typeIndex, std::int16_t lifetime)
    : lifetime{lifetime},
      typeIndex{typeIndex},
      // Offset the first time the creature can procreate by a random value.  Otherwise,
      // all creatures of the same type always produce offspring in the same step.
      procreationOffset{static_cast<std::uint8_t>(
          isPlant() ? defaultRNDist(rNG) % getProcreationInterval()
                    : getProcreationInterval() - 1)} {}

std::vector<CreatureType> Creature::creatureTypes;

// vim: tw=90 sts=-1 sw=3 et
