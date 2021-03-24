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
template<typename Hist_t>
double getFeature(Hist_t *hist) {
  struct Range {
    float min;
    float max;
    double getIntegral(Hist_t *hist) {
      return hist->Integral(hist->FindBin(min),hist->FindBin(max));
    }
  };
  Range mainPeak{-1.,1.};
  Range satellitePeak{-4.,-1.};
  double mainIntegral = mainPeak.getIntegral(hist);
  double satelliteIntegral = satellitePeak.getIntegral(hist);
  if(mainIntegral!=0) {
    return satelliteIntegral/mainIntegral;
  }
  else {
    return 0.;
  }

}

void taskTrendSpectrum() {
  std::string pathInputData = "/home/deliner/work/data/ageing/2018/LHC18p";
  std::string pathOutput = "result.root";
  auto mapRunsToFilepaths = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputData);
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  TH1F *histTrend = new TH1F("hTrend_ch11","hTrend_ch11",mapRunsToFilepaths.size(),0,mapRunsToFilepaths.size());
  listOutput->Add(histTrend);
  int binCount=0;
  for(const auto& entry: mapRunsToFilepaths) {
    auto runnum = entry.first;
    auto filepath = entry.second;
    binCount++;
    cout<<endl<<"\n========================";
    cout<<"\nRUN: "<<runnum;
    cout<<"\nPATH: "<<filepath[0];
    cout<<"\n========================\n";
    TFile *fileInput = new TFile(filepath[0].c_str(),"READ");
    TList *listInputAll = dynamic_cast<TList *>(fileInput->Get("outputList"));
    histTrend->GetXaxis()->SetBinLabel(binCount,Form("%i",runnum));
    if(listInputAll==nullptr) {
      cout<<"\nWARNING! NO DATA! RUN "<<runnum<<endl;
      continue;
    }
    listInputAll->SetOwner(true);



    TList *listInput = dynamic_cast<TList *>(listInputAll->FindObject("hists_v0"));
    listInput->SetOwner(true);
    TH2F *histInput = dynamic_cast<TH2F *>(listInput->FindObject("hTimeFull_zoom_hit1__v0"));

    //histTmp->SetName(histTmp->GetName()+Form("_run%i",runnum));
    TList *listTmp = utilities::Hists::decomposeHist<TH1F>(histInput,false);
    listTmp->SetOwner(true);
    listTmp->SetName(Form("run%i",runnum));

    TH1F *histTmp = (TH1F *) listTmp->FindObject("hTimeFull_zoom_hit1__v0_b11");
    histTrend->SetBinContent(binCount,getFeature(histTmp));
   // histTrend->
    for(auto en: (*listTmp)) {
      TH1F *hist = dynamic_cast<TH1F *>(en);
      std::string stName = hist->GetName();
      hist->SetName(std::string{stName+Form("_run%i",runnum)}.c_str());
      hist->SetDirectory(0);
    }
    listOutput->Add(listTmp);
    //listTmp->Print();
    delete listInputAll;
    fileInput->Close();
    delete fileInput;
  }

  /*
  for(const auto &entry: (*listOutput)) {
    ((TList *)entry)->Print();
  }
  */

  TFile *fileOutput = new TFile(pathOutput.c_str(),"RECREATE");
  fileOutput->WriteObject(listOutput,listOutput->GetName(),"SingleKey");
  /*
  for(const auto &entry: (*listOutput)) {
    TList *listTmp = ((TList *)entry);
    listTmp->Print();
    delete listTmp;
  }
  */
  delete listOutput;
  fileOutput->Close();
  delete fileOutput;
}
