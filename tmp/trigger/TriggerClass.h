#ifndef TriggerClass_h
#define TriggerClass_h
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <regex>

#include "TString.h"
#include "TMap.h"
#include "TObjString.h"
#include "TTree.h"
#include "TFile.h"

#include "AnalysisUtils.h"
struct TriggerClass {
  //Full trigger word;
  std::string mTriggerClassName;
  //Trigger title(for hists and other namings)
  std::string mTriggerClassTitle;
  //Trigger mask corresponding to current trigger name;
  //std::bitset<NBITS> mTriggerMaskBits;
  //Downscale factor
  double mDownscaleFactor;
  unsigned long long mTriggerMask;
  unsigned long long mTriggerMaskNext50;
  bool checkTriggerMask(unsigned long long triggerMask,unsigned long long triggerMaskNext50=0)  const;
  void print(bool csvFormat=false, unsigned int runnum=0) const;
};
struct TriggerClassManager {
  TriggerClassManager(unsigned int runnum,std::string filepaths):mRunNum(runnum),mFilepath(filepaths){}
  TriggerClassManager()=default;
  unsigned int mRunNum;
  std::string mFilepath;
  std::map<std::string,TriggerClass> mMapTrgNameToTrgClass;
  bool checkTriggerMask(std::string trgClassName,unsigned long long triggerMask,unsigned long long triggerMaskNext50=0) const;
  bool defineTrgSum(std::string name, std::string title, std::vector<std::string> vecKeywords, bool keywordExact);
  std::size_t defineTrgSumReg(std::string name, std::string title, std::string regExprSt);
  void init();
  void init(TTree *treeTrgClasses);
  void print(bool csvFormat=false) const;
  bool isReady() const;
};

#endif
