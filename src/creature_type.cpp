#include "creature_type.hpp"

namespace {
const std::string aquatic = u8"Wasserbewohner";
const std::string terrestrial = u8"Landbewohner";
const std::string plant = u8"Pflanze";
const std::string animal = u8"Tier";
const std::string herbivore = u8"Pflanzenfresser";
const std::string carnivore = u8"Fleischfresser";
}

bool CreatureAttrs::isAquatic() const { return !bitset.test(0); }

bool CreatureAttrs::isTerrestrial() const { return bitset.test(0); }

bool CreatureAttrs::isPlant() const { return !bitset.test(1); }

bool CreatureAttrs::isAnimal() const { return bitset.test(1); }

bool CreatureAttrs::isHerbivore() const { return !bitset.test(2); }

bool CreatureAttrs::isCarnivore() const { return bitset.test(2); }

CreatureAttrs::operator std::string() const {
   std::string s;
   s += isTerrestrial() ? terrestrial : aquatic;
   s += ' ';
   if (isAnimal()) {
      s += animal;
      s += ' ' + (isCarnivore() ? carnivore : herbivore);
   } else {
      s += plant;
   }
   return s;
}

std::ostream& operator<<(std::ostream& oS, const CreatureAttrs& creatureAttrs) {
   oS << static_cast<std::string>(creatureAttrs);
   return oS;
}

// vim: tw=90 sts=-1 sw=3 et
