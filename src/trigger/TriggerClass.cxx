#include "TriggerClass.h"

/*******************************************************************************************************************/
bool TriggerClass::checkTriggerMask(unsigned long long triggerMask,unsigned long long triggerMaskNext50)  const{
  return (mTriggerMask&triggerMask)||(mTriggerMaskNext50&triggerMaskNext50);
}
/*******************************************************************************************************************/
void TriggerClass::print(bool csvFormat) const{
  if(csvFormat) {
    //mRunNum;mTriggerClassName;mTriggerMask;mTriggerMaskNext50;mDownscaleFactor
    std::cout<<mRunNum<<";"<<mTriggerClassName<<";"<<mTriggerMask<<";"<<mTriggerMaskNext50<<";"<<mDownscaleFactor<<std::endl;
  }
  else {
    std::cout<<"\n========TriggerClass========";
    std::cout<<"\nRun number: "<<mRunNum;
    std::cout<<"\nName: "<<mTriggerClassName;
    std::cout<<std::hex<<"\nTriggerMask: 0x"<<mTriggerMask;
    std::cout<<"\nTriggerMaskNext50: 0x"<<mTriggerMaskNext50;
    std::cout<<std::dec<<"\nDownscale factor: "<<mDownscaleFactor;
    std::cout<<std::endl;
  }

}
/*******************************************************************************************************************/
//TriggerClassManager
/*******************************************************************************************************************/
bool TriggerClassManager::checkTriggerMask(std::string trgClassName,unsigned long long triggerMask,unsigned long long triggerMaskNext50) const{
  auto it = mMapTrgNameToTrgClass.find(trgClassName);
  if(it==mMapTrgNameToTrgClass.end()) {
   // cout<<endl<<"Warning! Cannot find trigger "<<trgClassName<<endl;
    return false;
  }
  else {
    return it->second.checkTriggerMask(triggerMask,triggerMaskNext50);
  }
}
/*******************************************************************************************************************/

/*******************************************************************************************************************/
void TriggerClassManager::init(unsigned int runnum,TTree *treeTrgClasses) {
  //TriggerManager trgManager;
  //trgManager.fillVecTrgClass(runnum,treeTrgClasses);
  mMapTrgNameToTrgClass.clear();
  unsigned int brRunNum;
  //std::cout<<"\nNent: "<<treeTrgClasses->GetEntries()<<endl;
  treeTrgClasses->SetBranchAddress("mRun",&brRunNum);
  for(int iEvent=0;iEvent<treeTrgClasses->GetEntries();iEvent++) {
    ULong64_t brTrgMask,brTrgMaskNext50;
    TObjString trgClassName;
    TObjString *brTrgClassName=&trgClassName;
    double brDownscaleFactor;
    TTree trTrgCls;
    TTree *brTrTrgCls = &trTrgCls;
    treeTrgClasses->SetBranchAddress("mTreeTrgClasses",&brTrTrgCls);
    treeTrgClasses->GetEntry(iEvent);
    brTrTrgCls->SetBranchAddress("mTrgClassName",&brTrgClassName);
    brTrTrgCls->SetBranchAddress("mTrgMask",&brTrgMask);
    brTrTrgCls->SetBranchAddress("mTrgMaskNext50",&brTrgMaskNext50);
    brTrTrgCls->SetBranchAddress("mDownscaleFactor",&brDownscaleFactor);
    //std::cout<<"\nRunNum:"<<brRunNum<<" | Nent: "<<brTrTrgCls->GetEntries()<<endl;
    if(brRunNum!=runnum) continue;
    for(int iEventTrg=0;iEventTrg<brTrTrgCls->GetEntries();iEventTrg++) {
      brTrTrgCls->GetEntry(iEventTrg);
      mMapTrgNameToTrgClass.insert({std::string(brTrgClassName->GetString())
                                    ,TriggerClass{std::string(brTrgClassName->GetString()),
                                    std::string(brTrgClassName->GetString()),brDownscaleFactor,brTrgMask,brTrgMaskNext50,runnum}});
//      cout<<endl<<brTrgClassName->GetString()<<endl;
//      cout<<endl<<brTrgMask<<"|"<<brTrgMaskNext50<<"|"<<brDownscaleFactor<<endl;
    }
  }
  if(mMapTrgNameToTrgClass.size()==0) std::cout<<"\nWARNING! NO TRIGGERS CLASSES DETECTED!\n";
}
/*******************************************************************************************************************/
void TriggerClassManager::print(bool csvFormat) const{
  if(csvFormat) {
    std::cout<<"\nmRunNum;mTriggerClassName;mTriggerMask;mTriggerMaskNext50;mDownscaleFactor";
    for(const auto &entry:mMapTrgNameToTrgClass) {
      entry.second.print(csvFormat);
    }
  }
  else {
    /*
    std::cout<<"\nTriggers: \n";
    for(const auto &entry:mMapTrgNameToTrgClass) {
      std::cout<<entry.first<<"|";
      std::cout<<std::hex<<entry.second.mTriggerMask<<"|"<<entry.second.mTriggerMaskNext50<<std::dec<<std::endl;
    }
    */
    for(const auto &entry:mMapTrgNameToTrgClass) {
      entry.second.print(csvFormat);
    }
  }
  std::cout<<"\nNumber of triggers: "<<mMapTrgNameToTrgClass.size()<<std::endl;
}
/*******************************************************************************************************************/
bool TriggerClassManager::isReady() const {
  return mMapTrgNameToTrgClass.size()!=0;
}
