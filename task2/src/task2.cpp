#include <array>
#include <iostream>
#include <string>

#include "bitmap.hpp"

int main(int argc, char* argv[])
{
   if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " [FILENAME]" << std::endl;
      return 1;
   }

   try {
      Bitmap bitmap{argv[1]};
      std::cout << bitmap;
   } catch (const std::exception& error) {
      std::cerr << error.what() << std::endl;
      return 2;
   }
}

// vim: tw=90 sts=-1 sw=3 et
