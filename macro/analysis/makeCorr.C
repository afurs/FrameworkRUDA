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
//For correlation between trends. Bin number/label taken as key ID
//
void makeCorr(std::string pathInputFile,
              std::string pathOutputFile,
              std::vector<std::tuple<std::string,std::string,std::string>> vecTupleHistNames
             ) {
/*
  std::string pathInputFile = "resultLumiTrendsRatio2018/LHC18p.root";
  std::string pathOutputFile = "resultLumiCorrPlots.root";
  std::vector<std::tuple<std::string,std::string,std::string>> vecTupleHistNames = {
    {"histRatio_cutInput0TVX_TO_INT7_bin17","hVZERO_bin1","hFeatureV0_to_Ratio"}
  };
*/
  using Hist_t = TH1F;

  ////////////////////////////////////

  TFile *fileInput = new TFile(pathInputFile.c_str(),"READ");
  TList *listInput = utilities::AnalysisUtils::getListObjFromFile<Hist_t>(*fileInput);
  listInput->Print();

  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");

  for(const auto &entry:vecTupleHistNames) {
    auto firstHistName = std::get<0>(entry);
    auto secondHistName = std::get<1>(entry);
    auto newHistName = std::get<2>(entry);
    ///CHECKING
    Hist_t *histFirst = dynamic_cast<Hist_t *>(listInput->FindObject(firstHistName.c_str()));
    if(histFirst==nullptr) {
      std::cout<<"\nWARNING! Cannot find object \""<<firstHistName<<"\"\n";
      continue;
    }
    Hist_t *histSecond = dynamic_cast<Hist_t *>(listInput->FindObject(secondHistName.c_str()));
    if(histSecond==nullptr) {
      std::cout<<"\nWARNING! Cannot find object \""<<secondHistName<<"\"\n";
      continue;
    }
    if(histFirst->GetEntries()==0) {
      std::cout<<"\nWARNING! No entries in \""<<firstHistName<<"\"\n";
      continue;
    }
    if(histSecond->GetEntries()==0) {
      std::cout<<"\nWARNING! No entries in \""<<secondHistName<<"\"\n";
      continue;
    }
    //////////////////////////////////
    std::map<std::string,double> mapFirst,mapSecond;
    std::multimap<double,double> mapValues;
    for(int iBin=1;iBin<histFirst->GetXaxis()->GetNbins()+1;iBin++) {
      mapFirst.insert({std::string{histFirst->GetXaxis()->GetBinLabel(iBin)}
                       ,histFirst->GetBinContent(iBin)});
    }
    for(int iBin=1;iBin<histSecond->GetXaxis()->GetNbins()+1;iBin++) {
      mapSecond.insert({std::string{histSecond->GetXaxis()->GetBinLabel(iBin)}
                       ,histSecond->GetBinContent(iBin)});
    }
    for(const auto &entryPair:mapFirst) {
      auto it = mapSecond.find(entryPair.first);
      if(it!=mapSecond.end()) mapValues.insert({entryPair.second,it->second});
    }
    int cntPoint=0;
    TGraph *gr = new TGraph(mapValues.size());
    gr->SetName(newHistName.c_str());
    gr->SetTitle(std::string{newHistName+";"+firstHistName+";"+secondHistName}.c_str());
    listOutput->Add(gr);
    for(const auto &entryPair:mapValues) {
      gr->SetPoint(cntPoint,entryPair.first,entryPair.second);
      cntPoint++;
    }
  }
  TFile *fileOutput = new TFile(pathOutputFile.c_str(),"RECREATE");
  fileOutput->WriteObject(listOutput,listOutput->GetName(),"SingleKey");
  delete listOutput;
  fileOutput->Close();
  delete fileOutput;

  delete listInput;
  fileInput->Close();
  delete fileInput;


}
