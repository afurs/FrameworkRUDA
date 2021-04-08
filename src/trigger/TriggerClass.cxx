#include "TriggerClass.h"

/*******************************************************************************************************************/
bool TriggerClass::checkTriggerMask(unsigned long long triggerMask,unsigned long long triggerMaskNext50)  const{
  return (mTriggerMask&triggerMask)||(mTriggerMaskNext50&triggerMaskNext50);
}
/*******************************************************************************************************************/
void TriggerClass::print(bool csvFormat,unsigned int runnum) const{
  if(csvFormat) {
    //mRunNum;mTriggerClassName;mTriggerMask;mTriggerMaskNext50;mDownscaleFactor
    if(runnum!=0)std::cout<<runnum<<";";
    std::cout<<mTriggerClassName<<";"<<mTriggerMask<<";"<<mTriggerMaskNext50<<";"<<mDownscaleFactor<<std::endl;

  }
  else {
    std::cout<<"\n========TriggerClass========";
    if(runnum!=0)std::cout<<"\nRun number: "<<runnum;
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
bool TriggerClassManager::defineTrgSum(std::string name, std::string title
                                       , std::vector<std::string> vecKeywords
                                       , bool keywordExact) {
  TriggerClass trgClass{name,title,1.,0,0};
  decltype(trgClass.mTriggerMask) trgMask=0;
  decltype(trgClass.mTriggerMaskNext50) trgMaskNext50=0;
  //Checking for keywords presence in trg names
  for(const auto &keyword: vecKeywords) {
    auto it = std::find_if(mMapTrgNameToTrgClass.begin()
              ,mMapTrgNameToTrgClass.end()
              ,[&keywordExact,&keyword](const auto&entry)->bool{
      if(keywordExact) {
        return entry.second.mTriggerClassName==keyword;
      }
      else{
        return entry.second.mTriggerClassName.find(keyword)!=std::string::npos;
      }
    }
    );
    if(it==mMapTrgNameToTrgClass.end()) {
      std::cout<<"\nCannot find match for keyword \""<<keyword<<"\" !\n";
      return false;
    }
  }
  std::vector<std::string> vecMatches;
  for(const auto &keyword:vecKeywords) {
    for(const auto &entry: mMapTrgNameToTrgClass) {
      const auto &trgClassEntry = entry.second;
      if(keywordExact && trgClassEntry.mTriggerClassName==keyword) {
        vecMatches.push_back(trgClassEntry.mTriggerClassName);
        trgMask|=trgClassEntry.mTriggerMask;
        trgMaskNext50|=trgClassEntry.mTriggerMaskNext50;
      }
      else if(!keywordExact && trgClassEntry.mTriggerClassName.find(keyword)!=std::string::npos) {
        vecMatches.push_back(trgClassEntry.mTriggerClassName);
        trgMask|=trgClassEntry.mTriggerMask;
        trgMaskNext50|=trgClassEntry.mTriggerMaskNext50;
      }
    }
  }
  std::cout<<"\n-----------------------------------------";
  std::cout<<"\nAdding custom trigger";
  std::cout<<"\nName: "<<name;
  std::cout<<"\nTitle: "<<title;
  std::cout<<"\nKeywords: ";
  for(const auto &keywords: vecKeywords)  std::cout<<keywords<<" ";
  std::cout<<"\nKeyword exact matching? : "<<keywordExact;
  std::cout<<"\nMatches: ";
  for(const auto &keywords: vecMatches)  std::cout<<keywords<<" ";
  std::cout<<"\nTrigger mask: "<<std::hex<<trgMask;
  std::cout<<"\nTrigger mask next 50: "<<std::hex<<trgMaskNext50;
  std::cout<<"\n-----------------------------------------\n";
  return true;
}
/*******************************************************************************************************************/
std::size_t TriggerClassManager::defineTrgSumReg(std::string name, std::string title
                                       , std::string regExprSt) {
  std::regex regExpr{regExprSt};
  TriggerClass trgClass{name,title,1.,0,0};
  decltype(trgClass.mTriggerMask) trgMask=0;
  decltype(trgClass.mTriggerMaskNext50) trgMaskNext50=0;
  std::vector<std::string> vecMatches;
  for(const auto &entry: mMapTrgNameToTrgClass) {
    const auto &trgClassEntry = entry.second;
    if(std::regex_match(trgClassEntry.mTriggerClassName,regExpr)) {
      vecMatches.push_back(trgClassEntry.mTriggerClassName);
      trgMask|=trgClassEntry.mTriggerMask;
      trgMaskNext50|=trgClassEntry.mTriggerMaskNext50;
    }
  }

  if(vecMatches.size()==0) {

    std::cout<<"\nCannot find any match for mask: \""<<regExprSt<<"\"\n";
    return vecMatches.size();
  }
  trgClass.mTriggerMask = trgMask;
  trgClass.mTriggerMaskNext50 = trgMaskNext50;
  mMapTrgNameToTrgClass.insert({trgClass.mTriggerClassName,trgClass});
  std::cout<<"\n-----------------------------------------";
  std::cout<<"\nAdding custom trigger";
  std::cout<<"\nName: "<<name;
  std::cout<<"\nTitle: "<<title;
  std::cout<<"\nKeyword mask: "<<regExprSt;
  std::cout<<"\nMatches: ";
  for(const auto &matches: vecMatches)  std::cout<<matches<<" ";
  std::cout<<"\n\nTrigger mask: "<<std::hex<<trgMask<<std::dec;
  std::cout<<"\nTrigger mask next 50: "<<std::hex<<trgMaskNext50<<std::dec;
  std::cout<<"\n-----------------------------------------\n";
  return vecMatches.size();
}
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
void TriggerClassManager::init()  {
  TFile fileTrgClassesTree(mFilepath.c_str(),"READ");
  TList *listTrgClassesTree = utilities::AnalysisUtils::getListObjFromFile<TTree>(fileTrgClassesTree);
  listTrgClassesTree->SetOwner(kTRUE);
  //AnalysisUtils::getTreesRecursively(&listTrgClassesTree,dynamic_cast<TList *>(fileTrgClassesTree.GetListOfKeys()));
  TTree *treeTrgClasses = (TTree *)listTrgClassesTree->First();
  init(treeTrgClasses);
  fileTrgClassesTree.Close();
  delete listTrgClassesTree;
  if(!isReady()) {
    std::cout<<"\nWARNING! TRIGGER CLASSES ARE NOT READY! ABORTING!\n";
    return;
  }
}
/*******************************************************************************************************************/
void TriggerClassManager::init(TTree *treeTrgClasses) {
  //TriggerManager trgManager;
  //trgManager.fillVecTrgClass(runnum,treeTrgClasses);
  if(mRunNum==0) {
    std::cout<<"\nWARNING! RUN NUMBER IS UNKNOWN!\n";
    return;
  }
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
    if(brRunNum!=mRunNum) continue;
    for(int iEventTrg=0;iEventTrg<brTrTrgCls->GetEntries();iEventTrg++) {
      brTrTrgCls->GetEntry(iEventTrg);
      mMapTrgNameToTrgClass.insert({std::string(brTrgClassName->GetString())
                                    ,TriggerClass{std::string(brTrgClassName->GetString()),
                                    std::string(brTrgClassName->GetString()),brDownscaleFactor,brTrgMask,brTrgMaskNext50}});
//      cout<<endl<<brTrgClassName->GetString()<<endl;
//      cout<<endl<<brTrgMask<<"|"<<brTrgMaskNext50<<"|"<<brDownscaleFactor<<endl;
    }
  }
  if(mMapTrgNameToTrgClass.size()==0) std::cout<<"\nWARNING! NO TRIGGERS CLASSES DETECTED!\n";
}
/*******************************************************************************************************************/
void TriggerClassManager::print(bool csvFormat) const{
  if(csvFormat) {
    std::cout<<std::endl;
    if(mRunNum!=0)  std::cout<<"mRunNum;";
    std::cout<<"mTriggerClassName;mTriggerMask;mTriggerMaskNext50;mDownscaleFactor";
    for(const auto &entry:mMapTrgNameToTrgClass) {
      entry.second.print(csvFormat,mRunNum);
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
      entry.second.print(csvFormat,mRunNum);
    }
  }
  std::cout<<"\nNumber of triggers: "<<mMapTrgNameToTrgClass.size()<<std::endl;
}
/*******************************************************************************************************************/
bool TriggerClassManager::isReady() const {
  return mMapTrgNameToTrgClass.size()!=0;
}
