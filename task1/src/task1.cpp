#include <fstream>
#include <iostream>
#include <boost/regex/icu.hpp>
#include <boost/regex.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "creatureTable.hpp"

typedef std::tuple<std::string, int, int, int, std::string, std::string>
CreatureType;

auto getName = [](const std::string& s) -> std::string {
   if (s.empty()) throw std::string{"name expected, got nothing"};
   static std::string pattern{R"([[:L*:] ]+)"};
   static auto regEx = boost::make_u32regex(pattern);
   if (!boost::u32regex_match(s, regEx)) {
      throw std::string{"regex '" + pattern + "' doesn't match name '" + s +
         "'"};
   }
   return s;
};

// For strength, speed, and lifetime.
auto getInt = [](const std::string& s) -> int {
   int i = 0;
   if (!s.empty()) {
      try {
         i = std::stoi(s);
      }
      catch (const std::invalid_argument& e) {
         throw std::string{"stoi: can't convert \'" + s + "\' to int"};
      }
      catch (const std::out_of_range& e) {
         throw std::string{"stoi: \'" + s + "\' is out of range of int"};
      }
   }
   return i;
};
// http://stackoverflow.com/questions/7663709/convert-string-to-int-c
// http://en.cppreference.com/w/cpp/string/basic_string/stol

auto getTraits = [](const std::string& s) -> std::string {
   static std::string pattern{R"(([A-Za-z0-9_]+( |$))*)"};
   static auto regEx = boost::make_u32regex(pattern);
   if (!boost::u32regex_match(s, regEx)) {
      throw std::string{"regex '" + pattern + "' doesn't match traits '" + s +
         "'"};
   }
   return s;
};

auto getPath = [](const std::string& s) -> std::string {
   static boost::regex regEx;

   // Writing regular expressions is when not having to consider s being empty.
   if (s.empty()) return s;

   // http://stackoverflow.com/questions/537772/what-is-the-most-correct-regular
   // http://en.wikipedia.org/wiki/Path_%28computing%29#POSIX_pathname_definition
   // I'm not sure wether I should allow paths beginning with two slashes.
   if (!boost::regex_match(s, boost::regex{R"([^\0]*)"})) {
      throw std::string{"filename '" + s + "' contains the null character'"};
   }

   // Only accept POSIX "Fully portable filenames".
   regEx = R"(([A-Za-z0-9._][A-Za-z0-9._-]{0,13}(/+|$))*)";
   if (!boost::regex_match(s, regEx)) {
      throw std::string{"path '" + s + "' should be made up of POSIX \"fully " +
         "portable filenames\""};
   }

   // Don't accept consecutive slashes.
   regEx = "//";
   if (boost::regex_search(s, regEx)) {
      throw std::string{"path '" + s + "' contains consecutive slashes"};
   }
   return s;
};

auto extractors = std::make_tuple(getName, getInt, getInt, getInt, getTraits,
   getPath);

   template <typename T>
   struct Print {
      void operator()(const T& t) {
         std::cout << "'" << t << "', ";
      }
   };

   template <>
   struct Print<int> {
      void operator()(const int& i) {
         std::cout << i << ", ";
      }
   };

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

   std::vector<std::string> errors;
   std::vector<CreatureType> creatureTypes;

   try {
      creatureTypes = loadCreatureTypes<CreatureType>(std::move(iStream),
         extractors, errors);
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

   // http://stackoverflow.com/questions/3072795/how-to-count-lines-of-a-file-in
   iStream.seekg(0);
   int lines = std::count(std::istreambuf_iterator<char>(iStream),
      std::istreambuf_iterator<char>(), '\n');

   std::cout << argv[0] << ": done parsing " << argv[1] << ":\n  "
             << creatureTypes.size()
             << " creature types imported successfully, "
             << lines - creatureTypes.size() << " lines skipped" << std::endl;

   if (!errors.empty()) {
      std::cerr << argv[0] << ": errors while parsing " << argv[1] << ':'
                << std::endl;
      for (const auto& error : errors) {
         std::cerr << "  " << error << std::endl;
      }
   }

   std::cout << argv[0] << ": imported creature types:" << std::endl;
   for (const auto& creatureType : creatureTypes) {
      std::cout << "  ";
      for_each<Print>(creatureType);
      std::cout << std::endl;
      //printCreatureType(creatureType);
   }
}

// vim: tw=90 sts=-1 sw=3 et
