#ifndef CREATURE_HPP_XZNFGDOY
#define CREATURE_HPP_XZNFGDOY

#include <cassert>  // assert
#include <cstdint>  // std::uint8_t, std::int16_t, std::uint16_t
#include <string>   // std::string
#include <vector>   // std::vector

#include "creature_type.hpp"
#include "tile_type.hpp"
#include "tuple_helpers.hpp"  // toUT

class World;

extern const std::uint16_t defaultAiState;

struct Creature {
   static void loadTypes(std::string filePath);
   inline static const std::vector<CreatureType>& getTypes();

   Creature(std::uint8_t typeIndex, int currentStep);
   Creature(std::uint8_t typeIndex, int currentStep, std::int16_t lifetime);

   inline std::uint8_t getTypeIndex() const;
   inline const CreatureType& getType() const;

   inline const std::string& getName() const;
   inline int getStrength() const;
   inline int getSpeed() const;
   inline int getMaxLifetime() const;
   inline std::string getAttributeString() const;
   inline const std::string& getBitmapName() const;

   inline bool isAquatic() const;
   inline bool isTerrestrial() const;
   inline bool isPlant() const;
   inline bool isAnimal() const;
   inline bool isHerbivore() const;
   inline bool isCarnivore() const;

   inline float getRelativeLifetime() const;

   // TODO: use a random constant and create offspring when the current step number modulo
   // the creature's procreation interval is equal to it.  The mechanics should be the
   // same and a uint8_t big enough.
   int timeOfLastProcreation;

   std::uint16_t aiState = defaultAiState;
   std::int16_t lifetime;
   const std::uint8_t typeIndex;

  private:
   static std::vector<CreatureType> creatureTypes;
};

const std::vector<CreatureType>& Creature::getTypes() { return creatureTypes; }

std::uint8_t Creature::getTypeIndex() const { return typeIndex; }
const CreatureType& Creature::getType() const { return creatureTypes[getTypeIndex()]; }

const std::string& Creature::getName() const { return getType().getName(); }
int Creature::getStrength() const { return getType().getStrength(); }
int Creature::getSpeed() const { return getType().getSpeed(); }
int Creature::getMaxLifetime() const { return getType().getMaxLifetime(); }
std::string Creature::getAttributeString() const {
   return getType().getAttributeString();
}
const std::string& Creature::getBitmapName() const { return getType().getBitmapName(); }

bool Creature::isAquatic() const { return getType().isAquatic(); }
bool Creature::isTerrestrial() const { return getType().isTerrestrial(); }
bool Creature::isPlant() const { return getType().isPlant(); }
bool Creature::isAnimal() const { return getType().isAnimal(); }
bool Creature::isHerbivore() const { return getType().isHerbivore(); }
bool Creature::isCarnivore() const {
   // Assert it's an animal; plants aren't partitioned into herbivores and carnivores.
   assert(isAnimal());
   return getType().isCarnivore();
}

float Creature::getRelativeLifetime() const {
   return static_cast<float>(lifetime) / getMaxLifetime();
}

#endif  // CREATURE_HPP_XZNFGDOY

// vim: tw=90 sts=-1 sw=3 et
