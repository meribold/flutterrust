#include "bitmap.hpp"

#include <cstdint>   // uint8_t, uint16_t
#include <fstream>   // ifstream
#include <stdexcept> // runtime_error
// #include <iostream>

using std::uint8_t;
using std::uint16_t;

// 18 bytes, always.
struct TargaHeader {
   uint8_t iDLength;
   uint8_t colorMapType;
   uint8_t imageType;
   struct ColorMapSpecification {
      uint16_t firstEntryIndex;
      uint16_t length;
      uint8_t  entrySize;
   } __attribute__((packed)) colorMapSpecification;
   struct ImageSpecification {
      uint16_t xOrigin;
      uint16_t yOrigin;
      uint16_t width;
      uint16_t height;
      uint8_t  pixelDepth;
      uint8_t  imageDescriptor;
   } __attribute__((packed)) imageSpecification;
} __attribute__((packed));

Bitmap::Bitmap(const std::string& path) {
   std::ifstream iS{path, std::ios::binary | std::ios::in};
   iS.exceptions(std::ifstream::failbit | std::ifstream::badbit);
   if (!iS.is_open()) {
      throw std::runtime_error{"Can't open file: \"" + path + '"'};
   }
   TargaHeader targaHeader;
   iS.read(reinterpret_cast<char*>(&targaHeader), sizeof(targaHeader));
   auto imageSpec = targaHeader.imageSpecification;

   /*
   auto colorMapSpecification = targaHeader.colorMapSpecification;
   auto cMSFirstEntryIndex = colorMapSpecification.firstEntryIndex;
   auto imageDescriptor = imageSpec.imageDescriptor;
   std::cerr << "iDLength: " << static_cast<int>(targaHeader.iDLength) << '\n'
             << "colorMapType: " << static_cast<int>(targaHeader.colorMapType) << '\n'
             << "imageType: " << static_cast<int>(targaHeader.imageType) << '\n'
             << "cMSFirstEntryIndex: " << static_cast<int>(cMSFirstEntryIndex) << '\n'
             << "xOrigin: " << static_cast<int>(imageSpec.xOrigin) << '\n'
             << "yOrigin: " << static_cast<int>(imageSpec.yOrigin) << '\n'
             << "width: " << static_cast<int>(imageSpec.width) << '\n'
             << "height: " << static_cast<int>(imageSpec.height) << '\n'
             << "pixelDepth: " << static_cast<int>(imageSpec.pixelDepth) << '\n'
             << "imageDescriptor: " << static_cast<int>(imageDescriptor) << std::endl;
   */

   // Skip the optional image ID field.  Not technically required for the assignment:
   // it's guaranteed to be zero for all TGAs we need to support.
   iS.ignore(targaHeader.iDLength);
   // Skip the color map data.  Not technically required for the same reason.
   iS.ignore(targaHeader.colorMapSpecification.length);

   numRows = imageSpec.width; numCols = imageSpec.height;
   std::streamsize numPixels = numRows * numCols;
   // quadlets = decltype(quadlets){new RGBAQuadlet[numPixels]};
   quadlets = std::make_unique<RGBAQuadlet[]>(numPixels);

   // Now get the pixel data.
   switch (imageSpec.pixelDepth) {
      case 24: {
         for (decltype(numPixels) i = 0; i < numPixels; ++i) {
            iS.read(reinterpret_cast<char*>(&quadlets[i]), 3);
            quadlets[i][3] = 0xff;
         }
         break;
      }
      case 32: {
         iS.read(reinterpret_cast<char*>(quadlets.get()), 4 * numPixels);
         break;
      }
      default:
         throw std::runtime_error{"Only 24 or 32 bits per pixel are supported"};
   }
}

const RGBAQuadlet* Bitmap::operator[](int row) const {
   return &quadlets[numCols * row];
}

const RGBAQuadlet& Bitmap::operator()(int row, int col) const {
   // TODO: this version could perform bounds checking.
   return quadlets[numCols * row + col];
}

// Get an ASCII character representing the intensity of the pixel.
char getGlyph(const RGBAQuadlet& quadlet) {
   static constexpr std::size_t size = 11;
   static constexpr auto divident = 255 * 3 * 255;
   static constexpr std::array<char, size> shadeGlyphs{{
      ' ', '.', ':', '-', '~', '"', '+', 'o', '*', '%', '#'
   }};
   std::size_t index = (quadlet[0] + quadlet[1] + quadlet[2]) * quadlet[3] * (size - 1) /
      divident;
   return shadeGlyphs.at(index);
}

// Output an ASCII representation of the bitmap.
std::ostream& operator<<(std::ostream& oS, const Bitmap& bitmap) {
   for (int row = bitmap.numRows - 1; row >= 0; --row) {
      for (std::size_t col = 0; col < bitmap.numCols; ++col) {
         auto glyph = getGlyph(bitmap[row][col]);
         oS << glyph << glyph;
      }
      oS << std::endl;
   }
   return oS;
}

// vim: tw=90 sts=-1 sw=3 et
