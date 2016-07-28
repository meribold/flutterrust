#include "creature.hpp"

Creature::Creature(std::size_t typeIndex) : typeIndex{typeIndex} {}

std::size_t Creature::getTypeIndex() const {
   return typeIndex;
}

// vim: tw=90 sts=-1 sw=3 et
