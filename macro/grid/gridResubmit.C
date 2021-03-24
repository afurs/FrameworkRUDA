
#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libGridRUDA.so)

#endif
#include <string>
#include <vector>
#include "JobObject.h"

//void gridResubmit(std::string st="") {
void gridResubmit(std::string statusWord = "ERROR_V",std::size_t nMaxJobs=10000) {

  
  gInterpreter->Load("libGridRUDA.so");
  auto vecJobs = JobObject::accumulateAllJobs();
  std::string command;
  if(statusWord=="ERROR_SPLT") {
   for(const auto &entry: vecJobs) {
     if(entry.mStatus=="ERROR_SPLT"&&!entry.isSubJob()) {
        command="resubmit ";
        command+=entry.mID;
        gGrid->Command(TString{command});
     }
   }
   vecJobs.clear();
   vecJobs.resize(0);
   return;
  }
  auto setSubJobs = JobObject::getSetOfSubjobs(vecJobs,statusWord);
  std::cout<<"\nStatus: "<<statusWord;
  std::cout<<"\nNumber of Subjobs: "<<setSubJobs.size();
  std::string IDs="";
  std::cout<<std::endl;
  std::size_t nJobs=1;
  for(const auto &id:setSubJobs) {
    if(nJobs>nMaxJobs) break;
    command = "resubmit ";
    command+=id;
    IDs+=id;
    IDs+=" ";
    //cout<<endl<<TString{command}<<endl;
    gGrid->Command(TString{command});
    nJobs++;
  }
  vecJobs.clear();
  setSubJobs.clear();
  vecJobs.resize(0);
 // setSubJobs.resize(0);
  //std::cout<<std::endl<<IDs<<std::endl;
 
}
