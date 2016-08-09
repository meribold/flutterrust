#include <cassert>        // assert
#include <climits>        // CHAR_BIT
#include <cmath>          // pow, lround, abs
#include <cstdint>        // int64_t
#include <cstdlib>        // abs
#include <queue>          // priority_queue
#include <random>         // std::default_random_engine, std::random_device, ...
#include <unordered_map>  // unordered_map
#include <vector>         // vector

#ifdef DEBUG
#include <iostream>
#endif

#include "world.hpp"

World::World() {}

// TODO: return a list of positions the GUI should redraw.
void World::step() {
   ++currentStep;
   for (auto it = creatures.begin(); it != creatures.end();) {
      const World::Pos& pos = it->first;
      if (!isCached(pos)) {
         ++it;
         continue;
      }
      Creature& creature = it->second;
      if (creature.isPlant()) {
         updatePlant(*it);
      } else {
         updateAnimal(*it);
      }
      if (creature.lifetime <= 0) {
         // TODO: if it's an animal, display the carcass graphic for 10 steps.
         it = creatures.erase(it);
      } else {
         ++it;
      }
   }

   for (auto& creature : offspringCache) {
      creatures.insert(creature);
   }
   offspringCache.clear();
}

void World::updatePlant(World::CreatureInfo& plantInfo) {
   const World::Pos& pos = plantInfo.first;
   Creature& plant = plantInfo.second;
   int stepsSinceProcreating = currentStep - plant.timeOfLastProcreation;
   bool shouldGrow = stepsSinceProcreating >= plant.getMaxLifetime() / 100;
   // bool shouldGrow = currentStep % (plant.getMaxLifetime() / 100) == 0;
   if (shouldGrow) {
      // Get the number of plants that have the same type within 5 tiles of the parent.
      int nearbyConspecificPlants = countCreatures(pos, 5, plant.getTypeIndex());
      // std::cerr << nearbyConspecificPlants << '\n';
      if (2 < nearbyConspecificPlants && nearbyConspecificPlants < 10) {
         for (int i = 0; i < 2; ++i) {
            spawnOffspring(plantInfo);
         }
      }
      plant.timeOfLastProcreation = currentStep;
   }
   age(plantInfo);
}

void World::updateAnimal(World::CreatureInfo& animalInfo) {
   // TODO.
}

bool World::isCached(std::int64_t x, std::int64_t y) const {
   return (left <= x && x < left + 2 * terrainBlockSize && top <= y &&
           y < top + 2 * terrainBlockSize);
}

bool World::isCached(const World::Pos& pos) const { return isCached(pos[0], pos[1]); }

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
   assert(isCached(x, y));
   auto i = y - top;
   auto j = x - left;
   auto blockIndex = 2 * (i / terrainBlockSize) + j / terrainBlockSize;
   i %= terrainBlockSize;
   j %= terrainBlockSize;
   assert(0 <= i && i < terrainBlockSize);
   assert(0 <= j && j < terrainBlockSize);
   return terrainBlocks[blockIndex][i][j];
}

bool World::isGoodPosition(const CreatureType& creatureType, World::Pos pos) const {
   assert(isCached(pos));
   const TileType tileType = getTileType(pos);
   if (tileType == TileType::deepWater || tileType == TileType::water) {
      return creatureType.isAquatic();
   } else {
      return creatureType.isTerrestrial();
   }
}

int World::countCreatures(const World::Pos& pos, int radius,
                          std::size_t creatureTypeIndex) const {
   int count = 0;
   for (int xOffset = -radius; xOffset <= radius; ++xOffset) {
      int maxYOffset = radius - std::abs(xOffset);
      for (int yOffset = -maxYOffset; yOffset <= maxYOffset; ++yOffset) {
         assert(std::abs(xOffset) + std::abs(yOffset) <= radius);
         auto range = creatures.equal_range({pos[0] + xOffset, pos[1] + yOffset});
         for (auto it = range.first; it != range.second; ++it) {
            if (it->second.getTypeIndex() == creatureTypeIndex) {
               ++count;
            }
         }
      }
   }
   return count;
}

void World::spawnCreature(std::size_t typeIndex, std::int64_t x, std::int64_t y) {
   // Assert we don't try to place a creature on a hostile tile (e.g. a fish on land).
   assert(isGoodPosition(Creature::getTypes()[typeIndex], {x, y}));
   creatures.emplace(Pos{x, y}, Creature{typeIndex, currentStep});
}

