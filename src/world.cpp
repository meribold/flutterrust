#include <cassert>  // assert
#include <climits>  // CHAR_BIT
#include <cmath>    // pow, lround
#include <cstdint>  // int64_t
#include <cstdlib>  // abs

#include "world.hpp"

World::World() {}

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
   std::int64_t i = std::lround(static_cast<float>(centerY) / terrainBlockSize) - 1;
   std::int64_t j = std::lround(static_cast<float>(centerX) / terrainBlockSize) - 1;

   // Reuse blocks when possible.
   {
      // Mappings of which terrain blocks should be copies of another block instead of
      // being generated.  For example, {3, 0} means block 0 should be copied to block 3
      // instead of invoking the MapGenerator; {0, 0} means the block has to be generated.
      // Indices into this array are based on deltaI's and deltaJ's values.
      constexpr std::size_t reuse[9][4][2]{
          {{3, 0}, {0, 0}, {1, 1}, {2, 2}},  // For scrolling up and left.
          {{2, 0}, {3, 1}, {0, 0}, {1, 1}},  // For scrolling up.
          {{2, 1}, {0, 0}, {1, 1}, {3, 3}},  // For scrolling up and right.
          {{1, 0}, {3, 2}, {0, 0}, {2, 2}},  // For scrolling left.
          {{0, 0}, {1, 1}, {2, 2}, {3, 3}},  // This doesn't happen.
          {{0, 1}, {2, 3}, {1, 1}, {3, 3}},  // For scrolling right.
          {{1, 2}, {0, 0}, {2, 2}, {3, 3}},  // For scrolling down and left.
          {{0, 2}, {1, 3}, {2, 2}, {3, 3}},  // For scrolling down.
          {{0, 3}, {1, 1}, {2, 2}, {3, 3}}   // For scrolling down and right.
      };

      // See how much the indices i and j changed.
      auto oldI = this->top / terrainBlockSize;
      auto oldJ = this->left / terrainBlockSize;
      auto deltaI = i - oldI;
      auto deltaJ = j - oldJ;
      assert(deltaI != 0 || deltaJ != 0);

      std::int64_t direction;
      if (std::abs(deltaI) <= 1 && std::abs(deltaJ) <= 1) {
         direction = 3 * (deltaI + 1) + (deltaJ + 1);  // Index into `reuse`.
         assert(0 <= direction && direction <= 8);
         assert(direction != 4);
      } else {
         // This is unlikely since the map can only be moved by scrolling (there is no way
         // to directly move to a random position).
         direction = 4;
      }
      for (const auto& mapping : reuse[direction]) {
         auto dest = mapping[0];
         auto source = mapping[1];
         if (dest != source) {
            // Copy a block.
            terrainBlocks[dest] = terrainBlocks[source];
         } else {
            // Generate a block.
            constexpr std::int64_t offsets[][2]{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
            auto& offset = offsets[dest];
            terrainBlocks[dest] = mapGen.getBlock(i + offset[0], j + offset[1]);
         }
      }
   }

   this->top = i * terrainBlockSize;
   this->left = j * terrainBlockSize;
}

// Increasing x means going right, increasing y means going down.
TileType World::getTileType(std::int64_t x, std::int64_t y) const {
#ifdef DEBUG  // {{{1
   std::int64_t right = left + 2 * terrainBlockSize;
   std::int64_t bottom = top + 2 * terrainBlockSize;
   assert(left <= x && x < right);
   assert(top <= y && y < bottom);
#endif  // }}}1
   auto i = y - top;
   auto j = x - left;
   auto blockIndex = 2 * (i / terrainBlockSize) + j / terrainBlockSize;
   i %= terrainBlockSize;
   j %= terrainBlockSize;
   assert(0 <= i && i < terrainBlockSize);
   assert(0 <= j && j < terrainBlockSize);
   return terrainBlocks[blockIndex][i][j];
}

bool World::addCreature(std::size_t typeIndex, std::int64_t x, std::int64_t y) {
   // When trying to place a creature on a tile of hostile type (e.g. a fish on land),
   // don't do anything and return false.  FIXME: just assert we don't try to do that, the
   // UI shouldn't allow it anymore.
   const TileType tileType = getTileType(x, y);
   const CreatureType& creatureType = Creature::getTypes()[typeIndex];
   bool aquatic = creatureType.isAquatic();
   if (tileType == TileType::deepWater || tileType == TileType::water) {
      if (!aquatic) return false;
   } else {
      if (aquatic) return false;
   }
   int lifetime = creatureType.getLifetime();
   creatures.emplace(Pos{x, y}, Creature{typeIndex, lifetime});
   return true;
}

decltype(World::creatures)::const_iterator World::getCreatures(std::int64_t x,
                                                               std::int64_t y) {
   return creatures.find({x, y});
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

// vim: tw=90 sts=-1 sw=3 et fdm=marker
