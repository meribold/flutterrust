#ifndef BITMAP_HPP_VEAHQYJM
#define BITMAP_HPP_VEAHQYJM

#include <array>
#include <climits> // CHAR_BIT
#include <cstdint> // uint8_t
#include <memory>  // unique_ptr

#include <boost/detail/endian.hpp>  // BOOST_LITTLE_ENDIAN

// Assert some basic things about the target machine at compile-time.  Prudence, not
// doubt.
static_assert (CHAR_BIT == 8, "The Bitmap class relies on bytes having 8 bits");
#ifndef BOOST_LITTLE_ENDIAN
   #error The Bitmap class relies on little-endian byte order being used
#endif

using RGBAQuadlet = std::array<std::uint8_t, 4>;

struct Bitmap {
   Bitmap(const std::string& path);
   const RGBAQuadlet* operator[](int row) const;
   const RGBAQuadlet& operator()(int row, int col) const;
   friend std::ostream& operator<<(std::ostream&, const Bitmap&);
  private:
   std::size_t numRows, numCols;
   std::unique_ptr<RGBAQuadlet[]> quadlets;
};

#endif // BITMAP_HPP_VEAHQYJM

// vim: tw=90 sts=-1 sw=3 et
