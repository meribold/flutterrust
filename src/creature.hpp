#ifndef CREATURE_HPP_XZNFGDOY
#define CREATURE_HPP_XZNFGDOY

#include <cassert>  // assert
#include <string>
#include <vector>

#include "creature_type.hpp"

class World;

class Creature {
  public:
   static void loadTypes(std::string filePath);
   inline static const std::vector<CreatureType>& getTypes();

   inline Creature(std::size_t typeIndex, int lifetime);

   inline std::size_t getTypeIndex() const;
   inline const CreatureType& getType() const;

   inline const std::string& getName() const;
   inline int getStrength() const;
   inline int getSpeed() const;
   inline int getLifetime() const;
   inline std::string getAttributeString() const;
   inline const std::string& getBitmapName() const;

   inline bool isAquatic() const;
   inline bool isTerrestrial() const;
   inline bool isPlant() const;
   inline bool isAnimal() const;
   inline bool isHerbivore() const;
   inline bool isCarnivore() const;

  private:
   static std::vector<CreatureType> creatureTypes;

   const std::size_t typeIndex;
   int lifetime;
};

const std::vector<CreatureType>& Creature::getTypes() { return creatureTypes; }

Creature::Creature(std::size_t typeIndex, int lifetime)
    : typeIndex{typeIndex}, lifetime{lifetime} {}

std::size_t Creature::getTypeIndex() const { return typeIndex; }
const CreatureType& Creature::getType() const { return creatureTypes[getTypeIndex()]; }

const std::string& Creature::getName() const { return getType().getName(); }
int Creature::getStrength() const { return getType().getStrength(); }
int Creature::getSpeed() const { return getType().getSpeed(); }
int Creature::getLifetime() const { return getType().getLifetime(); }
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

#endif  // CREATURE_HPP_XZNFGDOY

// vim: tw=90 sts=-1 sw=3 et
