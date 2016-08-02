#include "creature_type.hpp"

CreatureAttrs::operator std::string() const {
   static const std::string aquatic = u8"Wasserbewohner";
   static const std::string terrestrial = u8"Landbewohner";
   static const std::string plant = u8"Pflanze";
   static const std::string animal = u8"Tier";
   static const std::string herbivore = u8"Pflanzenfresser";
   static const std::string carnivore = u8"Fleischfresser";

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
