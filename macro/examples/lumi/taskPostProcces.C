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

void taskPostProcces(){

  //std::string pathInputData = "/home/deliner/work/data/ageing/2018/LHC18p";
  std::string pathInputData = "output";
  std::string pathOutput = "resultLumiRun%i.root";
  auto mapRunsToFilepaths = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputData);
  //INTERFACE FOR TRIGGER NAMES IN HISTS
  struct Triggers {
    std::string mC0TVX;
    std::string mCINT7;
    std::string mRatioCINT7_C0TVX;
    static std::vector<Triggers> makeTrgNames(const std::vector<std::string> &vecTrgSubNames) {
      std::vector<Triggers> vecTrg;
      for(const auto &entry:vecTrgSubNames) {
        vecTrg.push_back(Triggers{std::string{"C0TVX_"+entry},std::string{"CINT7_"+entry},std::string{"Ratio_CINT7_C0TVX_"+entry}});
      }
      return vecTrg;
    }
  };
  std::vector<std::string> vecTrgSubNamesMain = {"ALLNOTRD","CENTNOTRD","CENT","FAST","MUFAST"};
  std::vector<Triggers> vecTrg =Triggers::makeTrgNames(vecTrgSubNamesMain);

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
    for(const auto &subNames:vecTrg) {
      for(const auto &obj:(*listInputAll)) {
        TH1F *hist = dynamic_cast<TH1F *>(obj);
        std::string histNameC0TVX = hist->GetName();
        if(histNameC0TVX.find(subNames.mC0TVX)!=std::string::npos) {
          std::string histNameCINT7 = histNameC0TVX;
          histNameCINT7.replace(histNameCINT7.find(subNames.mC0TVX),subNames.mC0TVX.size(),subNames.mCINT7);
          std::string histNameRatio = histNameC0TVX;
          histNameRatio.replace(histNameRatio.find(subNames.mC0TVX),subNames.mC0TVX.size(),subNames.mRatioCINT7_C0TVX);
          vecTupleHistNames.push_back({histNameC0TVX,histNameCINT7,histNameRatio});
          //cout<<endl<<histNameC0TVX<<"|"<<histNameCINT7<<"|"<<histNameRatio<<endl;
        }
      }
    }

    //Calculate ratio
    TList *listRatio = utilities::Hists::makeDividedHists<TH1F>(*listInputAll,vecTupleHistNames,"b",true);
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
