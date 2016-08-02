#include "creature.hpp"

#include <fstream>    // ifstream
#include <stdexcept>  // runtime_error
#include <vector>

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

const std::vector<CreatureType>& Creature::getTypes() {
   return creatureTypes;
}

Creature::Creature(std::size_t typeIndex, int lifetime)
    : typeIndex{typeIndex}, lifetime{lifetime} {}

std::size_t Creature::getTypeIndex() const { return typeIndex; }

const CreatureType& Creature::getCreatureType() const {
   return creatureTypes[getTypeIndex()];
}

const CreatureAttrs& Creature::getCreatureAttrs() const {
   return std::get<cTFields::attributes>(getCreatureType());
}

bool Creature::isAquatic() const {
   return getCreatureAttrs().isAquatic();
}

bool Creature::isTerrestrial() const {
   return getCreatureAttrs().isTerrestrial();
}

bool Creature::isPlant() const {
   return getCreatureAttrs().isPlant();
}

bool Creature::isAnimal() const {
   return getCreatureAttrs().isAnimal();
}

bool Creature::isHerbivore() const {
   return getCreatureAttrs().isHerbivore();
}

bool Creature::isCarnivore() const {
   // Assert it's an animal; plants aren't partitioned into herbivores and carnivores.
   assert(isAnimal());
   return getCreatureAttrs().isCarnivore();
}

std::vector<CreatureType> Creature::creatureTypes;

// vim: tw=90 sts=-1 sw=3 et
