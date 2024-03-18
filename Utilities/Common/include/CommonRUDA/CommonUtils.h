#ifndef CommonUtils_h
#define CommonUtils_h
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <algorithm>
namespace utilities
{
namespace common
{
template<typename CallableInsert, typename Callable, typename ContType>
auto makeContainerProduct(CallableInsert &&opPush,Callable &&op, const ContType &cont1, const ContType &cont2)->ContType {
  ContType newContainer;
  for(const auto &entry1: cont1) {
    for(const auto &entry2: cont2) {
      opPush(newContainer, op(entry1, entry2));
    }
  }
  return newContainer;
}

template<typename MapType, typename ValueType = typename MapType::mapped_type>
auto makeMapInt2StringProduct(const std::vector<std::tuple<MapType, ValueType, ValueType>> &vecMaps, bool useMapSizeAsMultFactor=true) -> MapType {
  auto insertEntry = [](auto &newCont, auto &&val) {newCont.insert(val);};
  auto makeMap = [](const auto &entry) {
    const auto &mapEntry = std::get<0>(entry);
    const auto &prefix = std::get<1>(entry);
    const auto &suffix = std::get<2>(entry);
    MapType mapResult{};
    std::transform(mapEntry.cbegin(),mapEntry.cend(),std::inserter(mapResult, mapResult.end()),
    [&prefix, &suffix](auto &&entry) -> typename MapType::value_type {
      return {entry.first, prefix + entry.second + suffix};
    });
    return mapResult;
  };
  int multFactor = 0;
  auto makeNewEntry = [&multFactor] (auto &&entry1, auto &&entry2) -> typename MapType::value_type {
    return { multFactor * entry1.first + entry2.first, entry1.second + entry2.second};
  };
  if(vecMaps.size()>0) {
    MapType mapResult{};
    bool firstIter=true;
    for(const auto &entry: vecMaps) {
      const auto mapPrepared = makeMap(entry);
      if(firstIter) {
        mapResult = mapPrepared;
        firstIter=false;
        continue;
      }
      const auto lastPosSecondMap = mapPrepared.size() > 0 ? (--mapPrepared.end())->first : 0;
      multFactor = (useMapSizeAsMultFactor && mapPrepared.size() > 0) ? mapPrepared.size() : (lastPosSecondMap + 1);// captured by makeNewEntry
      mapResult = makeContainerProduct(insertEntry, makeNewEntry ,mapResult, mapPrepared);
    }
    return mapResult;
  }
  else {
    return {};
  }
}

}
}
#endif
