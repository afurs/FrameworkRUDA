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

void makeTrends() {
  //std::string pathInputData = "/home/deliner/work/data/ageing/2018/LHC18p";
  //std::string pathInputDataLumi = "/home/deliner/work/Analysis/ageing/input_lumi";
  std::string pathInputDataLumi = "resultLumiRatio2018_Full2";
  std::string pathOutput = "resultLumiTrend_forMeeting.root";
  //std::string pathOutput = "resultLumiTrend_forMeeting.root";

//  std::string stLumiHistName = "hist_cutRatio_CINT7_C0TVX_CENT";
  bool doLabeling = false;
  using Hist_t = TH1F;
  auto mapRunsToFilepathsLumi = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputDataLumi);
  struct Trends {
    std::string mName;
    std::map<std::string,std::tuple<double,double>> mRun2Ratio;
  };
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  std::map<std::string,std::map<int,Trends>> mapBin2TrendsFull;

  for(const auto&entry: mapRunsToFilepathsLumi) {
    auto runnum = entry.first;
    auto filepathLumi = entry.second;
    TFile *fileInputLumi = new TFile(filepathLumi[0].c_str(),"READ");
    TList *listInputAllLumi = dynamic_cast<TList *>(fileInputLumi->Get("output"));

    //histTrend->GetXaxis()->SetBinLabel(binCount,Form("%i",runnum));
    if(listInputAllLumi==nullptr) {
      cout<<"\nWARNING! NO DATA! RUN "<<runnum<<endl;
      continue;
    }

    /*
    Hist_t *histLumi = (Hist_t *)listInputAllLumi->FindObject(stLumiHistName.c_str());
    if(histLumi==nullptr) {
      cout<<"\nWARNING! NO HIST! RUN "<<runnum<<endl;
      continue;
    }*/
    //Proccesing bins in hist
    for(const auto &obj:(*listInputAllLumi)) {
      Hist_t *histObj = dynamic_cast<Hist_t *>(obj);
      auto pairInsertedFull=mapBin2TrendsFull.insert({std::string{histObj->GetName()},{}});
      auto &mapBin2Trends=pairInsertedFull.first->second;
      for(int iBin=1;iBin<histObj->GetXaxis()->GetNbins()+1;iBin++) {
        auto pairInserted=mapBin2Trends.insert({iBin,{std::string{Form("%i",runnum)},{}}});
        pairInserted.first->second.mName=std::string{histObj->GetXaxis()->GetBinLabel(iBin)};
        pairInserted.first->second.mRun2Ratio.insert({ std::string{Form("%i",runnum)}
                                                      ,{histObj->GetBinContent(iBin),histObj->GetBinError(iBin)} });
      }
    }

  }
  for(const auto &entry:mapBin2TrendsFull) {
    TList *listTmp=new TList();
    listTmp->SetOwner(kTRUE);
    listTmp->SetName(std::string{"list_"+entry.first}.c_str());
    listOutput->Add(listTmp);

    for(const auto &entryBins:entry.second) {
      Hist_t *hist = utilities::Hists::makeHist1DFromMap<Hist_t>(entryBins.second.mRun2Ratio
                                                                 ,entry.first+Form("_bin%i",entryBins.first)
                                                                 ,entryBins.second.mName
                                                                 ,doLabeling);
      if(hist->GetEntries()==0) {
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
