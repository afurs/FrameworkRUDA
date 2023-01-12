#ifndef CCDB_H
#define CCDB_H

#include "DataFormatsParameters/GRPLHCIFData.h"

#include <string>
#include <bitset>
#include <map>
#include <set>

namespace utilities {
namespace ccdb {
const o2::parameters::GRPLHCIFData *getGRPLHCIFData(unsigned int runnum) {
  const std::string tsKey = "SOR";
  o2::ccdb::CcdbApi ccdb_api;
  ccdb_api.init(o2::base::NameConf::getCCDBServer());
  const auto headers = ccdb_api.retrieveHeaders("RCT/Info/RunInformation", std::map<std::string, std::string>(),runnum);
  uint64_t ts{};
  const auto &itTimestamp = headers.find(tsKey);
  if(itTimestamp!=headers.end()) {
    ts = std::stoll(itTimestamp->second);
  }
  else {
    return nullptr;
  }
  std::map<std::string, std::string> mapMetadata;
  std::map<std::string, std::string> mapHeader;
  const auto *ptrGRPLHCIFData = ccdb_api.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF",mapMetadata,ts,&mapHeader);
  return ptrGRPLHCIFData;
}

std::map<unsigned int,const o2::parameters::GRPLHCIFData *> getMapRun2GRPLHCIFData(const std::set<unsigned int> &setRunnums) {
  std::map<unsigned int,const o2::parameters::GRPLHCIFData *> mapResult{};
  for(const auto &runnum: setRunnums) {
    const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData = getGRPLHCIFData(runnum);
    if(ptrGRPLHCIFData!=nullptr) {
      mapResult.insert({runnum,ptrGRPLHCIFData});
    }
  }
  return mapResult;

}
} //namespace ccdb
} //namespace utilities

#endif