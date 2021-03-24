#ifndef TriggerClass_h
#define TriggerClass_h
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string>

#include "TString.h"
#include "TMap.h"
#include "TObjString.h"
#include "TTree.h"

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
  unsigned int mRunNum;
  bool checkTriggerMask(unsigned long long triggerMask,unsigned long long triggerMaskNext50=0)  const;
  void print(bool csvFormat=false) const;
};
struct TriggerClassManager {
  std::map<std::string,TriggerClass> mMapTrgNameToTrgClass;
  bool checkTriggerMask(std::string trgClassName,unsigned long long triggerMask,unsigned long long triggerMaskNext50=0) const;
  void init(unsigned int runnum,TTree *treeTrgClasses);
  void print(bool csvFormat=false) const;
  bool isReady() const;

};

#endif
