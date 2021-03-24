
#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libGridRUDA.so)

#endif
#include <string>
#include <vector>
#include "JobObject.h"

//void gridResubmit(std::string st="") {
void gridFull() {
  auto vecJobs = JobObject::accumulateAllJobs();
/*
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
*/
  std::map<std::string,unsigned int> mapStats;

  for(const auto& errStatusName: (JobObject::sVecErrorStatusNames)) 
  {
    mapStats.insert({errStatusName,0});
  }
  
  for(auto &entry: mapStats) {
    auto nEntries = std::count_if(vecJobs.begin(),vecJobs.end(),[&entry](auto &elem)->bool {return (elem.mStatus==entry.first)&&(elem.isSubJob());});
    entry.second+=nEntries;
    std::cout<<"\n"<<entry.first<<": "<<entry.second;
  }
  unsigned int totalError=0;
  unsigned int totalDone=0;
  unsigned int totalErrorSplt=0;
  for(const auto& entry: vecJobs) {
    if(entry.mStatus=="ERROR_SPLT") {
      std::string command = "resubmit ";
      command+=entry.mID;
      totalErrorSplt++;
      gGrid->Command(TString{command});
    }

    if(entry.isMasterJob()) continue;
    if(mapStats.find(entry.mStatus)!=mapStats.end()) {
      std::string command = "resubmit ";
      command+=entry.mID;
      totalError++;
      //cout<<endl<<command<<endl;
      gGrid->Command(TString{command});
    }
    if(entry.mStatus=="DONE") {
      std::string command = "kill ";
      command+=entry.mID;
      totalDone++;
      gGrid->Command(TString{command});
    }
  }
  cout<<"\nTotal error subjobs: "<<totalError;
  cout<<"\nTotal error-split subjobs: "<<totalErrorSplt;

  cout<<"\nTotal \"DONE\" subjobs: "<<totalDone;

/*
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
 */
  std::cout<<std::endl;
}
