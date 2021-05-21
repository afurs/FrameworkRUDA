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

//Making trends from 1D hists distributed among single files
//Run number should exist in filename, for key-value definitions
void makeTrends(std::string pathInputData,std::string pathOutput="output.root",  bool doLabeling = true) {
  using HistSrc_t = TH1;
  using HistDest_t = TH1F;

  auto mapRunsToFilepaths = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputData);
  struct Trends {
    std::string mName;
    std::map<std::string,std::tuple<double,double>> mRun2Ratio;
  };
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  std::map<std::string,std::map<int,Trends>> mapBin2TrendsFull;

  for(const auto&entry: mapRunsToFilepaths) {
    auto runnum = entry.first;
    auto filepath = entry.second;
    cout<<endl<<"\n========================";
    cout<<"\nRUN: "<<runnum;
    cout<<"\nPATH: "<<filepath[0];
    cout<<"\n========================\n";
    TFile *fileInput = new TFile(filepath[0].c_str(),"READ");
    TList *listInputAll = utilities::AnalysisUtils::getListObjFromFile<HistSrc_t>(*fileInput);
    //TList *listInputAll = dynamic_cast<TList *>(fileInput->Get("output"));
    listInputAll->SetOwner(kTRUE);
    //histTrend->GetXaxis()->SetBinLabel(binCount,Form("%i",runnum));
    if(listInputAll==nullptr) {
      cout<<"\nWARNING! NO DATA! RUN "<<runnum<<endl;
      continue;
    }
    for(const auto &obj:(*listInputAll)) {
      HistSrc_t *histObj = dynamic_cast<HistSrc_t *>(obj);
      if(histObj==nullptr) continue;
//      if(histObj->GetXaxis()->GetNbins()>50) continue;
      auto pairInsertedFull=mapBin2TrendsFull.insert({std::string{histObj->GetName()},{}});
      auto &mapBin2Trends=pairInsertedFull.first->second;
      for(int iBin=1;iBin<histObj->GetXaxis()->GetNbins()+1;iBin++) {
        auto pairInserted=mapBin2Trends.insert({iBin,{std::string{Form("%i",runnum)},{}}});
        pairInserted.first->second.mName=std::string{histObj->GetXaxis()->GetBinLabel(iBin)};
        pairInserted.first->second.mRun2Ratio.insert({ std::string{Form("%i",runnum)}
                                                      ,{histObj->GetBinContent(iBin),histObj->GetBinError(iBin)} });
      }
    }
    delete listInputAll;
    fileInput->Close();
    delete fileInput;
  }
  for(const auto &entry:mapBin2TrendsFull) {
    TList *listTmp=new TList();
    listTmp->SetOwner(kTRUE);
    listTmp->SetName(std::string{"list_"+entry.first}.c_str());
    listOutput->Add(listTmp);

    for(const auto &entryBins:entry.second) {
      HistDest_t *hist = utilities::Hists::makeHist1DFromMap<HistDest_t>(entryBins.second.mRun2Ratio
                                                                 ,entry.first+Form("_bin%i",entryBins.first)
                                                                 ,entryBins.second.mName
                                                                 ,doLabeling);
      if(hist->GetEntries()==0) {
        cout<<"\nWARNING! No entries in hist: "<<hist->GetName()<<endl;
        delete hist;
        continue;
      }
      listTmp->Add(hist);
      std::cout<<"\n=============================";
      std::cout<<"\nHist name: "<<hist->GetName();
      std::cout<<"\nHist title: "<<hist->GetTitle();
      std::cout<<"\n=============================\n";
    }
  }

  TFile *fileOutput = new TFile(pathOutput.c_str(),"RECREATE");
  fileOutput->WriteObject(listOutput,listOutput->GetName(),"SingleKey");
  delete listOutput;
  fileOutput->Close();
  delete fileOutput;
}
