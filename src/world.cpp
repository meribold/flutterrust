#include <algorithm>      // std::fill_n
#include <cassert>        // assert
#include <climits>        // CHAR_BIT
#include <cmath>          // pow, lround, abs
#include <cstdint>        // int64_t
#include <cstdlib>        // abs
#include <memory>         // unique_ptr
#include <queue>          // priority_queue, queue
#include <random>         // std::default_random_engine, std::random_device, ...
#include <unordered_map>  // unordered_map
#include <vector>         // vector

#ifdef DEBUG
#include <iostream>
#endif

#include "world.hpp"

namespace {
std::default_random_engine rNG{std::random_device{}()};
std::uniform_int_distribution<int> defaultRNDist{};  //  [0, numeric_limits<int>::max()]
std::uniform_int_distribution<int> coinDist(0, 1);

// The maximum value of a component of an animal's offset to the destination it's roaming
// towards that can be stored.  While no destinations that are more than 10 tiles (in
// Manhattan metric) away from the animal are selected, the optimal path may initially
// move away from the destination.
constexpr int maxRoamDist = 40;

// Used to extract the x- and y-coordinates of the offset an animal is roaming towards
// from its AI state.
constexpr int roamDivisor = 2 * maxRoamDist + 1;  // 81

// The maximum AI state that indicates the animal is roaming.
constexpr std::uint16_t numRoamStates = roamDivisor * roamDivisor;  // 6561

// AI state that indicates an animal is roaming but has reached its destination.  3280.
// 3280 / 81 = 3280 % 81 = 40.
constexpr std::uint16_t defaultRoamState = (numRoamStates - 1) / 2;
}

enum animalStates : std::uint16_t {
   roam = 0,  // Not actually a valid state.
   procreate = animalStates::roam + numRoamStates,
   forage,
   consume,
   rest,
   decompose = animalStates::rest + 5,
   decomposed = animalStates::decompose + 10,
   SIZE
};

constexpr std::uint16_t defaultAiState = defaultRoamState;

// Get where an animal is moving towards relative to its current position.  Determined by
// the animal's AI state.
std::array<int, 2> roamStateToOffset(std::uint16_t aiState) {
   assert(aiState < numRoamStates);
   std::array<int, 2> offset;
   offset[0] = aiState % roamDivisor;
   offset[1] = aiState / roamDivisor;
   offset[0] -= maxRoamDist;
   offset[1] -= maxRoamDist;
   assert(std::abs(offset[0]) + std::abs(offset[1]) <= maxRoamDist);
   return offset;
}

std::uint16_t offsetToRoamState(const World::Pos& offset) {
   assert(std::abs(offset[0]) + std::abs(offset[1]) <= maxRoamDist);
   std::uint16_t roamState =
       roamDivisor * (offset[1] + maxRoamDist) + offset[0] + maxRoamDist;
   assert(roamState < numRoamStates);
   return roamState;
}

std::uint16_t offsetToRoamState(const World::Pos& start, const World::Pos& dest) {
   World::Pos offset{dest[0] - start[0], dest[1] - start[1]};
   assert(std::abs(offset[0]) + std::abs(offset[1]) <= maxRoamDist);
   return offsetToRoamState(offset);
}

// Get the position an animal is currently roaming towards.
World::Pos getRoamDest(const World::CreatureInfo& animalInfo) {
   const World::Pos& pos = animalInfo.first;
   const Creature& animal = animalInfo.second;
   assert(animal.aiState < numRoamStates);
   auto offset = roamStateToOffset(animal.aiState);
   World::Pos dest{pos};
   dest[0] += offset[0];
   dest[1] += offset[1];
   return dest;
}

World::World() {}

// TODO: return a list of positions the GUI should redraw.
void World::step() {
   ++currentStep;
#ifdef DEBUG  // {{{1
   std::cerr << "Starting step " << currentStep << ": " << creatures.size()
             << " denizens\n";
#endif  // }}}1
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
         updateAnimal(it);
      }
      if (creature.lifetime <= 0) {
         if (creature.isPlant()) {
            it = creatures.erase(it);
         } else {
            killList.push_back(it);
            ++it;
         }
      } else {
         ++it;
      }
   }

   // Move animals, remove animals, and insert new plants and animals into the hash map.
   commitStep();

   for (auto it = carcasses.begin(); it != carcasses.end();) {
      if (--it->second == 0) {
         it = carcasses.erase(it);
      } else {
         ++it;
      }
   }

#ifdef DEBUG  // {{{1
   std::cerr << "Finished step " << currentStep << ": " << creatures.size()
             << " denizens\n";
#endif  // }}}1
}

