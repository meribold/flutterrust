#include <iostream>
#include <string>
#include <tuple>
#include <utility> // std::move
#include <vector>

namespace {
   auto extractors = std::make_tuple(getName, getInt, getInt, getInt, getTraits, getPath);
}

std::vector<CreatureType>
loadCreatureTypes(std::istream&& iStream, std::vector<std::string>& errors)
{
   return loadResources<CreatureType>(std::move(iStream), extractors, errors);
}

// vim: tw=90 sts=-1 sw=3 et
