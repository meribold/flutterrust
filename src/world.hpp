#ifndef WORLD_HPP_L42R9DKX
#define WORLD_HPP_L42R9DKX

#include <array>
#include <cstddef>  // size_t
#include <cstdint>  // int64_t
#include <limits>   // numeric_limits
#include <string>
#include <unordered_map>  // unordered_multimap
#include <vector>

#include "creature.hpp"
#include "creature_type.hpp"
#include "map_generator.hpp"
#include "tile_type.hpp"

class World {
  public:
   World(std::string filePath);

   std::vector<CreatureType> creatureTypes;

   void step();

   // The origin is the index pair (i, j) of the top-left terrain block that is cached.
   // void setOrigin(std::int64_t i, std::int64_t j);
   // std::array<std::int64_t, 2> getOrigin();

   bool isCached(std::int64_t x, std::int64_t y);

   void assertCached(std::int64_t left, std::int64_t top, std::int64_t width,
                     std::int64_t height);

   // Coordinates are relative to the top-left cached terrain block.  No bounds-checking
   // is performed.  To access coordinates outside of the cached terrain, setOrigin has to
   // be called first.
   TileType getTileType(std::int64_t x, std::int64_t y) const;

   // TODO getCreatureIterator()();
   // TODO getCreatures()(int row, int col);

   void testHash();

  private:
   using Pos = std::int64_t[2];

   struct PosHash {
      std::size_t operator()(Pos const& pos) const;
   };

   MapGenerator mapGen;
   static constexpr std::int64_t terrainBlockSize = MapGenerator::blockSize;
   using TerrainBlock = MapGenerator::TerrainBlock;
   TerrainBlock terrainBlock;
   // Top-left, top-right, bottom-left, bottom-right.
   std::array<TerrainBlock, 4> terrainBlocks;
   // For coordinates (x, y) where x is in the range [left, left + 2 * terrainBlockSize)
   // and y is in the range [top, top + 2 * terrainBlockSize), the terrain is cached.
   // When the tile type for coordinates outside one of these ranges is queried, the
   // MapGenerator will be invoked to update the cache.
   std::int64_t top = std::numeric_limits<std::int64_t>::lowest();
   std::int64_t left = std::numeric_limits<std::int64_t>::lowest();

   std::unordered_multimap<World::Pos, Creature, World::PosHash> creatures;
};

#endif  // WORLD_HPP_L42R9DKX

// vim: tw=90 sts=-1 sw=3 et