void World::commitStep() {
// Really move animals.
#ifdef DEBUG  // ... {{{1
   const auto bucketCount = creatures.bucket_count();
#endif  // }}}1
   for (auto& moveeInfo : moveeCache) {
      // XXX: [en.cppreference.com][1] has this to say about `unordered_multimap::insert`:
      // "If rehashing occurs due to the insertion, all iterators are invalidated".  But
      // also "[r]ehashing occurs only if the new number of elements is greater than
      // `max_load_factor() * bucket_count()`".
      // This leads me to believe erasing and reinserting elements one a time without ever
      // increasing the number of elements should be safe.
      // [1]: http://en.cppreference.com/w/cpp/container/unordered_multimap/insert
      // [2]: http://en.cppreference.com/w/cpp/container/unordered_multimap/rehash
      auto copy = moveeInfo;
      creatures.erase(moveeInfo.second);
      creatures.emplace(copy.first, copy.second->second);
      assert(creatures.bucket_count() == bucketCount);
   }
   moveeCache.clear();

   // XXX: just erasing animals directly in the main loop in World::step() causes
   // undefined behavior if we try to move the animal later.  TODO: just remove the animal
   // from `moveeCache` as well, when removing it from `creatures`?
   for (World::CreatureIt animalIt : killList) {
      creatures.erase(animalIt);
      carcasses[animalIt->first] = 10;  // Display the carcass graphic for 10 steps.
   }
   killList.clear();

   // Actually spawn any new offspring.  XXX: this absolutely has to be done after moving
   // creatures.
   for (auto& offspringInfo : offspringCache) {
      creatures.insert(offspringInfo);
   }
   offspringCache.clear();
}

void World::updatePlant(World::CreatureInfo& plantInfo) {
   const World::Pos& pos = plantInfo.first;
   Creature& plant = plantInfo.second;
   if (plant.shouldProcreate(currentStep)) {
      // Get the number of plants that have the same type within 5 tiles of the parent.
      int nearbyConspecificPlants = countCreatures(pos, 5, plant.getTypeIndex());
      if (2 < nearbyConspecificPlants && nearbyConspecificPlants < 10) {
         spawnOffspring(plantInfo);
         spawnOffspring(plantInfo);
      }
   }
   age(plantInfo);
}

std::uint16_t World::getNewAnimalState(const World::CreatureInfo& animalInfo) const {
   const World::Pos& pos = animalInfo.first;
   const Creature& animal = animalInfo.second;
   auto& state = animal.aiState;
   if (/* animalStates::roam <= state && */ state < numRoamStates) {
      // The animal is currently roaming.  We can transition to: procreating, foraging /
      // hunting, resting, or continue roaming.
      if (animal.shouldProcreate(currentStep)) {
         int nearbyConspecifics = countCreatures(pos, 3, animal.getTypeIndex());
         if (1 < nearbyConspecifics && nearbyConspecifics < 5) {
            return animalStates::procreate;
         }
      }
      if (animal.isHungry()) {
         if (isFoodNearby(animalInfo, 10)) {
            return animalStates::forage;
         }
      }
      if (state == defaultRoamState) {
         // We were roaming but reached the destination.
         return animalStates::rest;
      }
      return state;  // Continue roaming.
   }
   if (state == animalStates::procreate) {
      return generateRoamState(animalInfo);
   }
   if (state == animalStates::forage) {
      if (isFoodNearby(animalInfo, 1)) {
         return animalStates::consume;
      } else if (isFoodNearby(animalInfo, 10)) {
         return animalStates::forage;
      }
      return generateRoamState(animalInfo);
   }
   if (state == animalStates::consume) {
      if (animal.isSated()) {
         return animalStates::rest;
      }
      if (isFoodNearby(animalInfo, 1)) {
         return animalStates::consume;
      }
      if (isFoodNearby(animalInfo, 10)) {
         return animalStates::forage;
      }
      return generateRoamState(animalInfo);
   }
   if (animalStates::rest <= state && state < animalStates::rest + 5) {
      if (state != animalStates::rest + 4) {
         return state + 1;  // Continue resting.
      } else {
         return generateRoamState(animalInfo);
      }
   }
   /*
   if (animalStates::decompose <= state && state < animalStates::decompose + 10) {
      return state + 1;  // Keep it up.
   }
   */
   assert(false);
}

