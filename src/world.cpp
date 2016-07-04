#include <cassert>    // assert
#include <climits>    // CHAR_BIT
#include <cmath>      // pow
#include <cstdint>    // int64_t
#include <fstream>    // ifstream
#include <stdexcept>  // runtime_error
#include <vector>

#include "creature_parser.hpp"
#include "world.hpp"

World::World(std::string filePath) : creatureTypes{} {
   std::ifstream iStream{filePath};

   if (!iStream.is_open()) {
      throw std::runtime_error{u8"couldn't open file " + filePath};
   }

   iStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

   std::vector<std::string> errors;
   creatureTypes = loadCreatureTypes(std::move(iStream), errors);
}

void World::step() {}

// Map a signed integer number z to the interval [0, 2^n - 1].  Injective for the domain
// [- 2^(n-1), 2^(n-1) - 1].
template <std::size_t n, typename Z>
inline std::size_t toNBits(Z z) {
   // Signed, because using an unsigned type in operations with signed ones can cause the
   // signed operand to be converted to an unsigned type ("usual arithmetic conversions").
   constexpr Z addend = std::pow(2, n - 1);
   constexpr Z divisor = std::pow(2, n);
   // C++ defines the modulo operation's result to have the same sign as the divident.
   // E.g., `(-128) % 129 == -128`.
   // return (((z + addend) % divisor) + divisor) % divisor;
   return ((z % divisor) + 3 * addend) % divisor;
}

std::size_t World::PosHash::operator()(Pos const& pos) const {
   // Odds are we only get ints with very small absolute values.  Construct a hash by
   // taking the moduli of both coordinates and the biggest number that fits in half the
   // bits of size_t.  Use those moduli chained together as the hash.
   // There will be no collisions for positions that aren't super far from the origin.
   constexpr std::size_t numBits = sizeof(std::size_t) * CHAR_BIT;
   // constexpr std::size_t numBits = 16;
   // Assert that size_t has an even number of bits.
   static_assert(numBits / 2 * 2 == numBits, "Seriously?");
   constexpr std::size_t halfNumBits = numBits / 2;
   // static_assert(sizeof(int) == sizeof(int64_t), "Oh.  TIL.");
   std::size_t lowBits = toNBits<halfNumBits, int64_t>(pos[0]);
   std::size_t highBits = toNBits<halfNumBits, int64_t>(pos[1]);
   // If size_t had 16 bits, lowBits and highBits would be in the range [0, 255] now (so
   // they only use 8 bits).
   assert((lowBits >> halfNumBits) == 0);
   assert((highBits >> halfNumBits) == 0);
   highBits <<= halfNumBits;
   assert((lowBits & highBits) == 0);
   return (lowBits | highBits);
}

// vim: tw=90 sts=-1 sw=3 et
