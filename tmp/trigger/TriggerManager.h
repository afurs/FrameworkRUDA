#ifndef TriggerManager_h
#define TriggerManager_h
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string>

#include "TString.h"
#include "TMap.h"
#include "TObjString.h"
#include "TGrid.h"
#include "TTree.h"

#include "AliTriggerClass.h"
#include "AliCDBManager.h"
#include "AliTriggerConfiguration.h"
#include "AliCentralTrigger.h"

class TriggerManager
{
public:
  TriggerManager();
  ~TriggerManager();
//////Setter functions
////Getter functions
  AliCDBManager* connectOCDB(unsigned int runNum);
  ULong64_t getTriggerMask(TString triggerClass,Double_t *downscaleFactor=NULL,Bool_t *isNext50=NULL);
  ULong64_t getTriggerMask(TList *listTrig,Double_t *downscaleFactor=NULL,Bool_t *isNext50=NULL);
  void setRunNum(unsigned int runNum)	{mRunNum = runNum;}
  void setTreeTrgClasses(TTree *treeTrgClasses)	{mTreeTrgClasses=treeTrgClasses;}
  void fillVecTrgClass() {
    fillVecTrgClass(mRunNum,mTreeTrgClasses);
  }
  void makeTreeTrgClasses(std::vector<unsigned int> &vecRunNums, TTree &treeTrgClasses);
  void fillVecTrgClass(unsigned int runnum,TTree *treeTrgClasses);
/////////////////
  TTree *mTreeTrgClasses;
  unsigned int mRunNum;
  std::vector<TriggerClass> mVecTrgClasses;
  std::vector<std::string> mVecTrgNames;
//  std::map<unsigned int,std::vector<TriggerClass>> mMapAllTrgClasses;
  std::vector<std::string> mVecAllTrgNames;
  void reprocessTree(std::string pathToSrc,std::string pathToDest);
};

#endif
