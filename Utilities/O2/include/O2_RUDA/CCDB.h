#ifndef CCDB_H
#define CCDB_H

#include "DataFormatsParameters/GRPLHCIFData.h"
#include "CCDB/BasicCCDBManager.h"
#include "CommonUtils/NameConf.h"
#include "CommonConstants/LHCConstants.h"

#include <string>
#include <bitset>
#include <map>
#include <set>

namespace utilities {
namespace ccdb {

struct EntryCCDB {
  EntryCCDB(unsigned int runnum,bool SEOR_init = false,bool firstLastOrbit_init = false, bool GRPLHCIFData_init=false):mRunnum(runnum) {
    if(SEOR_init) {
      initStartEndOfRun();
    }
    if(firstLastOrbit_init) {
      initFirstLastOrbit();
    }
    if(GRPLHCIFData_init) {
      initGRPLHCIFData();
    }
  }
  static constexpr const char* sPathCCDB_RunInformation = "RCT/Info/RunInformation";
  static constexpr const char* sPathCCDB_OrbitReset = "CTP/Calib/OrbitReset";
  static constexpr const char* sPathCCDB_GRPLHCIF = "GLO/Config/GRPLHCIF";
  //Fields from CCDB
  unsigned int mRunnum{};
  uint64_t mSOR{};
  uint64_t mEOR{};
  uint32_t mFirstOrbit{};
  uint32_t mLastOrbit{};
  o2::parameters::GRPLHCIFData mGRPLHCIFData{};
  //

  bool initStartEndOfRun() {
    getRunTimeMeta(mRunnum,mSOR,mEOR);
    if(mSOR==0 || mEOR==0) {
      return false;
    }
    else {
      return true;
    }
  }

  bool initFirstLastOrbit() {
    getFirstLastOrbit(mRunnum, mFirstOrbit,mLastOrbit);
    if(mFirstOrbit==0 || mLastOrbit==0) {
      return false;
    }
    else {
      return true;
    }
  }
  bool initGRPLHCIFData() {
      const auto ptrGRPLHCIFData = getGRPLHCIFData(mRunnum);

    if(ptrGRPLHCIFData == nullptr) {
      return false;
    }
    else {
      mGRPLHCIFData = *ptrGRPLHCIFData;
      return true;
    }
  }

  //Static methods
  static void getRunTimeMeta(unsigned int runnum, uint64_t &sor, uint64_t &eor) {
    auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
    const auto &startEndTime = ccdbManager.getRunDuration(runnum);
    sor = startEndTime.first;
    eor = startEndTime.second;
  }
  static void getFirstLastOrbit(unsigned int runnum,uint32_t &firstOrbit, uint32_t &lastOrbit) {
    uint64_t tsSOR{};
    uint64_t tsEOR{};
    getRunTimeMeta(runnum, tsSOR,tsEOR);

    auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
    const auto *ptrOrbitReset = ccdbManager.getForTimeStamp<std::vector<Long64_t> >(sPathCCDB_OrbitReset,tsSOR);
    if(tsSOR==0||tsEOR==0||ptrOrbitReset==nullptr) {
      firstOrbit = 0;
      lastOrbit = 0;
      return;
    }
    const int64_t orbitReset = (*ptrOrbitReset)[0];
    if(orbitReset>0) {
      firstOrbit = (1000 * tsSOR - static_cast<uint64_t>(orbitReset)) / o2::constants::lhc::LHCOrbitMUS;
      lastOrbit = (1000 * tsEOR - static_cast<uint64_t>(orbitReset)) / o2::constants::lhc::LHCOrbitMUS;
    }
    else {
      firstOrbit = 0;
      lastOrbit = 0;
    }
  }
  static const o2::parameters::GRPLHCIFData *getGRPLHCIFData(unsigned int runnum) {
    uint64_t tsSOR{};
    uint64_t tsEOR{};
    getRunTimeMeta(runnum, tsSOR,tsEOR);
    if(tsSOR == 0) {
      return nullptr;
    }
    auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
    const auto *ptrGRPLHCIFData = ccdbManager.getForTimeStamp<o2::parameters::GRPLHCIFData>(sPathCCDB_GRPLHCIF,tsSOR);
    return ptrGRPLHCIFData;
  }
  static std::map<unsigned int,o2::parameters::GRPLHCIFData> getMapRun2GRPLHCIFData(const std::set<unsigned int> &setRunnums) {
    std::map<unsigned int,o2::parameters::GRPLHCIFData> mapResult{};
    for(const auto &runnum: setRunnums) {
      const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData = getGRPLHCIFData(runnum);
      if(ptrGRPLHCIFData!=nullptr) {
        mapResult.insert({runnum,*ptrGRPLHCIFData});
      }
    }
    return mapResult;
  }
  static std::map<unsigned int,utilities::ccdb::EntryCCDB> getMapRun2EntryCCDB(const std::set<unsigned int> &setRunnums,
    bool SEOR_init = false,bool firstLastOrbit_init = false, bool GRPLHCIFData_init = false) {
    std::map<unsigned int,utilities::ccdb::EntryCCDB> mapResult{};
    for(const auto &runnum: setRunnums) {
      mapResult.emplace(runnum,utilities::ccdb::EntryCCDB(runnum,SEOR_init,firstLastOrbit_init,GRPLHCIFData_init));
    }
    return mapResult;
  }
};
} //namespace ccdb
} //namespace utilities

#endif