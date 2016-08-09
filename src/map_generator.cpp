#include "map_generator.hpp"

#include <array>
#include <cassert>  // assert
#include <cstdint>  // int64_t

#ifdef DEBUG
#include <iostream>
#endif

// Use a real random seed if available.
MapGenerator::MapGenerator() : seed{std::random_device{}()} {}

MapGenerator::MapGenerator(MapGenerator::SeedType seed) : seed{seed} {}

// Linearly interpolate between a and b.  The weight should be in the range [0.0, 1.0].
float lerp(float a, float b, float weight) { return (1.f - weight) * a + weight * b; }

std::array<float, 2> distance(std::array<float, 2> from, std::array<float, 2> to) {
   return {to[0] - from[0], to[1] - from[1]};
}

float dotProduct(std::array<float, 2> a, std::array<float, 2> b) {
   return a[0] * b[0] + a[1] * b[1];
}

MapGenerator::TerrainBlock MapGenerator::getBlock(std::int64_t row,
                                                  std::int64_t col) const {
   MapGenerator::TerrainBlock terrainBlock;

   // We need size^2 gradient vectors.
   constexpr std::size_t size = blockSize / gridSize + 1;

   // Assign a random gradient vector of unit length to each grid node.  TODO: we really
   // only need to store (size + 2) vectors at a time.
   std::array<std::array<std::array<float, 2>, size>, size> gradients;
   {
      // Use the gradient vectors of adjacent blocks for two edges.  Otherwise we would
      // get visible transitions at block edges, since adjacent tiles would use different
      // gradient vectors.
      std::size_t i = size - 1;
      std::size_t j = 0;

      // Seed of the adjacent block to the bottom.
      seedRNG(row + 1, col);
      // Get the random gradients used for the top row of the block to the bottom.  Use
      // them for this bottom row.
      for (; j < size - 1; ++j) {
         gradients[i][j] = getGradient();
      }

      // Seed of the adjacent block to the bottom-right.
      seedRNG(row + 1, col + 1);
      assert(j == size - 1);
      gradients[i][j] = getGradient();

      // Seed of the adjacent block to the right.
      seedRNG(row, col + 1);
      i = 0;
      assert(j == size - 1);
      gradients[i][j] = getGradient();
      // Throw the remaining (size - 2) pairs of random numbers for the top row away.
      rNGen.discard(2 * size - 4);
      // The next random numbers are those the block to the right uses for its leftmost
      // column.  Use them for the rightmost column.
      for (++i; i < size - 1; ++i) {
         gradients[i][j] = getGradient();
      }
   }
   // Finally, use the seed of this block.
   seedRNG(row, col);
   // The gradient vectors for the top row and leftmost column are the ones adjacent
   // blocks also use.  They are computed first.  Otherwise we would have to waste more
   // time generating and throwing away random numbers in the code generating gradients of
   // other blocks above.
   for (std::size_t j = 0; j < size - 1; ++j) {
      gradients[0][j] = getGradient();
   }
   for (std::size_t j = 0; j < size - 1; ++j) {
      for (std::size_t i = 1; i < size - 1; ++i) {
         gradients[i][j] = getGradient();
      }
   }

#ifdef DEBUG  // Assert all the gradients are unit vectors. {{{1
   {
      constexpr float epsilon = 0.01f;
      for (std::size_t i = 0; i < size; ++i) {
         for (std::size_t j = 0; j < size; ++j) {
            auto& gradient = gradients[i][j];
            assert(dotProduct(gradient, gradient) <= 1.f + epsilon);
         }
      }
   }
#endif  // }}}1

   for (std::size_t i = 0; i < blockSize; ++i) {
      for (std::size_t j = 0; j < blockSize; ++j) {
         // Convert the indices to floats in the gradient grid.
         std::array<float, 2> point{static_cast<float>(j) / gridSize,
                                    static_cast<float>(i) / gridSize};
         // Determine into which grid cell (i, j) falls; store the cell's top-left corner
         // which is also the index of that point's gradient vector.  TODO: we could
         // iterate over grid cells and avoid repeating this computation.
         std::array<std::size_t, 2> topLeft{j / gridSize, i / gridSize};

#ifdef DEBUG  // Assert use of topLeft to index gradients is correct. {{{1
         assert(topLeft[0] + 1 < gradients.size());
         assert(topLeft[1] + 1 < gradients[0].size());
#endif  // }}}1

         // For each corner of that cell, determine the distance vector from the corner to
         // the point.
         std::array<std::array<float, 2>, 4> distance;
         distance[0] = {point[0] - topLeft[0], point[1] - topLeft[1]};
         distance[1] = {distance[0][0] - 1.f, distance[0][1]};
         distance[2] = {distance[0][0], distance[0][1] - 1.f};
         distance[3] = {distance[1][0], distance[2][1]};

#ifdef DEBUG  // Validate value of distance[0]. {{{1
         assert(0.f <= distance[0][0] && distance[0][0] <= 1.f);
         assert(0.f <= distance[0][1] && distance[0][1] <= 1.f);
#endif  // }}}1

         // For each of the 4 distance vectors, compute the dot product between it and the
         // corner's gradient vector.
         std::array<float, 4> dots{
             dotProduct(distance[0], gradients[topLeft[1]][topLeft[0]]),
             dotProduct(distance[1], gradients[topLeft[1]][topLeft[0] + 1]),
             dotProduct(distance[2], gradients[topLeft[1] + 1][topLeft[0]]),
             dotProduct(distance[3], gradients[topLeft[1] + 1][topLeft[0] + 1])};

#ifdef DEBUG  // ... {{{1
         {
            constexpr float epsilon = 0.01f;
            constexpr float maxDist = std::sqrt(2) + epsilon;
            for (std::size_t n = 0; n < 4; ++n) {
               assert(dots[n] <= maxDist);
            }
         }
#endif  // }}}1

         // Interpolate between the 4 dot products.
         float xWeight = point[0] - topLeft[0];
         float yWeight = point[1] - topLeft[1];
         float topXAverage = lerp(dots[0], dots[1], xWeight);
         float bottomXAverage = lerp(dots[2], dots[3], xWeight);
         float value = lerp(topXAverage, bottomXAverage, yWeight);

         // constexpr float maxVal = std::sqrt(2) / 2;
         // assert(-maxVal <= value && value <= maxVal);
         // value += maxVal;      // Transform value into the range [0, 2 * maxValue].
         // value /= 2 * maxVal;  // Transform it into the range [0, 1].

         // I think my implementation of Perlin noise can result in values in the range
         // [-maxVal, maxVal].  However, the vast majority of values are much closer to
         // zero, so the commented out code above doesn't work very well.

         // This should kind of move most values into the range [-2.5, 2.5].
         value *= 5.f;
         // Now most should be in the range [0, 5].
         value += 2.5f;
         // Treat everything negative as 0 and everything bigger than or equal to 6 as 5.
         if (value < 0.f) value = 0.f;
         if (value >= 6.f) value = 5.f;
         // Now we only have values in [0, 6).  They can be converted to TileType values
         // by truncating.
         terrainBlock[i][j] = static_cast<TileType>(value);
      }
   }
   return terrainBlock;
}

void MapGenerator::seedRNG(std::int64_t i, std::int64_t j) const {
   // Seed the PRNG.  FIXME: do it in a way that doesn't suck.
   SeedType blockSeed;
   if (i <= 0) {
      blockSeed = (-i) * 2;
   } else {
      blockSeed = i * 2 - 1;
   }
   blockSeed <<= 16;
   if (j <= 0) {
      blockSeed |= (-j) * 2;
   } else {
      blockSeed |= j * 2 - 1;
   }
   rNGen.seed(blockSeed ^ seed);
   // Throw some random numbers away.  The first random numbers generated from similar
   // seeds are also very similar.
   rNGen.discard(3);
}

std::array<float, 2> MapGenerator::getGradient() const {
   std::array<float, 2> gradient;
   gradient[0] = rNDist(rNGen);
   gradient[1] = std::sqrt(1 - gradient[0] * gradient[0]);
   if (coin(rNGen)) gradient[1] = -gradient[1];
   return gradient;
}

// vim: tw=90 sts=-1 sw=3 et fdm=marker
