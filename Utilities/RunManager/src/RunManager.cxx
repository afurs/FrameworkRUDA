#include "RunManagerRUDA/RunManager.h"
namespace utilities {
namespace run_manager{

void RunConditionTable::connectTree(TTree *treeInput)  {
  if(treeInput==nullptr) {
    std::cout<<"\nWARNING! NO TREE PTR!\n";
    return;
  }
  treeInput->SetBranchAddress("raw_run",&mRunNum);
  treeInput->SetBranchAddress("energy",&mEnergy);
  treeInput->SetBranchAddress("fillno",&mFill);
  treeInput->SetBranchAddress("filling_config",&mFillingConfig);
  treeInput->SetBranchAddress("mu",&mMu);
  treeInput->SetBranchAddress("rate",&mRate);
  treeInput->SetBranchAddress("interacting_bunches",&mIntBunches);
  treeInput->SetBranchAddress("t00_value",&mStatusT00);
  treeInput->SetBranchAddress("v00_value",&mStatusV00);
}
void RunConditionTable::print() const {
  std::cout<<"\n====================================";
  std::cout<<"\nRun number: "<<mRunNum;
  std::cout<<"\nEnergy: "<<mEnergy;
  std::cout<<"\nFill: "<<mFill;
  std::cout<<"\nFilling config: "<<mFillingConfig;
  std::cout<<"\nMu: "<<mMu;
  std::cout<<"\nRate: "<<mRate;
  std::cout<<"\nInteracting bunches: "<<mIntBunches;
  std::cout<<"\nTZERO status: "<<mStatusT00;
  std::cout<<"\nVZERO status: "<<mStatusV00;
  std::cout<<"\n====================================\n";
}
std::vector<RunConditionTable> RunConditionTable::getVecRCT(std::string filepath) {
  TFile tf(filepath.c_str(),"READ");
  TTree *tr = (TTree *)tf.Get("RCT");
  RunConditionTable entryRCT{};
  entryRCT.connectTree(tr);
  std::vector<RunConditionTable> vecResult;
  for(int iEntry=0;iEntry<tr->GetEntries();iEntry++) {
    tr->GetEntry(iEntry);
    vecResult.push_back(entryRCT);
  }
  return vecResult;
}

std::map<std::string,std::set<unsigned int>> RunConditionTable::makeMapPeriod2Runs(
    const std::map<std::string, std::string> &mapPeriod2Filepaths) {
  std::map<std::string,std::set<unsigned int>> mapPeriod2Run;
  for(const auto&entry:mapPeriod2Filepaths) {
    std::string periodName = entry.first;
    auto vecRCT = getVecRCT(entry.second);
    auto pairInserted = mapPeriod2Run.insert({periodName,{}});
    for(const auto &entryRCT: vecRCT) {
      pairInserted.first->second.insert(static_cast<unsigned int>(entryRCT.mRunNum));
    }
  }
  return mapPeriod2Run;
}
int RunConditionTable::getYear(unsigned int runnum) {
  if(208365<=runnum&&runnum<=247170)	return 2015;
  if(247171<=runnum&&runnum<=267254)	return 2016;
  if(267258<=runnum&&runnum<=282900)	return 2017;
  if(282901<=runnum&&runnum<300000)	return 2018;
  return 0;
}
int RunConditionTable::getFromPeriod(std::string period) {
  return int{2000+std::stoi(period.substr(3,2))};
}
}
}
