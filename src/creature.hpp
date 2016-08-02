#ifndef CREATURE_HPP_XZNFGDOY
#define CREATURE_HPP_XZNFGDOY

#include <string>
#include <vector>

#include "creature_type.hpp"

class World;

class Creature {
  public:
   static void loadTypes(std::string filePath);
   static const std::vector<CreatureType>& getTypes();

   Creature(std::size_t typeIndex, int lifetime);

   std::size_t getTypeIndex() const;
   const CreatureType& getCreatureType() const;
   const CreatureAttrs& getCreatureAttrs() const;

   bool isAquatic() const;
   bool isTerrestrial() const;
   bool isPlant() const;
   bool isAnimal() const;
   bool isHerbivore() const;
   bool isCarnivore() const;

  private:
   static std::vector<CreatureType> creatureTypes;

   const std::size_t typeIndex;
   int lifetime;
};

#endif  // CREATURE_HPP_XZNFGDOY

// vim: tw=90 sts=-1 sw=3 et
