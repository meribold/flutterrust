#ifndef CREATURE_TABLE_HPP
#define CREATURE_TABLE_HPP

#include <istream>
#include <string>
#include <tuple>
#include <vector>

template <typename CreatureType, typename Extractors>
std::vector<CreatureType>
loadCreatureTypes(std::istream&&, Extractors, std::vector<std::string>& errors);

template<std::size_t i = 0, typename... Ts>
inline typename std::enable_if<i < sizeof...(Ts) - 1, void>::type
printCreatureType(const std::tuple<Ts...>& t);

template<template <typename T> class Function, std::size_t i = 0, typename... Tuple>
inline typename std::enable_if<i == sizeof...(Tuple), void>::type
for_each(const std::tuple<Tuple...>&);

#include "creatureTable.ipp"

#endif

// vim: tw=90 sts=-1 sw=3 et
