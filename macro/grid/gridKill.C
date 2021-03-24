#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libGridRUDA.so)
#endif
#include "JobObject.h"
void gridKill(std::string statusWord = "DONE") {
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
  auto setSubJobs = JobObject::getSetOfSubjobs(vecJobs,statusWord);
  cout<<endl<<"Status: "<<statusWord;
  cout<<endl<<"Number of Subjobs: "<<setSubJobs.size();

  cout<<endl;
  for(const auto &id:setSubJobs) {
    command = "kill ";
    command+=id;
    //cout<<endl<<TString{command}<<endl;
    gGrid->Command(TString{command});
  }
  vecJobs.clear();
  setSubJobs.clear();

  vecJobs.resize(0);
  //setSubJobs.resize(0);
}