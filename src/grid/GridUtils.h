#ifndef GridUtils_h
#define GridUtils_h
#include <iostream>


#include "TGrid.h"
#include "TGridResult.h"
#include "TGridJDL.h"
#include "TSystem.h"
#include "TObjString.h"
#include "TXMLEngine.h"
#include "TMap.h"
#include "TList.h"
#include "TEntryList.h"
#include "TObjArray.h"
#include "AnalysisUtils.h"
namespace utilities {
namespace grid {
/*
struct JobObject {
  std::string mSplit;// "split"-"2076330635" equals to zero if masterJob
  std::string mStatus;// "status" - "DONE"
  std::string mName;// "name" - "TaskAmpT0_HarmonicsOverBC100_pass1LHC17j_task.sh"
  std::string mOwner;// "owner " - "afurs"
  std::string mID; // "id " - "2076331093"
  std::string mPriority; // "priority" - "0"
  JobObject () =default;
  JobObject(const JobObject&jo)=default;
  JobObject& operator=(const JobObject&) = default;
  bool isMasterJob() const {return mSplit=="0";}
  bool isSubJob() const {return mSplit!="0";}
  bool isStatus(std::string statusName) const {return mStatus==statusName;}
  const static std::vector<std::string> sVecStatusNames;
  const static std::vector<std::string> sVecFieldNames;
  void print() const;
  static void accumulateAllJobs(std::vector<JobObject> &vecResult);
  static std::vector<JobObject> accumulateAllJobs();
  static std::set<std::string> getSetOfStatus(const std::vector<JobObject> &vecJO);
  static std::set<std::string> getSetOfSubjobs(const std::vector<JobObject> &vecJO,std::string statusWord);
};
*/

struct GridUtils{

  //Create input XML
  //filepathInputMask = "./TaskRatioT0_pass1_2018_LHC18m/TaskRatioT0_pass1_2018_LHC18m_result/ *output.root"
  //filepathResult = "test.xml"
  static TGridResult* makeInputXML(std::string filepathResult,std::string filepathInputMask,bool makeXMLonGridSide=false);
  static TGridResult* makeCollectionXML(std::string pathGrid, std::string filename);
  static void makeLocalInputXML(std::string filepathResult,std::string filepathInputMask);
  //Make map run->dir_path
  static std::map<unsigned int,std::string> makeMapRun2Path(std::string dirpath);
  static std::map<unsigned int, std::string> makeMapRun2Filepath(std::string dirpath);
  static std::vector<std::string> makeListFilepath(std::string dirpath);
  static TList* getCollectionFromXML(TString filename);
};


}//grid
}//utilities
#endif
