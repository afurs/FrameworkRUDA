#ifndef JobObject_h
#define JobObject_h
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <set>

#include "TGrid.h"
#include "TGridResult.h"
#include "TMap.h"
//#include "TPair.h"
#include "TObjString.h"
class JobObject {
 public:
  typedef std::map<std::string,std::reference_wrapper<std::string>> FieldMap_t;
  std::string mSplit;// "split"-"2076330635" equals to zero if masterJob
  std::string mStatus;// "status" - "DONE"
  std::string mName;// "name" - "TaskAmpT0_HarmonicsOverBC100_pass1LHC17j_task.sh"
  std::string mOwner;// "owner " - "afurs"
  std::string mID; // "id " - "2076331093"
  std::string mPriority; // "priority" - "0"

  JobObject ();
  JobObject(const JobObject&jo);
  JobObject& operator=(const JobObject&) = default;
  bool isMasterJob() const {return mSplit=="0";}
  bool isSubJob() const {return mSplit!="0";}
  bool isStatus(std::string statusName) const {return mStatus==statusName;}
  const static std::vector<std::string> sVecStatusNames;
  const static std::vector<std::string> sVecErrorStatusNames;
  const static std::vector<std::string> sVecFieldNames;
  void print() const;
  static void accumulateAllJobs(std::vector<JobObject> &vecResult);
  static std::vector<JobObject> accumulateAllJobs();
  static std::set<std::string> getSetOfStatus(const std::vector<JobObject> &vecJO);
  static std::set<std::string> getSetOfSubjobs(const std::vector<JobObject> &vecJO,std::string statusWord);
 private:
  const FieldMap_t mFieldMap;
};
const std::vector<std::string> JobObject::sVecErrorStatusNames = {
  "ERROR_E"
  ,"ERROR_V"
  ,"ERROR_IB"
  ,"ERROR_SV"
  ,"EXPIRED"
  ,"OVER_WAITING"
  ,"ZOMBIE"
};
const std::vector<std::string> JobObject::sVecStatusNames = {
  "DONE"
  ,"KILLED"
  ,"SPLIT"
  ,"ERROR_SPLT"
  ,"ERROR_E"
  ,"ERROR_V"
  ,"ERROR_IB"
  ,"EXPIRED"
  ,"ERROR_SV"
  ,"OVER_WAITING"
};

#endif
