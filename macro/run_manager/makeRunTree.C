#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libboost_filesystem.so)
//R__LOAD_LIBRARY(libAnalysisRUDA.so)
R__LOAD_LIBRARY(libCommonRUDA.so)
R__LOAD_LIBRARY(libRunManagerRUDA.so)
#endif
//#include
#include "RunManager.h"
#include "AnalysisUtils.h"
void makeRunTree() {
  std::string pathToLogbook = gSystem->Getenv("ALICE_LOGBOOK_PATH");
  pathToLogbook+="/trees/RCT/data/2018";
  std::string filenameOutput = "runlistRCT2018_pp.root";
  auto runFilter = [](const utilities::run_manager::RunConditionTable &entry)->bool {
    TString fillingScheme{entry.mFillingConfig};
    if(entry.mStatusT00==1
       && entry.mIntBunches>0
       && !(fillingScheme.Contains("Pb")||fillingScheme.Contains("pb")||fillingScheme.Contains("Xe"))
       && entry.mEnergy>6000 ) {
      return true;
    }
    return false;
  };

  auto vecFilepaths = utilities::AnalysisUtils::makeVecFilepathsROOT(pathToLogbook);
  TTree *treeOutput = new TTree("RCT","RCT");
  Int_t fRunNum;
  const std::string brName = "raw_run";
  treeOutput->Branch(brName.c_str(),&fRunNum);
  treeOutput->SetDirectory(0);
  std::set<Int_t> setRunNums;
  for(const auto &enFilepaths: vecFilepaths) {
    auto vectorRuns = utilities::run_manager::RunConditionTable::getVecRCT(enFilepaths);
    for(const auto& entryRun: vectorRuns) {
      if(runFilter(entryRun)) {
        setRunNums.insert(entryRun.mRunNum);
        entryRun.print();
      }
    }
  }

  for(const auto& runnum: setRunNums) {
    fRunNum=runnum;
    treeOutput->Fill();
  }
  std::cout<<std::endl;
  if(setRunNums.size()!=0) {
    std::cout<<"First run: "<<*setRunNums.begin()<<std::endl;
    std::cout<<"Last run: "<<*(--setRunNums.end())<<std::endl;
  }
  std::cout<<"Total runs: "<<setRunNums.size()<<std::endl;
  TFile *fileOutput = new TFile(filenameOutput.c_str(),"RECREATE");
  fileOutput->WriteObject(treeOutput,treeOutput->GetName(),"SingleKey");
  fileOutput->Close();
  delete fileOutput;
}