bool World::spawnOffspring(World::CreatureInfo& parentInfo) {
   static std::default_random_engine rNG(std::random_device{}());
   static std::uniform_int_distribution<int> rNDist{-5, 5};
   static std::uniform_int_distribution<int> coin(0, 1);

   const World::Pos& pos = parentInfo.first;
   Creature& parent = parentInfo.second;
   const CreatureType& creatureType = parent.getType();
   if (parent.isPlant()) {
      // Randomly pick a position and create offspring if the position's type matches the
      // plants natural environment (land or water).  TODO: create a list of eligible
      // positions first and randomly pick one instead?
      int xOffset = rNDist(rNG);
      int yOffset = 5 - std::abs(xOffset);
      if (coin(rNG)) {
         yOffset = -yOffset;
      }
      World::Pos childPos{pos[0] + xOffset, pos[1] + yOffset};
      assert(getDistance(pos, childPos) == 5);
      if (!isCached(childPos)) {
         return false;
      }
      if (!isGoodPosition(creatureType, childPos)) {
         return false;
      }
      auto range = creatures.equal_range(childPos);
      for (auto it = range.first; it != range.second; ++it) {
         if (it->second.isPlant()) {
            return false;  // The tile is already covered by vegetation.
         }
      }
      offspringCache.push_back(
          CreatureInfo{childPos, Creature{parent.getTypeIndex(), currentStep}});
      return true;
   } else {
      return false;  // TODO.
   }
}

void World::age(CreatureInfo& creatureInfo) {
   Creature& creature = creatureInfo.second;
   if (creature.isPlant()) {
      const World::Pos& pos = creatureInfo.first;
      const TileType tileType = getTileType(pos);
      if (tileType == TileType::water || tileType == TileType::sand ||
          tileType == TileType::dirt) {
         creature.lifetime -= 10;
      } else {
         creature.lifetime -= 25;
      }
   } else {
      // TODO.
   }
}

int World::getMovementCost(const World::Pos& pos, bool terrestrial) const {
   // Movement costs for aquatic and terrestrial animals.
   static constexpr std::array<int, toUT(TileType::SIZE)> movementCosts[2]{
       {3, 1, -1, -1, -1, -1}, {-1, -1, 1, 1, 4, 2}};
   return movementCosts[terrestrial][toUT(getTileType(pos))];
}

// Manhattan distance from a to b.
std::int64_t World::getDistance(const World::Pos& a, const World::Pos& b) const {
   return std::abs(b[0] - a[0]) + std::abs(b[1] - a[1]);
}

// Compute the shortest path from `start` to `dest` using the A* algorithm.  Based on
// [this introduction][1].  TODO: based on the demos, I think the linked page uses the
// estimated distance to the destination as a tiebreaker when multiple positions have the
// same priority; mayby implement that optimization.
// [1]: http://redblobgames.com/pathfinding/a-star/introduction.html
std::vector<World::Pos> World::getPath(World::Pos start, World::Pos dest) const {
   using P3 = std::pair<int, World::Pos>;  // Priority-position pair.
   struct P3Compare {
      constexpr bool operator()(const P3& lhs, const P3& rhs) const {
         return lhs.first > rhs.first;
      }
   };
   // The element with the smallest priority is returned by frontier.top() and removed
   // with frontier.pop().
   std::priority_queue<P3, std::vector<P3>, P3Compare> frontier;
   frontier.emplace(0, start);

   // Extra information saved for positions we visited: the previous position based on the
   // best known path and the resulting total cost.
   struct PosInfo {
      World::Pos previous;
      unsigned cost;
   };
   // TODO: maybe use an array.
   std::unordered_map<World::Pos, PosInfo, World::PosHash> posInfoMap{
       {start, PosInfo{{0, 0}, 0}}};

   // When `dest` can't be reached, we return the fastest path to the closest reachable
   // position.
   World::Pos closest = start;
   auto bestDistance = getDistance(start, dest);

   bool onLand = isLand(start);

   World::Pos current;
   while (!frontier.empty()) {
      current = frontier.top().second;
      frontier.pop();
      if (current == dest) {
         break;
      }
      static constexpr World::Pos neighborOffsets[] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}};
      for (const auto& offset : neighborOffsets) {
         World::Pos next{current[0] + offset[0], current[1] + offset[1]};
         // Only look at the cached part of the map.  XXX: this means the path will depend
         // on which part of the map is cached in some cases.
         if (!isCached(next[0], next[1])) {
            continue;
         }
         int tileCost = getMovementCost(next, onLand);
         if (tileCost < 0) {
            continue;
         }
         unsigned nextCost = posInfoMap[current].cost + tileCost;
         auto it = posInfoMap.find(next);
         if (it != posInfoMap.end()) {
            // We already visited this position.
            const PosInfo& nextInfo = it->second;
            if (nextCost >= nextInfo.cost) {
               // We already have a path to `next` that's just as fast or faster.
               continue;
            }
         }
         // This is the best path to `next` so far.
         auto distance = getDistance(next, dest);
         frontier.emplace(nextCost + distance, next);
         posInfoMap[next] = PosInfo{current, nextCost};
         if (distance < bestDistance) {
            closest = next;
            bestDistance = distance;
         }
      }
   }

   // Construct the path by going backwards from the destination (or the closest position
   // to the destination).
   if (current != dest) {
      current = closest;
   }
   std::vector<World::Pos> path;
   path.push_back(current);
   while (current != start) {
      current = posInfoMap[current].previous;
      path.push_back(current);
   }
   return path;
}

decltype(World::creatures)::iterator World::getCreatures(const World::Pos& pos) {
   return creatures.find(pos);
}

decltype(World::creatures)::const_iterator World::getCreatures(
    const World::Pos& pos) const {
   return creatures.find(pos);
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

// vim: tw=90 sts=-1 sw=3 et fdm=marker
