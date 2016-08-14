#include <algorithm>      // std::fill_n, std::max, std::min
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
#include <iomanip>   // setfill, setw
#include <iostream>  // cout, cerr
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
   hunt,
   consume,
   rest,
   SIZE = animalStates::rest + 5
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

void World::step() {
   ++currentStep;
#ifdef DEBUG  // {{{1
   std::cerr << "Step " << std::setfill('0') << std::setw(4) << currentStep << ": ";
#endif  // }}}1
   changedPositions.clear();
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
         changedPositions.push_back(pos);
         if (creature.isPlant()) {
            it = creatures.erase(it);
         } else {
            it = removeAnimal(it);
         }
      } else {
         ++it;
      }
   }

   // Move animals and insert new plants and animals into the hash map.
   commitStep();

   for (auto it = carcasses.begin(); it != carcasses.end();) {
      if (--it->second == 0) {
         changedPositions.push_back(it->first);
         it = carcasses.erase(it);
      } else {
         ++it;
      }
   }
#ifdef DEBUG  // {{{1
   std::cerr << creatures.size() << " denizens\n";
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
      World::Pos pos{moveeInfo.first};
      Creature animal{moveeInfo.second->second};

      changedPositions.push_back(moveeInfo.second->first);
      changedPositions.push_back(pos);

      creatures.erase(moveeInfo.second);
      creatures.emplace(pos, animal);
      assert(creatures.bucket_count() == bucketCount);
   }
   moveeCache.clear();

   // Actually spawn any new offspring.  XXX: this absolutely has to be done after moving
   // creatures.
   for (auto& offspringInfo : offspringCache) {
      creatures.insert(offspringInfo);
      changedPositions.push_back(offspringInfo.first);
   }
   offspringCache.clear();
}

void World::updatePlant(World::CreatureInfo& plantInfo) {
   const World::Pos& pos = plantInfo.first;
   Creature& plant = plantInfo.second;
   assert(plant.isPlant());
   if (plant.canProcreate(currentStep)) {
      // Get the number of plants that have the same type within 5 tiles of the parent.
      int nearbyConspecificPlants = countCreatures(pos, 5, plant.getTypeIndex());
      if (2 < nearbyConspecificPlants && nearbyConspecificPlants < 10) {
         spawnOffspring(plantInfo);
         spawnOffspring(plantInfo);
      }
   }
   const TileType tileType = getTileType(pos);
   if (tileType == TileType::water || tileType == TileType::sand ||
       tileType == TileType::dirt) {
      plant.lifetime -= 10;
   } else {
      plant.lifetime -= 25;
   }
}

std::uint16_t World::getNewAnimalState(const World::CreatureInfo& animalInfo) {
   const World::Pos& pos = animalInfo.first;
   const Creature& animal = animalInfo.second;
   auto state = animal.aiState;

   bool roaming = /* animalStates::roam <= state && */ state < numRoamStates;
   bool resting = animalStates::rest <= state && state < animalStates::rest + 5;
   bool foraging = state == animalStates::hunt;
   bool consuming = state == animalStates::consume;
   bool procreated = state == animalStates::procreate;

   if (roaming || resting) {
      if (animal.canProcreate(currentStep)) {
         int nearbyConspecifics = countCreatures(pos, 3, animal.getTypeIndex());
         if (1 < nearbyConspecifics && nearbyConspecifics < 5) {
            return animalStates::procreate;
         }
      }
   }
   if (((roaming || procreated) && animal.isHungry()) ||
       ((foraging || consuming) && !animal.isSated())) {
      // Writes the travelling distance to the closest creature the animal can feed on to
      // `distanceToFood`.  A list of the found creatures is temporarily cached in
      // `foodCache`.
      int distanceToFood;
      foodCache = findFood<10>(animalInfo, distanceToFood);
      if (!foodCache.empty()) {
         if (distanceToFood <= 1) {
            return animalStates::consume;
         } else if (distanceToFood <= 10) {
            return animalStates::hunt;
         }
      }
   }
   if (roaming && state != defaultRoamState) {
      return state;  // Continue roaming.
   }
   if (procreated) {
      return generateRoamState(animalInfo);
   }
   if (state == defaultRoamState || foraging || consuming) {
      // We were roaming but reached the destination.  Or we were foraging but there's no
      // more food; maybe it died or got too far away.  Or we were eating something but
      // are sated or there's no more food.
      return animalStates::rest;
   }
   if (resting) {
      auto timeRested = 1 + state - animalStates::rest;
      if (timeRested < std::lround(animal.getRelativeLifetime() * 5)) {
         return state + 1;  // Continue resting.
      } else {
         return generateRoamState(animalInfo);
      }
   }
   assert(false);
   return defaultAiState;
}

