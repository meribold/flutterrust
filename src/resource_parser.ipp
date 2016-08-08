#include <algorithm>
#include <array>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <iostream>
#include <type_traits>
#include <utility>

#include "tuple_helpers.hpp"

namespace {
   // FIXME: Use the generic forEach?
   template <std::size_t i = 0, typename... Entries, typename... Extractors>
   inline typename std::enable_if<i == sizeof...(Entries), void>::type
   loadResources(std::tuple<Entries...>&,
                 std::tuple<Extractors...>&,
                 const std::array<std::string, sizeof...(Entries)>&) {}

   template <std::size_t i = 0, typename... Entries, typename... Extractors>
   inline typename std::enable_if<i < sizeof...(Entries), void>::type
   loadResources(std::tuple<Entries...>& creatureType,
                 std::tuple<Extractors...>& extractors,
                 const std::array<std::string, sizeof...(Entries)>& fields)
   {
      try {
         std::get<i>(creatureType) = std::get<i>(extractors)(fields[i]);
      }
      catch (const std::exception& e) {
         throw std::string{"token " + std::to_string(i) + ": " + e.what()};
      }
      catch (const std::string& s) {
         throw std::string{"token " + std::to_string(i) + ": " + s};
      }
      catch (...) {
         throw std::string{"token " + std::to_string(i)};
      }
      loadResources<i + 1, Entries...>(creatureType, extractors, fields);
   }
}

template <typename Tuple, typename Extractors>
std::vector<Tuple>
loadResources(std::istream&& iStream, Extractors extractors,
              std::vector<std::string>& errors)
{
   constexpr auto fieldCount = std::tuple_size<Tuple>::value;

   std::vector<Tuple> resources;
   std::istream::char_type nextChar;

   int line   = 1;
   int column = 1;

   // Use whatever type fieldCount has but make sure it's not const (or volatile).
   typename std::remove_cv<decltype(fieldCount)>::type fieldIndex = 0;

   std::array<std::string, fieldCount> fields;

   while ((nextChar = iStream.peek()) != std::istream::traits_type::eof()) {
      if (nextChar == '\n') {
         if (fieldIndex < fieldCount - 1 /*&& column > 1*/) {
            // Missing tokens.
            errors.push_back(std::to_string(line) + ":" + std::to_string(column) +
               ": additional entry expected before linebreak");
         }
         else if (fieldIndex == fieldCount - 1) {
            resources.push_back(Tuple{});
            try {
               loadResources(resources.back(), extractors, fields);
            } catch (const std::string& s) {
               resources.pop_back();
               errors.push_back(std::to_string(line) + ": parsing error: " + s);
            } catch (...) {
               resources.pop_back();
               errors.push_back(std::to_string(line) + ": unknown parsing error: ");
            }
         }
         for (auto& s : fields) {
            s.clear();
         }
         ++line; column = 0; fieldIndex = 0;
      }
      else if (nextChar == ',') {
         if (++fieldIndex > fieldCount - 1) {
            // To many tokens.
            errors.push_back(std::to_string(line) + ":" + std::to_string(column) +
               ": entry expected to be final; got \'" + nextChar + "'");
            iStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            iStream.unget();
            continue;
         }
      }
      else {
         fields[fieldIndex].push_back(nextChar);
      }

      iStream.seekg(1, std::ios_base::cur);
      ++column;
   }

   return resources;
}

auto getName = [](const std::string& s) -> std::string {
   if (s.empty()) throw std::string{"name expected, got nothing"};
   static std::string pattern{R"([[:L*:] ]+)"};
   static auto regEx = boost::make_u32regex(pattern);
   if (!boost::u32regex_match(s, regEx)) {
      throw std::string{"regex '" + pattern + "' doesn't match name '" + s + "'"};
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

auto getPath = [](const std::string& s) -> std::string {
   static boost::regex regEx;

   // Writing regular expressions is easier when not having to consider s being empty.
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

// vim: tw=90 sts=-1 sw=3 et
