
#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)

//R__LOAD_LIBRARY(libboost_filesystem.so)
R__LOAD_LIBRARY(libAnalysisRUDA.so)
R__LOAD_LIBRARY(libCommonRUDA.so)
R__LOAD_LIBRARY(libTriggerRUDA.so)
R__LOAD_LIBRARY(libRunManagerRUDA.so)
#include "taskAnalysisFull.C"
#endif
#include "RunManager.h"
#include "AnalysisUtils.h"
#include "TTree.h"
#include "TFile.h"

void taskGrid(std::string pathInputData="") {
  bool isLocal;
  std::string pathToLogbook;
  if(pathInputData=="") {//GRID
    isLocal=false;
  }
  else {//LOCAL
    isLocal=true;
    pathToLogbook = gSystem->Getenv("ALICE_LOGBOOK_PATH");
  }
  std::map<unsigned int,std::vector<std::string>> mapRuns2VecFileaths;
  if(isLocal) {  //LOCAL
    mapRuns2VecFileaths = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputData);
  }
  else {  //GRID
    TGrid::Connect("alien://");
    // Set temporary merging directory to current one
    gSystem->Setenv("TMPDIR", gSystem->pwd());

    // Set temporary compilation directory to current one
    gSystem->SetBuildDir(gSystem->pwd(), kTRUE);

    // Reset existing include path and add current directory first in the search
    gSystem->SetIncludePath("-I.");
    // load base root libraries
    gSystem->Load("libTree");
    gSystem->Load("libGeom");
    gSystem->Load("libVMC");
    gSystem->Load("libPhysics");
    gSystem->Load("libMinuit");
    auto coll = gGrid->OpenCollection("wn.xml");
    while(coll->Next()) {
      auto regRunNum = std::regex{"000[2][\\d]{5}"};
      std::smatch sm;
      std::string filepath = coll->GetTURL();
      bool searchResult = std::regex_search(filepath,sm,regRunNum);
      if(searchResult) {
        auto pairResult = mapRuns2VecFileaths.insert({std::stoi(sm.str()),{}});
        pairResult.first->second.push_back(filepath);
      }
    }
  }
  for(const auto& entry: mapRuns2VecFileaths) {
    cout<<endl<<"\n========================\n"<<"RUN: "<<entry.first<<"\n========================\n";
    for(const auto& fp:entry.second ) cout<<fp<<endl;
  }

  //std::string stTrg = "/home/deliner/work/logbook_alice/trees/TrgClasses/TrgClasses2018.root";
  std::string stTrg;
  std::string stOutput;
  for(const auto& entry: mapRuns2VecFileaths) {
    auto runnum = entry.first;
    auto vecFilesPaths = entry.second;
    int year = utilities::run_manager::RunConditionTable::getYear(runnum);
    if(year==0) {
      cout<<"\nWARNING! INCORRECT YEAR FOR RUN: "<<runnum<<endl;
      continue;
    }
    if(isLocal) {//LOCAL

      stOutput = Form("outputLumiRun%i.root",runnum);
      stTrg = pathToLogbook+Form("/trees/TrgClasses/TrgClasses%i.root",year);
    }
    else { //GRID
      stTrg = Form("TrgClasses%i.root",year);
      //
      stOutput = "output.root";
    }

    taskAnalysisFull(vecFilesPaths,stOutput,stTrg,runnum);
  }

  ofstream out;
  out.open("outputs_valid", ios::out);
  out.close();
}