void World::updateAnimal(World::CreatureIt animalIt) {
   Creature& animal = animalIt->second;

   auto& state = animal.aiState;
   state = getNewAnimalState(*animalIt);

   // The first (numRoamStates - 1) states all indicate the animal is roaming.  The actual
   // value of aiState identifies the destination position relative to the animal's
   // current position.
   if (state < numRoamStates) {
      roam(animalIt);
   } else if (state == animalStates::procreate) {
      assert(animal.procreationOffset == 0);
      assert(animal.getRelativeLifetime() > 0.5);
      if (spawnOffspring(*animalIt)) {
         assert(animal.procreationOffset == animal.getProcreationInterval() - 1);
      } else {
         assert(animal.procreationOffset == 0);
      }
   } else if (state == animalStates::hunt) {
      hunt(animalIt);
   } else if (state == animalStates::consume) {
      leech(animalIt);
   } else if (animalStates::rest <= state && state < animalStates::rest + 5) {
      animal.lifetime -= 5;
   } else {
      assert(false);
   }

   if (animal.procreationOffset > 0) --animal.procreationOffset;
}

bool World::isCached(std::int64_t x, std::int64_t y) const {
   // TODO: `right` and `bottom` data members.
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
         return true;  // The tile is covered by vegetation.
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

bool isPlant(World::CreatureIt creatureIt) { return creatureIt->second.isPlant(); }

bool isHerbivore(World::CreatureIt creatureIt) {
   return creatureIt->second.isHerbivore();
}

// Get information about all nearby creatures that can be reached from `start` without
// moving a distance greater than `maxDist` for which the `UnaryPredicate` returns `true`
// and that are at least as close to `start` as all other creatures satisfying the former
// conditions.  E.g., find food.
// TODO: is there an elegant way to provide a `const` version of this function that
// returns an `std::vector<decltype(creatures)::const_iterator`?
// TODO: specialize for `maxDist == 0` and `maxDist == 1`?
template <int maxDist, typename UnaryPredicate>
std::vector<World::CreatureIt> World::getReachableCreatures(const World::Pos& start,
                                                            UnaryPredicate pred,
                                                            int& bestDist) {
   std::vector<World::CreatureIt> matches;
   auto range = creatures.equal_range(start);
   for (auto it = range.first; it != range.second; ++it) {
      if (pred(it)) {
         matches.push_back(it);
      }
   }
   if (!matches.empty()) {
      bestDist = 0;
      return matches;
   }

   bestDist = maxDist;

   // This code is regrettably similar to but also curiously different from
   // `World::getReachablePositions`.  TODO: DRY?
   using PosDistPair = std::pair<World::Pos, int>;
   std::queue<PosDistPair> frontier;
   frontier.emplace(start, 0);
   constexpr int diameter = 2 * maxDist + 1;
   auto getIndex = [&](const World::Pos& pos) {
      int i = pos[1] - start[1] + maxDist;
      int j = pos[0] - start[0] + maxDist;
      return diameter * i + j;
   };
   static bool visited[diameter * diameter];
   std::fill_n(visited, diameter * diameter, false);
   // Set the element corresponding to `start` to `true`;
   visited[diameter * maxDist + maxDist] = true;
   bool onLand = isLand(start);
   while (!frontier.empty()) {
      const World::Pos current = frontier.front().first;
      int dist = frontier.front().second + 1;
      // ...
      if (dist > bestDist) break;
      frontier.pop();
      static constexpr World::Pos neighborOffsets[] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}};
      for (const auto& offset : neighborOffsets) {
         const World::Pos next{current[0] + offset[0], current[1] + offset[1]};
         if (!isCached(next) || onLand != isLand(next)) {
            continue;
         }
         bool& visitedNext = visited[getIndex(next)];
         if (visitedNext) {
            continue;
         }
         visitedNext = true;
         auto range = creatures.equal_range(next);
         for (auto it = range.first; it != range.second; ++it) {
            if (pred(it)) {
               // Gotcha.
               matches.push_back(it);
               bestDist = dist;
            }
         }
         // We stop adding positions to `frontier` once we found any match, because we
         // aren't interested in matches that are further away from `start` than others.
         if (dist < bestDist) {
            assert(matches.empty());
            frontier.emplace(next, dist);
         }
      }
   }
   return matches;
}

template <int maxDist>
std::vector<World::CreatureIt> World::findFood(const World::CreatureInfo& animalInfo,
                                               int& distanceToFood) {
   const World::Pos& pos = animalInfo.first;
   const Creature& animal = animalInfo.second;
   assert(animal.isAnimal());
   if (animal.isHerbivore()) {
      return getReachableCreatures<maxDist>(pos, &isPlant, distanceToFood);
   } else {
      return getReachableCreatures<maxDist>(pos, &isHerbivore, distanceToFood);
   }
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
      // Get all positions the parent can reach without moving a distance greater than 3.
      std::vector<World::Pos> positions = getReachablePositions(pos, 3);
      if (positions.size() == 1) return false;  // There's no space.
      assert(positions[0] == pos);
      // Pick a random position other than the one of the parent.
      World::Pos childPos = positions[1 + defaultRNDist(rNG) % (positions.size() - 1)];
      assert(isGoodPosition(creatureType, childPos));
      std::int16_t childLifetime = std::lround(0.5 * parent.lifetime);
      offspringCache.push_back(
          CreatureInfo{childPos, Creature{parent.getTypeIndex(), childLifetime}});
      parent.lifetime = std::lround(0.75 * parent.lifetime);
      // Reset the timer specifying when the animal can reproduce again.
      parent.procreationOffset = parent.getProcreationInterval() - 1;
      return true;
   }
}

