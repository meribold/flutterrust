#ifndef WORLD_HPP_L42R9DKX
#define WORLD_HPP_L42R9DKX

#include <array>          // array
#include <cstddef>        // size_t
#include <cstdint>        // int64_t, uint8_t
#include <limits>         // numeric_limits
#include <unordered_map>  // unordered_multimap
#include <utility>        // std::pair
#include <vector>         // vector

// #include <experimental/optional>

#include "creature.hpp"
#include "creature_type.hpp"
#include "map_generator.hpp"
#include "tile_type.hpp"

// namespace ex9l = std::experimental;

class World {
  public:
   using Pos = std::array<std::int64_t, 2>;
   using CreatureInfo = std::pair<const Pos, Creature>;

   struct PosHash {
      std::size_t operator()(const Pos& pos) const;
   };

   std::unordered_multimap<Pos, Creature, PosHash> creatures;

   using CreatureIt = decltype(creatures)::iterator;

   World();

   void step();
   void commitStep();
   void updatePlant(CreatureInfo&);
   void updateAnimal(CreatureIt);

   // The origin is the index pair (i, j) of the top-left terrain block that is cached.
   // void setOrigin(std::int64_t i, std::int64_t j);
   // std::array<std::int64_t, 2> getOrigin();

   bool isCached(std::int64_t x, std::int64_t y) const;
   bool isCached(const Pos&) const;

   void assertCached(std::int64_t left, std::int64_t top, std::int64_t width,
                     std::int64_t height);

   // Coordinates are relative to the top-left cached terrain block.  No bounds-checking
   // is performed.  To access coordinates outside of the cached terrain, assertCached has
   // to be called first.
   TileType getTileType(std::int64_t x, std::int64_t y) const;
   inline TileType getTileType(Pos) const;
   inline bool isWater(std::int64_t x, std::int64_t y) const;
   inline bool isWater(Pos) const;
   inline bool isLand(std::int64_t x, std::int64_t y) const;
   inline bool isLand(Pos) const;

   bool isVegetated(Pos) const;

   // Can a creature of the given type survive at the given position?  I.e., does the tile
   // type match the creatures natural environment (auqatic or terrestrial)?
   bool isGoodPosition(const CreatureType&, Pos) const;
   bool isGoodPosition(const CreatureType&, std::int64_t x, std::int64_t y) const;

   int countCreatures(const Pos&, int radius, std::uint8_t creatureTypeIndex) const;

   // void spawnCreature(CreatureInfo&);
   void spawnCreature(std::uint8_t creatureType, std::int64_t x, std::int64_t y);
   // TODO:
   // void spawnPlant(CreatureInfo& parent);
   // void spawnAnimal(CreatureInfo& parent);
   bool spawnOffspring(CreatureInfo& parentInfo);

   // ex9l::optional<CreatureInfo> getOffspring(CreatureInfo& parentInfo);
   // TODO:
   // CreatureInfo* spawnOffspring(CreatureInfo& parentInfo);
   // CreatureInfo* spawnOffspring(CreatureInfo& parentInfo);

   void age(CreatureInfo&);
   void age(Creature&, int lifetime);
   void leech(Creature& actor, Creature& target, int lifetime);
   void procreate(Creature&);

   int getMovementCost(const Pos&, bool onLand) const;

   // Get a random position the animal should move to.
   std::uint16_t generateRoamState(const CreatureInfo&) const;

   void roam(CreatureIt animalIt);

   // Get the shortest path from `start` to `dest` using the A* algorithm.
   std::vector<Pos> getPath(Pos start, Pos dest) const;

   // Get all positions that are reachable without moving a distance greater than
   // `maxDist`.  I.e., positions that are within a distance of `maxDist` but require
   // moving along a longer path (e.g. because something blocks a more direct one) are
   // excluded.  Uses breadth-first search.
   std::vector<Pos> getReachablePositions(const Pos& start, int maxDist) const;

   Pos moveTowards(CreatureIt animalIt, const Pos& dest);

  private:
   MapGenerator mapGen;
   static constexpr std::int64_t terrainBlockSize = MapGenerator::blockSize;
   using TerrainBlock = MapGenerator::TerrainBlock;
   TerrainBlock terrainBlock;
   // Top-left, top-right, bottom-left, bottom-right.
   std::array<TerrainBlock, 4> terrainBlocks;
   // For coordinates (x, y) where x is in the range [left, left + 2 * terrainBlockSize)
   // and y is in the range [top, top + 2 * terrainBlockSize), the terrain is cached.
   // When the tile type for coordinates outside one of these ranges is queried, the
   // ~~MapGenerator will be invoked to update the cache~~ program will crash.
   std::int64_t top = std::numeric_limits<std::int64_t>::lowest();
   std::int64_t left = std::numeric_limits<std::int64_t>::lowest();

   // Used to cache all the offspring spawned in one step before it is inserted into the
   // hash map.  Directly inserting new creatures into the hash map can invalidate
   // iterators.  It also would probably depend on the insertee's position whether the
   // current step's loop over the hash map will have its body executed for the new
   // creature or not.
   std::vector<World::CreatureInfo> offspringCache;

   // Movee: one who is being moved, obviously.
   std::vector<std::pair<const Pos, CreatureIt>> moveeCache;

   int currentStep = 0;
};

// Manhattan metric.
std::int64_t distance(const World::Pos&, const World::Pos&);

TileType World::getTileType(World::Pos pos) const { return getTileType(pos[0], pos[1]); }

bool World::isWater(std::int64_t x, std::int64_t y) const {
   return toUT(getTileType(x, y)) <= 1;
}

bool World::isWater(World::Pos pos) const { return isWater(pos[0], pos[1]); }

bool World::isLand(std::int64_t x, std::int64_t y) const {
   return toUT(getTileType(x, y)) >= 2;
}

bool World::isLand(World::Pos pos) const { return isLand(pos[0], pos[1]); }

#endif  // WORLD_HPP_L42R9DKX

// vim: tw=90 sts=-1 sw=3 et
