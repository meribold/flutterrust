#ifndef CREATURE_HPP_XZNFGDOY
#define CREATURE_HPP_XZNFGDOY

#include "creature_type.hpp"

class World;

class Creature {
  public:
   Creature(std::size_t typeIndex);

   std::size_t getTypeIndex() const;

  private:
   const std::size_t typeIndex;
   // friend class World;
};

#endif  // CREATURE_HPP_XZNFGDOY

// vim: tw=90 sts=-1 sw=3 et