void World::leech(World::CreatureIt actorIt, World::CreatureIt targetIt) {
   Creature& actor = actorIt->second;
   Creature& target = targetIt->second;
   assert(actor.isAnimal());
   assert(target.lifetime > 0);
   std::int16_t amount = std::min(
       {static_cast<std::int16_t>(actor.getStrength()), target.lifetime,
        static_cast<std::int16_t>(2 * (actor.getMaxLifetime() - actor.lifetime))});
   actor.lifetime += amount / 2;
   assert(actor.lifetime <= actor.getMaxLifetime());
   target.lifetime -= amount;
   if (target.lifetime <= 0) {
      changedPositions.push_back(targetIt->first);
      if (target.isPlant()) {
         creatures.erase(targetIt);
      } else {
         removeAnimal(targetIt);
      }
   }
}

void World::leech(World::CreatureIt actorIt) {
   assert(actorIt->second.isAnimal());
   assert(!foodCache.empty());
   World::CreatureIt targetIt;
   if (foodCache.size() == 1)
      // This is probably common enough to make it worth the optimization.
      targetIt = foodCache[0];
   else
      targetIt = foodCache[defaultRNDist(rNG) % (foodCache.size())];
   assert(targetIt->second.lifetime > 0);
   leech(actorIt, targetIt);
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
   Creature& animal = animalIt->second;
   assert(isCached(animalIt->first));
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

void World::hunt(World::CreatureIt animalIt) {
   assert(animalIt->second.isAnimal());
   assert(!foodCache.empty());
   // Pick a random creature.
   World::CreatureIt targetIt;
   if (foodCache.size() == 1)
      // This is probably common enough to make it worth the optimization.
      targetIt = foodCache[0];
   else
      targetIt = foodCache[defaultRNDist(rNG) % (foodCache.size())];
   const World::Pos& dest = targetIt->first;
   // FIXME: we almost already computed the path when we built `foodCache`...
   moveTowards(animalIt, dest, true);
}

// Compute the shortest path from `start` to `dest` using the A* algorithm.  Based on
// [this introduction][1].  TODO: based on the demos, I think the linked page uses the
// estimated distance to the destination as a tiebreaker when multiple positions have the
// same priority; maybe implement that optimization.
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

   if (current != dest) {
      current = closest;
   }

   // Construct the path by going backwards from the destination (or the closest position
   // to the destination).  XXX: the vector we return is reversed.
   std::vector<World::Pos> path;
   path.push_back(current);
   while (current != start) {
      current = posInfoMap[current].previous;
      path.push_back(current);
   }
   return path;
}

std::vector<World::Pos> World::getReachablePositions(const World::Pos& start,
                                                     int maxDist) const {
   std::vector<World::Pos> positions{start};
   using PosDistPair = std::pair<World::Pos, int>;
   std::queue<PosDistPair> frontier;
   frontier.emplace(start, 0);
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

World::Pos World::moveTowards(World::CreatureIt animalIt, const World::Pos& dest,
                              bool run) {
   assert(animalIt != creatures.end());
   const World::Pos& pos = animalIt->first;
   Creature& animal = animalIt->second;
   assert(isGoodPosition(animal.getType(), dest));
   std::size_t range = run ? animal.getRunSpeed() : animal.getWalkSpeed();
   assert(distance(pos, dest) <= maxRoamDist);
   const std::vector<Pos> path = getPath(pos, dest);
   // The path includes the current position.
   auto distanceMoved = std::min(range, path.size() - 1);
   assert(distanceMoved <= maxRoamDist);
   if (run) {
      animal.lifetime -= 10 * distanceMoved;
   } else {
      animal.lifetime -= 2 * distanceMoved;
   }
   World::Pos newPos = *(path.rbegin() + distanceMoved);
   assert(distance(newPos, dest) <= maxRoamDist);
   moveeCache.push_back(std::make_pair(newPos, animalIt));
   return newPos;
}

World::CreatureIt World::removeAnimal(World::CreatureIt animalIt) {
   assert(animalIt->second.isAnimal());
   // FIXME: inefficient.
   for (auto it = moveeCache.begin(); it != moveeCache.end();) {
      if (it->second == animalIt) {
         it = moveeCache.erase(it);
      } else {
         ++it;
      }
   }
   carcasses[animalIt->first] = 10;  // Display the carcass graphic for 10 steps.
   return creatures.erase(animalIt);
}

// Map a signed integer number z to the interval [0, 2^n - 1].  Injective for the domain
// [- 2^(n-1), 2^(n-1) - 1].
template <std::size_t n, typename Z>
inline std::size_t toNBits(Z z) {
   // Signed, because using an unsigned type in operations with signed ones can cause the
   // signed operand to be converted to an unsigned type ("usual arithmetic conversions").
   constexpr Z addend = Z{1} << (n - 1);  // 2^(n-1)
   constexpr Z divisor = Z{1} << n;       // 2^n
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
