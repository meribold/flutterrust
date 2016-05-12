#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "creature_parser.hpp"

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

   // http://stackoverflow.com/q/3072795/how-to-count-lines-of-a-file-in-c
   const int lines = std::count(std::istreambuf_iterator<char>(iStream),
      std::istreambuf_iterator<char>(), '\n');
   iStream.seekg(0);

   std::vector<CreatureType> creatureTypes;
   std::vector<std::string> errors;

   try {
      creatureTypes = loadCreatureTypes(std::move(iStream), errors);
   } catch (const std::ios_base::failure& e) {
      std::cerr << argv[0] << ": exception thrown while parsing " << argv[1]
                << ": " << e.what() << std::endl;
      //return e.code();
   } catch (const std::exception& e) {
      std::cerr << argv[0] << ": exception thrown while parsing " << argv[1]
                << ": " << e.what() << std::endl;
      return 3;
   } catch (...) {
      std::cerr << argv[0] << ": unknown exception thrown while parsing "
                << argv[1] << std::endl;
      return 3;
   }

   std::cout << argv[0] << ": done parsing " << argv[1] << ":\n  "
             << creatureTypes.size() << " creature types imported successfully, "
             << lines - creatureTypes.size() << " line(s) skipped" << std::endl;

   if (!errors.empty()) {
      std::cerr << argv[0] << ": errors while parsing " << argv[1] << ':' << std::endl;
      for (const auto& error : errors) {
         std::cerr << "  " << error << std::endl;
      }
   }

   std::cout << argv[0] << ": imported creature types:" << std::endl;
   for (const auto& creatureType : creatureTypes) {
      std::cout << "  ";
      printTuple(creatureType);
      std::cout << std::endl;
   }
}

// vim: tw=90 sts=-1 sw=3 et
