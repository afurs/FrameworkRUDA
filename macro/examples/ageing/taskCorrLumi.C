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

void taskCorrLumi() {
  std::string pathInputData = "/home/deliner/work/data/ageing/2018/LHC18p";
  std::string pathInputDataLumi = "input_lumi";
  std::string pathOutput = "result2.root";
  std::string binNameLumi="V0_v1 noPileup PhysSel vertexGlobal";
  auto mapRunsToFilepaths = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputData);
  auto mapRunsToFilepathsLumi = utilities::AnalysisUtils::makeMapRunsToFilepathsROOT(pathInputDataLumi);
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  //TH1F *histTrend = new TH1F("hTrend_ch11","hTrend_ch11",mapRunsToFilepaths.size(),0,mapRunsToFilepaths.size());
  TH2F *histCorr = new TH2F("hCorr","hCorr;RatioC0TVX_CINT7;RatioSatellite",1000,0.4,0.5,3500,0,0.35);
  listOutput->Add(histCorr);
  int binCount=0;
  std::map<int,double> mapRatio;//
  std::map<int,double> mapFeature;//
  for(const auto& entry: mapRunsToFilepaths) {
    auto runnum = entry.first;
    auto filepath = entry.second;
    binCount++;
    cout<<endl<<"\n========================";
    cout<<"\nRUN: "<<runnum;
    cout<<"\nPATH: "<<filepath[0];
    cout<<"\n========================\n";
    if(mapRunsToFilepathsLumi.find(runnum)==mapRunsToFilepathsLumi.end()) {
      cout<<"\nNO CORRELATION FOR RUN "<<runnum<<endl;
      continue;
    }
    auto filepathLumi = mapRunsToFilepathsLumi.find(runnum)->second;
    TFile *fileInput = new TFile(filepath[0].c_str(),"READ");
    TList *listInputAll = dynamic_cast<TList *>(fileInput->Get("outputList"));
    TFile *fileInputLumi = new TFile(filepathLumi[0].c_str(),"READ");
    TList *listInputAllLumi = dynamic_cast<TList *>(fileInputLumi->Get("output"));
    //histTrend->GetXaxis()->SetBinLabel(binCount,Form("%i",runnum));
    if(listInputAll==nullptr||listInputAllLumi==nullptr) {
      cout<<"\nWARNING! NO DATA! RUN "<<runnum<<endl;
      continue;
    }
    TH1F *histLumi = (TH1F *)listInputAllLumi->FindObject("hCutRatio_CINT7_C0TVX_CENT");
    if(histLumi==nullptr) {
      cout<<"\nWARNING! NO HIST! RUN "<<runnum<<endl;
      continue;
    }
    //double ratio = histLumi->GetBinContent(histLumi->GetXaxis()->FindBin(binNameLumi.c_str()));
    double ratio = histLumi->GetBinContent(14);
    listInputAll->SetOwner(true);
    listInputAllLumi->SetOwner(true);


    TList *listInput = dynamic_cast<TList *>(listInputAll->FindObject("hists_v0"));
    listInput->SetOwner(true);
    TH2F *histInput = dynamic_cast<TH2F *>(listInput->FindObject("hTimeFull_zoom_hit1__v0"));

    //histTmp->SetName(histTmp->GetName()+Form("_run%i",runnum));
    TList *listTmp = utilities::Hists::decomposeHist<TH1F>(histInput,false);
    listTmp->SetOwner(true);
    listTmp->SetName(Form("run%i",runnum));

    TH1F *histTmp = (TH1F *) listTmp->FindObject("hTimeFull_zoom_hit1__v0_b11");
    double feature = getFeature(histTmp);
    cout<<endl<<ratio<<"|"<<feature<<endl;
    //histTrend->SetBinContent(binCount,feature);
    histCorr->Fill(ratio,feature);
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
