#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

int main(int argc, char* argv[])
{
   // Input files should use UTF-8 encoding.
   std::locale::global(std::locale{"de_DE.utf8"});

   if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " [FILENAME]" << std::endl;
      return 1;
   }

   std::ifstream iStream{argv[1]};
   iStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

   if (not iStream.is_open()) {
      std::cerr << argv[0] << ": couldn't open file " << argv[1] << std::endl;
      return 2;
   }
}

// vim: tw=100 sw=3 et
