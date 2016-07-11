#include <cassert>    // assert
#include <climits>    // CHAR_BIT
#include <cmath>      // pow, lround
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

void World::step() {
   // ...
}

bool World::isCached(std::int64_t x, std::int64_t y) {
   return (left <= x && x < left + 2 * terrainBlockSize && top <= y &&
           y < top + 2 * terrainBlockSize);
}

void World::assertCached(std::int64_t left, std::int64_t top, std::int64_t width,
                         std::int64_t height) {
   std::int64_t right = left + width;
   std::int64_t bottom = top + height;
   if (isCached(left, top) && isCached(right, bottom)) {
      return;
   }
   // Figure out the indices of the top-left block we need.
   auto centerX = left + width / 2;
   auto centerY = top + height / 2;
   // Round fairly.
   std::int64_t i = std::lround(static_cast<float>(centerY) / terrainBlockSize);
   std::int64_t j = std::lround(static_cast<float>(centerX) / terrainBlockSize);
   // TODO: reuse blocks when possible.
   terrainBlocks[0] = mapGen.getBlock(i - 1, j - 1);
   terrainBlocks[1] = mapGen.getBlock(i - 1, j);
   terrainBlocks[2] = mapGen.getBlock(i, j - 1);
   terrainBlocks[3] = mapGen.getBlock(i, j);
   this->top = (i - 1) * terrainBlockSize;
   this->left = (j - 1) * terrainBlockSize;
}

// Increasing x means going right, increasing y means going down.
TileType World::getTileType(std::int64_t x, std::int64_t y) const {
#ifndef NDEBUG
   std::int64_t right = left + 2 * terrainBlockSize;
   std::int64_t bottom = top + 2 * terrainBlockSize;
   assert(left <= x && x < right);
   assert(top <= y && y < bottom);
#endif
   auto i = y - top;
   auto j = x - left;
   auto blockIndex = 2 * (i / terrainBlockSize) + j / terrainBlockSize;
   i %= terrainBlockSize;
   j %= terrainBlockSize;
   assert(0 <= i && i < terrainBlockSize);
   assert(0 <= j && j < terrainBlockSize);
   return terrainBlocks[blockIndex][i][j];
   /*
   auto blockIndex = 0;
   auto i = y - top;
   auto j = x - left;
   if (i >= terrainBlockSize) {
      // Use one of the bottom blocks.
      blockIndex += 2;
      i -= terrainBlockSize;
   }
   if (j >= terrainBlockSize) {
      // Use one of the right blocks.
      blockIndex += 1;
      j -= terrainBlockSize;
   }
   return terrainBlocks[blockIndex][i][j];
   */
}

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
   // Odds are we only get ints with very small absolute values.  Convert both coordinates
   // to values that fit into half the bits of size_t and chain those together.  There are
   // no collisions when both coordinates are in [- 2^(n-1), 2^(n-1) - 1].
   constexpr std::size_t numBits = sizeof(std::size_t) * CHAR_BIT;
   // Assert that size_t has an even number of bits.
   static_assert(numBits / 2 * 2 == numBits, "Seriously?");
   constexpr std::size_t halfNumBits = numBits / 2;
   // static_assert(sizeof(int) == sizeof(std::int64_t), "Oh.  TIL.");
   std::size_t lowBits = toNBits<halfNumBits, std::int64_t>(pos[0]);
   std::size_t highBits = toNBits<halfNumBits, std::int64_t>(pos[1]);
   // If size_t had 16 bits, lowBits and highBits would be in the range [0, 255] now (so
   // they only use 8 bits).
   assert((lowBits >> halfNumBits) == 0);
   assert((highBits >> halfNumBits) == 0);
   highBits <<= halfNumBits;
   assert((lowBits & highBits) == 0);
   return (lowBits | highBits);
}

void World::testHash() {
   /*
   World::PosHash hash{};
   std::cerr << "-386: " << hash({-386, -128}) << '\n';
   std::cerr << "-385: " << hash({-385, -128}) << '\n';
   std::cerr << "-384: " << hash({-384, -128}) << '\n';
   std::cerr << "-383: " << hash({-383, -128}) << '\n';
   std::cerr << "-382: " << hash({-382, -128}) << '\n';
   std::cerr << "-130: " << hash({-130, -128}) << '\n';
   std::cerr << "-129: " << hash({-129, -128}) << '\n';
   std::cerr << "-128: " << hash({-128, -128}) << '\n';
   std::cerr << "-127: " << hash({-127, -128}) << '\n';
   std::cerr << "-126: " << hash({-126, -128}) << '\n';
   std::cerr << "-1: " << hash({-1, -128}) << '\n';
   std::cerr << "0: " << hash({0, -128}) << '\n';
   std::cerr << "1: " << hash({1, -128}) << '\n';
   std::cerr << "126: " << hash({126, -128}) << '\n';
   std::cerr << "127: " << hash({127, -128}) << '\n';
   std::cerr << "128: " << hash({128, -128}) << '\n';
   std::cerr << "129: " << hash({129, -128}) << '\n';
   std::cerr << "130: " << hash({130, -128}) << '\n';
   std::cerr << "382: " << hash({382, -128}) << '\n';
   std::cerr << "383: " << hash({383, -128}) << '\n';
   std::cerr << "384: " << hash({384, -128}) << '\n';
   std::cerr << "385: " << hash({385, -128}) << '\n';
   std::cerr << "386: " << hash({386, -128}) << '\n';
   */
}

// vim: tw=90 sts=-1 sw=3 et
