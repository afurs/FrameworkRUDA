#include "JobObject.h"
/*******************************************************************************************************************/
JobObject::JobObject ():mFieldMap({{"split",std::ref(mSplit)}
                       ,{"status",std::ref(mStatus)}
                       ,{"name",std::ref(mName)}
                       ,{"owner ",std::ref(mOwner)}
                       ,{"id ",std::ref(mID)}
                       ,{"priority",std::ref(mPriority)}
                     }) {}
/*******************************************************************************************************************/
JobObject::JobObject(const JobObject&jo):mFieldMap({{"split",std::ref(mSplit)}
                                        ,{"status",std::ref(mStatus)}
                                        ,{"name",std::ref(mName)}
                                        ,{"owner ",std::ref(mOwner)}
                                        ,{"id ",std::ref(mID)}
                                        ,{"priority",std::ref(mPriority)}
                                      })
{
  mSplit=jo.mSplit;
  mStatus=jo.mStatus;
  mName=jo.mName;
  mOwner=jo.mOwner;
  mID=jo.mID;
  mPriority=jo.mPriority;
}
/*******************************************************************************************************************/
void JobObject::print() const {
  std::cout<<"\n###################################################\n";
  for(const auto&entry:mFieldMap) {
    std::cout<<entry.first<<" | "<<entry.second.get()<<std::endl;
  }
}
/*******************************************************************************************************************/
void JobObject::accumulateAllJobs(std::vector<JobObject> &vecResult) {
  if(!gGrid) TGrid::Connect("alien://");
  TGrid *alien = (TGrid *)gGrid;
  TGridResult *alienResult = gGrid->Command("ps -A");
  for(const auto &entryMap:(*alienResult)) {//iterating over jobs/subjobs
    TMap *mapData = dynamic_cast<TMap *>(entryMap);
    if(mapData==nullptr) {
      std::cout<<"\nINCORRECT OBJECT TYPE!\n";
      continue;
    }
    JobObject job;
    for(const auto &entryPair:(*mapData)) {//iterating over fields for given job/subjob
      TPair *fieldPair = (TPair *)entryPair;
      TObjString *objstKey = dynamic_cast<TObjString *>(fieldPair->Key());
      if(objstKey==nullptr) {
        std::cout<<"\nWarning! Incorrect key type in TMap!\n";
        continue;
      }
      auto itPair = job.mFieldMap.find(std::string{objstKey->GetString()});

      if(itPair==job.mFieldMap.end()) {
        std::cout<<"\nWarning! Incorrect TMap field: |"<<objstKey->GetString()<<"|\n";
        continue;
      }
      TObjString *objstValue = dynamic_cast<TObjString *>(fieldPair->Value());
      if(objstValue==nullptr) {
        std::cout<<"\nWarning! Incorrect value type in TMap!\n";
        continue;
      }
      itPair->second.get() = std::string{objstValue->GetString()};
    }
    vecResult.push_back(job);
  }
  //for(const auto&entry:vecResult) entry.print();
}
/*******************************************************************************************************************/
std::vector<JobObject> JobObject::accumulateAllJobs() {
  std::vector<JobObject> vecResult;
  accumulateAllJobs(vecResult);
  return std::move(vecResult);
}
/*******************************************************************************************************************/
std::set<std::string> JobObject::getSetOfStatus(const std::vector<JobObject> &vecJO) {
  std::set<std::string> setOfStatus;
  for(const auto &entry:vecJO) setOfStatus.insert(entry.mStatus);
  return setOfStatus;
}
/*******************************************************************************************************************/
std::set<std::string> JobObject::getSetOfSubjobs(const std::vector<JobObject> &vecJO,std::string statusWord) {
  std::set<std::string> setSubjos;
  for(const auto &entry:vecJO) {
    if(entry.mStatus==statusWord&&entry.isSubJob()) setSubjos.insert(entry.mID);
  }
  return setSubjos;
}
