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
  std::string pathInputDataLumi = "/home/deliner/work/Analysis/ageing/input_lumi";
  std::string pathOutput = "resultLumiTrend.root";
  std::string stLumiHistName = "hCutRatio_CINT7_C0TVX_CENT";
  using Hist_t = TH1F;
  auto mapRunsToFilepathsLumi = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputDataLumi);
  struct Trends {
    std::string mName;
    std::map<std::string,std::tuple<double,double>> mRun2Ratio;
  };
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  int cntBin=0;
  std::map<int,Trends> mapBin2Trends;
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
    Hist_t *histLumi = (Hist_t *)listInputAllLumi->FindObject(stLumiHistName.c_str());
    if(histLumi==nullptr) {
      cout<<"\nWARNING! NO HIST! RUN "<<runnum<<endl;
      continue;
    }
    //Proccesing bins in hist
    for(int iBin=1;iBin<histLumi->GetXaxis()->GetNbins()+1;iBin++) {
      auto pairInserted=mapBin2Trends.insert({iBin,{std::string{Form("%i",runnum)},{}}});
      pairInserted.first->second.mName=std::string{histLumi->GetXaxis()->GetBinLabel(iBin)};
      pairInserted.first->second.mRun2Ratio.insert({ std::string{Form("%i",runnum)}
                                                    ,{histLumi->GetBinContent(iBin),histLumi->GetBinError(iBin)} });
    }
    cntBin++;
  }
  for(const auto &entry:mapBin2Trends) {
    Hist_t *hist = utilities::Hists::makeHist1DFromMap<Hist_t>(entry.second.mRun2Ratio
                                                               ,Form("hBin%i",entry.first)
                                                               ,entry.second.mName
                                                               ,true);
    if(hist->GetEntries()==0) {
      delete hist;
      continue;
    }
    listOutput->Add(hist);
    std::cout<<"\n=============================";
    std::cout<<"\nHist name: "<<hist->GetName();
    std::cout<<"\nHist title: "<<hist->GetTitle();
    std::cout<<"\n=============================\n";

  }

  TFile *fileOutput = new TFile(pathOutput.c_str(),"RECREATE");
  fileOutput->WriteObject(listOutput,listOutput->GetName(),"SingleKey");
  delete listOutput;
  fileOutput->Close();
  delete fileOutput;
}
