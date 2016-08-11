#ifndef CREATURE_TYPE_HPP_21UKGANC
#define CREATURE_TYPE_HPP_21UKGANC

#include <bitset>
#include <cstdint>  // int16_t
#include <ostream>
#include <string>
#include <tuple>

struct CreatureAttrs {
   inline bool isAquatic() const;
   inline bool isTerrestrial() const;
   inline bool isAnimal() const;
   inline bool isPlant() const;
   inline bool isHerbivore() const;
   inline bool isCarnivore() const;

   operator std::string() const;
   friend std::ostream& operator<<(std::ostream&, const CreatureAttrs&);

   std::bitset<3> bitset;
};

struct CreatureType {
   inline const std::string& getName() const;
   inline int getStrength() const;
   inline int getSpeed() const;
   inline int16_t getMaxLifetime() const;
   inline std::string getAttributeString() const;
   inline const std::string& getBitmapName() const;

   inline bool isAquatic() const;
   inline bool isTerrestrial() const;
   inline bool isPlant() const;
   inline bool isAnimal() const;
   inline bool isHerbivore() const;
   inline bool isCarnivore() const;

   using Tuple = std::tuple<std::string, int, int, int16_t, CreatureAttrs, std::string>;
   Tuple tuple;

  private:
   inline CreatureAttrs getAttributes() const;
};

// Names for the tuple elements to replace obscure code like `std::get<4>(creatureType)`
// with `std::get<cTFields::attributes>(creatureType).
namespace {
namespace cTFields {
enum { name, strength, speed, lifetime, attributes, bitmap };
}
}

/*
enum class CTFields { name, strength, speed, lifetime, attributes, bitmap };
*/

/*
constexpr struct {
   std::size_t name = 0, strength = 1, speed = 2, lifetime = 3, attributes = 4,
               bitmap = 5;
} cTFields;
*/

/*
class CreatureType {
  public:
   CreatureType() = delete;
   CreatureType();

  private:
   using PropertyTuple = std::tuple<std::string, int, int, int, std::string, std::string>;
   PropertyTuple propertyTuple;
};
*/

bool CreatureAttrs::isAquatic() const { return !bitset.test(0); }
bool CreatureAttrs::isTerrestrial() const { return bitset.test(0); }
bool CreatureAttrs::isPlant() const { return !bitset.test(1); }
bool CreatureAttrs::isAnimal() const { return bitset.test(1); }
bool CreatureAttrs::isHerbivore() const { return isAnimal() && !bitset.test(2); }
bool CreatureAttrs::isCarnivore() const { return bitset.test(2); }

const std::string& CreatureType::getName() const {
   return std::get<cTFields::name>(tuple);
}

int CreatureType::getStrength() const { return std::get<cTFields::strength>(tuple); }
int CreatureType::getSpeed() const { return std::get<cTFields::speed>(tuple); }
int16_t CreatureType::getMaxLifetime() const {
   return std::get<cTFields::lifetime>(tuple);
}

CreatureAttrs CreatureType::getAttributes() const {
   return std::get<cTFields::attributes>(tuple);
}

std::string CreatureType::getAttributeString() const {
   return std::string{getAttributes()};
}

const std::string& CreatureType::getBitmapName() const {
   return std::get<cTFields::bitmap>(tuple);
}

bool CreatureType::isAquatic() const { return getAttributes().isAquatic(); }
bool CreatureType::isTerrestrial() const { return getAttributes().isTerrestrial(); }
bool CreatureType::isPlant() const { return getAttributes().isPlant(); }
bool CreatureType::isAnimal() const { return getAttributes().isAnimal(); }
bool CreatureType::isHerbivore() const { return getAttributes().isHerbivore(); }
bool CreatureType::isCarnivore() const { return getAttributes().isCarnivore(); }

#endif  // CREATURE_TYPE_HPP_21UKGANC

// vim: tw=90 sts=-1 sw=3 et
