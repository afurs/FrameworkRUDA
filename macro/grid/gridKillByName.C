#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/inluce)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libGridRUDA.so)
#endif
#include "JobObject.h"
void gridKillByName(std::string jobName) {
  auto vecJobs = JobObject::accumulateAllJobs();
  std::string command;
/*
  if(statusWord=="ERROR_SPLT") {
   for(const auto &entry: vecJobs) {
     if(entry.mStatus=="ERROR_SPLT"&&!entry.isSubJob()) {
        command="resubmit ";
        command+=entry.mID;
        gGrid->Command(TString{command});
     }
   }
   return;
  }
*/
  std::set<std::string> setJobs;
  for(const auto &entry:vecJobs) {
  //  if(TString{entry.mName}.Contains(TString{jobName})&&entry.mSplit=="0")cout<<endl<<"|"<<entry.mStatus<<"|"<<endl;
    if(TString{entry.mName}.Contains(TString{jobName})&&entry.mSplit=="0"&&entry.mStatus!="KILLED") setJobs.insert(entry.mID);
  }
  cout<<endl<<"Name: "<<jobName;
  cout<<endl<<"Number of Jobs+Subjobs: "<<setJobs.size();

  cout<<endl;
  for(const auto &id:setJobs) {
    command = "kill ";
    command+=id;
    //cout<<endl<<TString{command}<<endl;
    gGrid->Command(TString{command});
  }
  vecJobs.clear();
  setJobs.clear();

  vecJobs.resize(0);
  //setSubJobs.resize(0);
}