void World::updateAnimal(World::CreatureIt animalIt) {
   Creature& animal = animalIt->second;

   auto& state = animal.aiState;
   /*
   // Allow multiple transitions.
   std::uint16_t oldState;
   do {
      oldState = state;
      state = getNewAnimalState(*animalIt);
   } while (state != oldState);
   */
   state = getNewAnimalState(*animalIt);

   // The first (numRoamStates - 1) states all indicate the animal is roaming.  The actual
   // value of aiState identifies the destination position relative to the animal's
   // current position.
   if (state < numRoamStates) {
      std::cerr << "Roam.\n";
      roam(animalIt);
   } else if (state == animalStates::procreate) {
      std::cerr << "Procreate.\n";
   } else if (state == animalStates::forage) {
      std::cerr << "Forage.\n";
   } else if (state == animalStates::consume) {
      std::cerr << "Consume.\n";
   } else if (animalStates::rest <= state && state < animalStates::rest + 5) {
      std::cerr << "Rest: " << state - animalStates::rest << '\n';
      animal.lifetime -= 5;
   }
   /*
   else if (animalStates::decompose <= state && state < animalStates::decompose + 10) {
      // ...
   } else if (state == animalStates::decomposed) {
      // killList.push_back(animalIt);
   }
   */
   else {
      assert(false);
   }

   /*
   if (animal.lifetime <= 0 && state < animalStates::decompose) {
      state = animalStates::decompose;
   }
   */
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

bool World::isVegetated(const World::Pos pos) const {
   auto range = creatures.equal_range(pos);
   for (auto it = range.first; it != range.second; ++it) {
      if (it->second.isPlant()) {
         // The tile is covered by vegetation.
         return true;
      }
   }
   return false;
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

bool World::isGoodPosition(const CreatureType& creatureType, std::int64_t x,
                           std::int64_t y) const {
   return isGoodPosition(creatureType, World::Pos{x, y});
}

int World::countCreatures(const World::Pos& pos, int radius,
                          std::uint8_t creatureTypeIndex) const {
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

bool World::isFoodNearby(const CreatureInfo& animalInfo, int radius) const {
   const World::Pos& pos = animalInfo.first;
   const Creature& animal = animalInfo.second;
   assert(animal.isAnimal());
   for (int xOffset = -radius; xOffset <= radius; ++xOffset) {
      int maxYOffset = radius - std::abs(xOffset);
      for (int yOffset = -maxYOffset; yOffset <= maxYOffset; ++yOffset) {
         assert(std::abs(xOffset) + std::abs(yOffset) <= radius);
         auto range = creatures.equal_range({pos[0] + xOffset, pos[1] + yOffset});
         for (auto it = range.first; it != range.second; ++it) {
            const auto& food = it->second;
            if (animal.isHerbivore() && food.isPlant()) {
               return true;
            } else if (animal.isCarnivore() && food.isHerbivore()) {
               return true;
            }
         }
      }
   }
   return false;
}

void World::spawnCreature(std::uint8_t typeIndex, std::int64_t x, std::int64_t y) {
   // Assert we don't try to place a creature on a hostile tile (e.g. a fish on land).
   assert(isGoodPosition(Creature::getTypes()[typeIndex], {x, y}));
   auto it = creatures.emplace(Pos{x, y}, Creature{typeIndex});
   it->second.aiState = generateRoamState(*it);
}

bool World::spawnOffspring(World::CreatureInfo& parentInfo) {
   static std::uniform_int_distribution<int> rNDist{-5, 5};
   const World::Pos& pos = parentInfo.first;
   Creature& parent = parentInfo.second;
   const CreatureType& creatureType = parent.getType();
   if (parent.isPlant()) {
      // Randomly pick a position and create offspring if the position's type matches the
      // plants natural environment (land or water).  TODO: create a list of eligible
      // positions first and randomly pick one of those instead?
      int xOffset = rNDist(rNG);
      int yOffset = 5 - std::abs(xOffset);
      if (coinDist(rNG)) {
         yOffset = -yOffset;
      }
      World::Pos childPos{pos[0] + xOffset, pos[1] + yOffset};
      assert(distance(pos, childPos) == 5);
      if (!isCached(childPos)) {
         return false;
      }
      if (!isGoodPosition(creatureType, childPos)) {
         return false;
      }
      if (isVegetated(childPos)) {
         return false;
      }
      offspringCache.push_back(CreatureInfo{childPos, Creature{parent.getTypeIndex()}});
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

// Generate a random AI state corresponding to a position the animal can move to.
std::uint16_t World::generateRoamState(const World::CreatureInfo& animalInfo) const {
   const World::Pos& pos = animalInfo.first;
   // TODO: exclude the animal's current positions from the candidates?  What if that's
   // the only candidate?  It is the only one that is guaranteed.
   std::vector<World::Pos> positions = getReachablePositions(pos, 10);
   const World::Pos dest = positions[defaultRNDist(rNG) % positions.size()];
   assert(isCached(dest));
   // The path includes the current position.
   assert(getPath(pos, dest).size() <= maxRoamDist + 1);
   return offsetToRoamState(pos, dest);
}

void World::roam(World::CreatureIt animalIt) {
   const World::Pos& pos = animalIt->first;
   Creature& animal = animalIt->second;
   assert(isCached(pos));
   assert(animal.aiState < numRoamStates);
   if (animal.aiState == defaultRoamState) {
      // TODO: should we allow that this happens?  Maybe the animal can't move anywhere.
   } else {
      const World::Pos dest = getRoamDest(*animalIt);
      if (!isCached(dest)) {
         // XXX: the user scrolled and the position the animal was roaming towards is no
         // longer cached.
         animal.aiState = defaultRoamState;
         animal.lifetime -= 5;
      } else {
         const World::Pos newPos = moveTowards(animalIt, dest, false);
         animal.aiState = offsetToRoamState(newPos, dest);
         assert(animal.aiState < numRoamStates);
      }
   }
}

// Compute the shortest path from `start` to `dest` using the A* algorithm.  Based on
// [this introduction][1].  TODO: based on the demos, I think the linked page uses the
// estimated distance to the destination as a tiebreaker when multiple positions have the
// same priority; mayby implement that optimization.
// [1]: http://redblobgames.com/pathfinding/a-star/introduction.html
std::vector<World::Pos> World::getPath(World::Pos start, World::Pos dest) const {
   assert(isCached(start));
   assert(isCached(dest));
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
   auto bestDistance = distance(start, dest);

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
         auto newDistance = distance(next, dest);
         frontier.emplace(nextCost + newDistance, next);
         posInfoMap[next] = PosInfo{current, nextCost};
         if (newDistance < bestDistance) {
            closest = next;
            bestDistance = newDistance;
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
      // FIXME: inefficient!!
      path.insert(path.begin(), current);
      // path.push_back(current);
   }
   return path;
}

std::vector<World::Pos> World::getReachablePositions(const World::Pos& start,
                                                     int maxDist) const {
   using PosDistPair = std::pair<World::Pos, int>;
   std::queue<PosDistPair> frontier;
   frontier.emplace(start, 0);
   std::vector<World::Pos> positions{start};  // TODO: reserve some space.
   // Diameter of the square containing all positions that are potentially reachable
   // without moving a distance greater than `maxDist`.
   const int diameter = 2 * maxDist + 1;
   // This should be the absolute maximum number of elements we may need.
   positions.reserve(diameter * diameter - 1);
   auto getIndex = [&](const World::Pos& pos) {
      int i = pos[1] - start[1] + maxDist;
      int j = pos[0] - start[0] + maxDist;
      assert(0 <= i && i < diameter);
      assert(0 <= j && j < diameter);
      return diameter * i + j;
   };
   std::unique_ptr<bool[]> visited(new bool[diameter * diameter]);
   // Initialize the array by setting all elements to `false`.
   std::fill_n(visited.get(), diameter * diameter, false);
   // Set the element corresponding to `start` to `true`;
   visited[diameter * maxDist + maxDist] = true;
   bool onLand = isLand(start);
   while (!frontier.empty()) {
      const World::Pos current = frontier.front().first;
      int dist = frontier.front().second + 1;
      assert(dist <= maxDist);
      frontier.pop();
      static constexpr World::Pos neighborOffsets[] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}};
      for (const auto& offset : neighborOffsets) {
         const World::Pos next{current[0] + offset[0], current[1] + offset[1]};
         if (!isCached(next) || onLand != isLand(next)) {
            continue;
         }
         assert(distance(current, next) == 1);
         assert(visited[getIndex(current)]);
         assert(distance(start, next) <= maxDist);
         bool& visitedNext = visited[getIndex(next)];
         if (visitedNext) {
            continue;
         }
         assert(distance(start, next) <= dist);  // We can't get a path that's shorter
                                                 // than the Manhattan distance.
         visitedNext = true;
         positions.push_back(next);
         if (dist != maxDist) {
            frontier.emplace(next, dist);
         }
      }
   }
   return positions;
}

World::Pos World::moveTowards(CreatureIt animalIt, const World::Pos& dest, bool run) {
   assert(animalIt != creatures.end());
   const World::Pos& pos = animalIt->first;
   Creature& animal = animalIt->second;
   assert(isGoodPosition(animal.getType(), dest));
   std::size_t range = run ? animal.getRunSpeed() : animal.getWalkSpeed();
   assert(distance(pos, dest) <= maxRoamDist);
   const std::vector<Pos> path = getPath(pos, dest);
   auto distanceMoved = path.size() - 1;  // The path includes the current position.
   assert(distanceMoved <= maxRoamDist);
   if (run) {
      animal.lifetime -= 10 * distanceMoved;
   } else {
      animal.lifetime -= 5 * distanceMoved;
   }
   World::Pos newPos = range < path.size() ? path[range] : path.back();
   assert(distance(newPos, dest) <= maxRoamDist);
   moveeCache.push_back(std::make_pair(newPos, animalIt));
   return newPos;
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

// Manhattan distance from a to b.
std::int64_t distance(const World::Pos& a, const World::Pos& b) {
   return std::abs(b[0] - a[0]) + std::abs(b[1] - a[1]);
}

// vim: tw=90 sts=-1 sw=3 et fdm=marker
