#ifndef CCDB_H
#define CCDB_H

#include "DataFormatsParameters/GRPLHCIFData.h"
#include "CCDB/BasicCCDBManager.h"
#include "CommonUtils/NameConf.h"
#include "CommonConstants/LHCConstants.h"
#include "DataFormatsCTP/Configuration.h"

#include <string>
#include <bitset>
#include <map>
#include <set>
#include <iostream>

namespace utilities {
namespace ccdb {

struct EntryCCDB {
  EntryCCDB(unsigned int runnum,bool SEOR_init = false,bool firstLastOrbit_init = false, bool GRPLHCIFData_init=false,bool CTPConfig_init=false):mRunnum(runnum) {
    if(SEOR_init) {
      initStartEndOfRun();
    }
    if(firstLastOrbit_init) {
      initFirstLastOrbit();
    }
    if(GRPLHCIFData_init) {
      initGRPLHCIFData();
    }
    if(CTPConfig_init) {
      initCTPConfiguration();
    }
  }
  static constexpr const char* sPathCCDB_RunInformation = "RCT/Info/RunInformation";
  static constexpr const char* sPathCCDB_OrbitReset = "CTP/Calib/OrbitReset";
  static constexpr const char* sPathCCDB_CTPConfiguration = "CTP/Config/Config";
  static constexpr const char* sPathCCDB_GRPLHCIF = "GLO/Config/GRPLHCIF";
  //Fields from CCDB
  unsigned int mRunnum{};
  long mSOR{-1};
  long mEOR{};
  uint32_t mFirstOrbit{};
  uint32_t mLastOrbit{};
  o2::parameters::GRPLHCIFData mGRPLHCIFData{};
  o2::ctp::CTPConfiguration mCTPConfiguration{};
  //

  bool initStartEndOfRun() {
    getRunTimeMeta(mRunnum,mSOR,mEOR);
    if(mSOR==-1 || mEOR==0) {
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
    const auto ptrGRPLHCIFData = getGRPLHCIFData(mRunnum,mSOR);
    if(ptrGRPLHCIFData == nullptr) {
      std::cout<<"\nError! There are no GRPLHCIFData object for run #"<<mRunnum<<std::endl;
      return false;
    }
    else {
      mGRPLHCIFData = *ptrGRPLHCIFData;
      return true;
    }
  }
  bool initCTPConfiguration() {
    const auto ptrCTPConfiguration = getCTPConfiguration(mRunnum,mSOR);
    if(ptrCTPConfiguration == nullptr) {
      std::cout<<"\nError! There are no GRPLHCIFData object for run #"<<mRunnum<<std::endl;
      return false;
    }
    else {
      mCTPConfiguration = *ptrCTPConfiguration;
      return true;
    }

    
  }

  //Static methods
  static void getRunTimeMeta(unsigned int runnum, long &sor, long &eor) {
    auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
    const auto &startEndTime = ccdbManager.getRunDuration(runnum);
    sor = startEndTime.first;
    eor = startEndTime.second;
  }
  static void getFirstLastOrbit(unsigned int runnum,uint32_t &firstOrbit, uint32_t &lastOrbit) {
    long tsSOR{};
    long tsEOR{};
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
  static const o2::parameters::GRPLHCIFData *getGRPLHCIFData(unsigned int runnum, long tsSOR) {
    //getRunTimeMeta(runnum, tsSOR,tsEOR);
    //if(tsSOR == 0) {
      //return nullptr;
    //}
    if(tsSOR==-1) {
      std::cout<<"\n WARNING! Fetching GRPLHCIFData with current timestamp!\n";
    }
    auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
    const auto *ptrGRPLHCIFData = ccdbManager.getForTimeStamp<o2::parameters::GRPLHCIFData>(sPathCCDB_GRPLHCIF,tsSOR);
    return ptrGRPLHCIFData;
  }

  static const o2::ctp::CTPConfiguration *getCTPConfiguration(unsigned int runnum, long tsSOR) {
    if(tsSOR==-1) {
      std::cout<<"\n WARNING! Fetching CTPConfiguration with current timestamp!\n";
    }
    auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
    const auto *ptrCTPConfiguration = ccdbManager.getForTimeStamp<o2::ctp::CTPConfiguration>(sPathCCDB_CTPConfiguration,tsSOR);
    return ptrCTPConfiguration;
  }

  static std::map<unsigned int,o2::parameters::GRPLHCIFData> getMapRun2GRPLHCIFData(const std::set<unsigned int> &setRunnums) {
    std::map<unsigned int,o2::parameters::GRPLHCIFData> mapResult{};
    for(const auto &runnum: setRunnums) {
      try {
        long sor{-1};
        long eor{};
        getRunTimeMeta(runnum,sor,eor);
        const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData = getGRPLHCIFData(runnum,sor);
        if(ptrGRPLHCIFData!=nullptr) {
          mapResult.insert({runnum,*ptrGRPLHCIFData});
        }
      }
      catch (const std::exception& e) {
        std::cout << e.what();
      }
    }
    return mapResult;
  }
  static std::map<unsigned int,utilities::ccdb::EntryCCDB> getMapRun2EntryCCDB(const std::set<unsigned int> &setRunnums,
    bool SEOR_init = false,bool firstLastOrbit_init = false, bool GRPLHCIFData_init = false,bool CTPConfig_init=false) {
    std::map<unsigned int,utilities::ccdb::EntryCCDB> mapResult{};
    for(const auto &runnum: setRunnums) {
      try {
        mapResult.emplace(runnum,utilities::ccdb::EntryCCDB(runnum,SEOR_init,firstLastOrbit_init,GRPLHCIFData_init,CTPConfig_init));
      }
      catch(const std::exception& e) {
        std::cout << e.what();
        std::cout<<"CANNOT ACCESS CCDB OBJECT FOR RUN "<<runnum<<" !\n";
      }
    }
    return mapResult;
  }
};
} //namespace ccdb
} //namespace utilities

#endif