#ifndef CREATURE_TYPE_HPP_21UKGANC
#define CREATURE_TYPE_HPP_21UKGANC

#include <bitset>
#include <ostream>
#include <string>
#include <tuple>

struct CreatureAttrs {
   bool isAquatic() const;
   bool isTerrestrial() const;
   bool isAnimal() const;
   bool isPlant() const;
   bool isHerbivore() const;
   bool isCarnivore() const;

   operator std::string() const;
   friend std::ostream& operator<<(std::ostream&, const CreatureAttrs&);

   std::bitset<3> bitset;
};

// class CreatureType {};

using CreatureType = std::tuple<std::string, int, int, int, CreatureAttrs, std::string>;

// Names for the tuple elements to replace obscure code like `std::get<4>(creatureType)`
// with `std::get<cTFields::attributes>(creatureType).
namespace cTFields {
enum { name, strength, speed, lifetime, attributes, bitmap };
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
class CreatureType
{
   public:

   CreatureType() = delete;
   CreatureType()

   private:

   using PropertyTuple = std::tuple<std::string, int, int, int, std::string, std::string>;
   PropertyTuple propertyTuple;
};
*/

#endif  // CREATURE_TYPE_HPP_21UKGANC

// vim: tw=90 sts=-1 sw=3 et
