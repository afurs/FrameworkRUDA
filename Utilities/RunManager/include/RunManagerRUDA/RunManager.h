#ifndef RunManager_h
#define RunManager_h
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>

#include "TTree.h"
#include "TFile.h"

namespace utilities {
namespace run_manager {

class RunConditionTable {
 public:
  Int_t mRunNum;
  Int_t mFill;
  Double_t mEnergy;
  Double_t mMu;
  Double_t mRate;
  Int_t mIntBunches;
  Int_t mStatusT00;
  Int_t mStatusV00;
  Char_t mFillingConfig[300];
  void connectTree(TTree *treeInput);
  void print() const;
  static std::vector<RunConditionTable> getVecRCT(std::string filepath);
  static std::map<std::string,std::set<unsigned int>> makeMapPeriod2Runs(
      const std::map<std::string, std::string> &mapPeriod2Filepaths);
  static int getYear(unsigned int runnum);
  static int getFromPeriod(std::string period);
};

} //run_manager
}//utilities
#endif
