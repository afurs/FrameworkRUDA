#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libGridRUDA.so)
#endif
#include "JobObject.h"
void gridPrint() {
  auto vecJobs = JobObject::accumulateAllJobs();
  std::map<std::string,unsigned int> mapSubJobsStatus;
  std::map<std::string,unsigned int> mapMasterJobsStatus;
  for(const auto& entry: vecJobs) {
    if(entry.isSubJob()) {
      auto pairRes = mapSubJobsStatus.insert({entry.mStatus,1});
      if(!pairRes.second) ++(*(pairRes.first)).second;
    }
    else {
      auto pairRes = mapMasterJobsStatus.insert({entry.mStatus,1});
      if(!pairRes.second) ++(*(pairRes.first)).second;

    }
  }
  std::cout<<"\n=======MASTER JOBS=======";
  for(const auto &entry: mapMasterJobsStatus) std::cout<<"\n"<<entry.first<<": "<<entry.second;
  std::cout<<"\n=======SUB JOBS==========";
  for(const auto &entry: mapSubJobsStatus) std::cout<<"\n"<<entry.first<<": "<<entry.second;
  std::cout<<std::endl;

}