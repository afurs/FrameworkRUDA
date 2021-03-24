#include <set>
#include <string>

#include "AliExternalInfo.h"

void makeRCTtrees() {
  std::set<std::string> setPeriodNames2018 = {"LHC18a","LHC18b","LHC18c","LHC18d","LHC18e","LHC18f","LHC18g","LHC18h",
                                             "LHC18i","LHC18j","LHC18k","LHC18l","LHC18m","LHC18n","LHC18o","LHC18p",
                                             "LHC18q","LHC18r"};
  std::set<std::string> setPeriodNames2017 = {//"LHC17a","LHC17b","LHC17c","LHC17d","LHC17e","LHC17f","LHC17g","LHC17h",
                                             "LHC17i"//,"LHC17j","LHC17k","LHC17l","LHC17m","LHC17n","LHC17o","LHC17p",
                                             /*"LHC17q","LHC17r"*/};

  std::set<std::string> setPeriodNames2016 = {"LHC16b","LHC16c","LHC16d","LHC16e","LHC16f","LHC16g","LHC16h",
                                             "LHC16i","LHC16j","LHC16k","LHC16l","LHC16m","LHC16n","LHC16o","LHC16p",
                                             "LHC16q","LHC16r","LHC16s","LHC16t"};

  AliExternalInfo extInfo;
  for(const auto& entry: setPeriodNames2016) {
    TTree *tr = extInfo.GetTreeRCT(entry.c_str(),"pass1");
  }
}
