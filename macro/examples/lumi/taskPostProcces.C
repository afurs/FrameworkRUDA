#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libboost_filesystem.so)
R__LOAD_LIBRARY(libAnalysisRUDA.so)
R__LOAD_LIBRARY(libCommonRUDA.so)
R__LOAD_LIBRARY(libTriggerRUDA.so)
R__LOAD_LIBRARY(libRunManagerRUDA.so)
#endif
#include "TObjectTable.h"
#include "AnalysisManagerBase.h"
#include "HistUtils.h"
#include "AnalysisUtils.h"
#include "TriggerClass.h"
#include "RunManager.h"

void taskPostProcces(
){
  std::string periodName="LHC18p";
  //std::string pathInputData = "/home/deliner/work/data/ageing/2018/LHC18p";
  std::string pathInputData = "/home/deliner/work/data/lumi/result2018_Full/"+periodName;
  std::string pathOutputDir = "resultLumiRatio2018/"+periodName;
  gSystem->mkdir(pathOutputDir.c_str());
  std::string pathOutput = pathOutputDir+"/resultLumiRun%i.root";
  auto mapRunsToFilepaths = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputData);
  //INTERFACE FOR TRIGGER NAMES IN HISTS
  std::vector<std::string> vecTrgNames = {
    "C0TVX"
    ,"CINT7"
    ,"0EMC"
    ,"0MSL"
    ,"0MUL"
  };
  std::vector<std::string> vecTrgClusterNames = {
     "CENTNOTRD"
      ,"CENT"
      ,"FAST"
      ,"MUFAST"
    ,"ALLNOTRD"
  };

  /*
  struct Triggers {
    std::string mTrgNameNum;
    std::string mCINT7;
    std::string mRatioNum_CINT7;
    const std::string histNamePrefix = "hist_cut";
    static std::vector<Triggers> makeTrgNames(const std::vector<std::string> &vecTrgNames
                                              ,const std::vector<std::string> &vecTrgClusterNames) {
      std::vector<Triggers> vecTrg;
      for(const auto &trgName: vecTrgNames) {
        if(trgName=="CINT7") continue;
        for(const auto &trgCluster: vecTrgNames) {
          std::string histNameNum = histNamePrefix+trgName+"_"+trgCluster;
          std::string histNameCINT7 = histNamePrefix+"CINT7"+"_"+trgCluster;
          std::string histNameRatio= histNamePrefix+trgName+"_CINT7_"+trgCluster;
          vecTrg.push_back(Triggers{histNameNum,histNameCINT7,histNameRatio});
        }
      }
      return vecTrg;
    }
  };
  */
  const std::string histNamePrefix = "hist_cut";
  std::vector<std::tuple<std::string,std::string,std::string>> vecTrg;
  for(const auto &trgName: vecTrgNames) {
    if(trgName=="CINT7") continue;
    for(const auto &trgCluster: vecTrgClusterNames) {
      std::string histNameNum = histNamePrefix+trgName+"_"+trgCluster;
      std::string histNameCINT7 = histNamePrefix+"CINT7"+"_"+trgCluster;
      std::string histNameRatio= histNamePrefix+trgName+"_CINT7_"+trgCluster;
      vecTrg.push_back({histNameNum,histNameCINT7,histNameRatio});
      if(trgName!="C0TVX") {
        std::string histNameNum = histNamePrefix+trgName+"_"+trgCluster;
        std::string histNameCINT7 = histNamePrefix+"C0TVX"+"_"+trgCluster;
        std::string histNameRatio= histNamePrefix+trgName+"_C0TVX_"+trgCluster;
        vecTrg.push_back({histNameNum,histNameCINT7,histNameRatio});
      }
    }
  }
  //std::vector<Triggers> vecTrg =Triggers::makeTrgNames(vecTrgNames,vecTrgClusterNames);

  ////////////////////////
  for(const auto& entry: mapRunsToFilepaths) {
    auto runnum = entry.first;
    auto filepath = entry.second;
    cout<<endl<<"\n========================";
    cout<<"\nRUN: "<<runnum;
    cout<<"\nPATH: "<<filepath[0];
    cout<<"\n========================\n";
    std::vector<std::tuple<std::string,std::string,std::string>> vecTupleHistNames;
    if(filepath.size()>1) {
      cout<<"\nWARNING! MORE THAN ONE FILE PER RUN!\n";
      continue;
    }
    TList *listOutput = new TList();
    listOutput->SetOwner(true);
    listOutput->SetName("output");
    TFile *fileInput = new TFile(filepath[0].c_str(),"READ");
    TList *listInputAll = dynamic_cast<TList *>(fileInput->Get("output"));

    if(listInputAll==nullptr) {
      cout<<"\nWARNING! NO DATA! RUN "<<runnum<<endl;
      continue;
    }
    listInputAll->SetOwner(true);
/*    for(const auto &subNames:vecTrg) {
      for(const auto &obj:(*listInputAll)) {
        TH1F *hist = dynamic_cast<TH1F *>(obj);
        std::string histNameC0TVX = hist->GetName();
        if(histNameC0TVX.find(subNames.mTrgNameNum)!=std::string::npos) {
          std::string histNameCINT7 = histNameC0TVX;
          histNameCINT7.replace(histNameCINT7.find(subNames.mTrgNameNum),subNames.mTrgNameNum.size(),subNames.mCINT7);
          std::string histNameRatio = histNameC0TVX;
          histNameRatio.replace(histNameRatio.find(subNames.mTrgNameNum),subNames.mTrgNameNum.size(),subNames.mRatioNum_CINT7);
          vecTupleHistNames.push_back({histNameC0TVX,histNameCINT7,histNameRatio});
          //cout<<endl<<histNameC0TVX<<"|"<<histNameCINT7<<"|"<<histNameRatio<<endl;
        }
      }
    }
*/
    //Calculate ratio
    TList *listRatio = utilities::Hists::makeDividedHists<TH1F>(*listInputAll,vecTrg,"b",true);
    //listRatio->Print();
    listOutput->AddAll(listRatio);
    //Add run number to hist title
    for(const auto &obj:(*listOutput)) {
      TH1F *objHist = dynamic_cast<TH1F *>(obj);
      if(objHist==nullptr) {
        std::cout<<"\nWARNING! CANNOT FIND OBJECT: "<<obj->GetName()<<std::endl;
        continue;
      }
      std::string title = objHist->GetTitle();
      std::string runDescriptor = Form("_run%i",runnum);
      title+=runDescriptor;
      objHist->SetTitle(title.c_str());
      objHist->SetDirectory(0);
    }
     //Deleting empty hists
    TList *listGarbage = new TList();
    listGarbage->SetOwner(kTRUE);
    for(const auto&entry:(*listOutput)) {
      TH1F *hist = dynamic_cast<TH1F *>(entry);
      if(hist==nullptr) {
        std::cout<<"\nWARNING! Not a hist object in the list: "<<entry->GetName()<<std::endl;
        continue;
      }
      if(hist->GetEntries()==0) {
        //std::cout<<"\nDeleting empty hist: "<<hist->GetName()<<std::endl;
        auto objRemoved = listOutput->Remove(hist);
        listGarbage->Add(objRemoved);
        //delete hist;
        //delete objRemoved;
      }
    }
    delete listGarbage;

    //listTmp->Print();
    delete listInputAll;
    fileInput->Close();
    delete fileInput;
    if(listOutput->GetSize()==0) {
      delete listOutput;
      continue;
    }
    TFile *fileOutput = new TFile(Form(pathOutput.c_str(),runnum),"RECREATE");
    fileOutput->WriteObject(listOutput,listOutput->GetName(),"SingleKey");
    delete listOutput;
    fileOutput->Close();
    delete fileOutput;
  }
}
