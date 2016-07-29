#ifndef RESOURCE_PARSER_HPP_E1ZJ3OTU
#define RESOURCE_PARSER_HPP_E1ZJ3OTU

#include <istream>
#include <string>
#include <tuple>
#include <vector>

template <typename T, typename Extractors>
std::vector<T> loadResources(std::istream&&, Extractors,
                             std::vector<std::string>& errors);

#include "resource_parser.ipp"

#endif  // RESOURCE_PARSER_HPP_E1ZJ3OTU

// vim: tw=90 sts=-1 sw=3 et
