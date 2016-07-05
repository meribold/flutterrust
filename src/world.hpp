#ifndef WORLD_HPP_L42R9DKX
#define WORLD_HPP_L42R9DKX

#include <array>
#include <cstdint>  // int64_t
#include <string>
#include <unordered_map>  // unordered_multimap
#include <vector>
// #include <cstddef>  // size_t
// #include <climits>  // INT_MIN, INT_MAX

#include "creature.hpp"
#include "creature_type.hpp"
#include "tile.hpp"

enum class TileType : uint8_t { deepWater = 0, water, sand, dirt, rock, snow };

class World {
  public:
   World(std::string filePath);

   std::vector<CreatureType> creatureTypes;

   void step();

   TileType getTileType(int row, int col) const;

   // TODO getCreatureIterator()();
   // TODO getCreatures()(int row, int col);

   void testHash();

  private:
   using Pos = int64_t[2];

   struct PosHash {
      std::size_t operator()(Pos const& pos) const;
   };

   std::unordered_multimap<World::Pos, Creature, World::PosHash> creatures;
   // static constexpr std::size_t numRows = 64;
   // static constexpr std::size_t numCols = 64;
   // std::array<std::array<Tile, numRows>, numCols> tiles;
};

#endif  // WORLD_HPP_L42R9DKX

// vim: tw=90 sts=-1 sw=3 et
