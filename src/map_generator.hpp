#ifndef MAP_GENERATOR_HPP_BFBAHR9V
#define MAP_GENERATOR_HPP_BFBAHR9V

#include <array>
#include <cmath>    // sqrt
#include <cstddef>  // size_t
#include <cstdint>  // int64_t
#include <random>   // minstd_rand

#include "tile_type.hpp"

class MapGenerator {
   using RNGen = std::minstd_rand;  // Meh but cheap, I guess [1].
   // [1]: http://en.cppreference.com/w/cpp/numeric/random/linear_congruential_engine

  public:
   using SeedType = RNGen::result_type;  // std::uint_fast32_t

   // The size of terrain blocks that can be requested.
   static constexpr std::size_t blockSize = 64;
   using TerrainBlock = std::array<std::array<TileType, blockSize>, blockSize>;

   MapGenerator();
   MapGenerator(SeedType seed);

   // Generate a block of blockSize^2 TileType values.
   TerrainBlock getBlock(std::int64_t i, std::int64_t j) const;

  private:
   // Set the seed of the PRNG to the one used for block (i, j).
   void seedRNG(std::int64_t i, std::int64_t j) const;

   // Get a random unit vector.
   std::array<float, 2> getGradient() const;

   mutable RNGen rNGen;
   mutable std::uniform_real_distribution<float> rNDist{0.f, 1.f};
   const SeedType seed;

   // I stopped using a class template with gridSize as a non-type template parameter
   // because that slows down compilation.  The program doesn't need MapGenerator objects
   // with different values of gridSize anyway.
   static constexpr std::size_t gridSize = 16;
};

#endif  // MAP_GENERATOR_HPP_BFBAHR9V

// vim: tw=90 sts=-1 sw=3 et
