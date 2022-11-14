#include <algorithm>
#include <vector>
#include <map>
#include <functional>
#include <string>

#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>
#include <TSystem.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/DynamicTable.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
void makeDynTable(const std::string &pathToSrc) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Input parameters
  //Default runnum
  std::function<unsigned int(unsigned int)>  getRunnum = [](unsigned int runnum) { return runnum; };
  //Satellite fraction
/*
  std::function<double(const TList *)>   funcFractionSatellite = [](const TList *listInput) {
    TH2F *histVrtTrg = dynamic_cast<TH2F*>(listInput->FindObject("hCollisionTimeVsVertex_vrtTrg"));
    TH2F *histNoCut = dynamic_cast<TH2F*>(listInput->FindObject("hCollisionTimeVsVertex_noCut"));
    if(histVrtTrg==nullptr) {
      std::cout<<"\nCannot find hCollisionTimeVsVertex_vrtTrg\n";
      return 0.;
    }
    if(histNoCut==nullptr) {
      std::cout<<"\nCannot find hCollisionTimeVsVertex_noCut\n";
      return 0.;
    }

    const double vrtTrgIntegral = histVrtTrg->Integral();
    const int binX1 = histNoCut->GetXaxis()->FindBin(-100.);
    const int binX2 = histNoCut->GetXaxis()->FindBin(-60.);
    const int binY1 = histNoCut->GetYaxis()->FindBin(-4.);
    const int binY2 = histNoCut->GetYaxis()->FindBin(-2.);
    const double satelliteIntegral = histNoCut->Integral(std::min(binX1,binX2),std::max(binX1,binX2),std::min(binY1,binY2),std::max(binY1,binY2));
    if(satelliteIntegral!=0) {
      return satelliteIntegral/vrtTrgIntegral;
    }
    else {
      return 0.;
    }
  };
*/
  //
  std::function<double(const TList *)> correlationFactor = [](const TList *listInput) {
    TH2F *hist = dynamic_cast<TH2F*>(listInput->FindObject("hNchannels_FDD"));
    if(hist==nullptr) {
      std::cout<<"\nCannot find histogram\n";
      return 0.;
    }
    const double correlationFactor = hist->GetCorrelationFactor();
    return correlationFactor;
  };

  const std::string filepathOutput="trends.csv";
  auto dynTable = common::DynamicTable(getRunnum, correlationFactor);
  dynTable.mArrFieldNames[0]="runnum";
  dynTable.mArrFieldNames[1]="correlationFactor";

  //Get map run number->file
  const auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(pathToSrc);
  //Processing
  for(const auto &entry: mapRunToFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    for(const auto &filepathInput : vecFilepaths) {
      //if(filepathInput) continue;
      std::cout<<"\nProcessing file: "<<filepathInput<<std::endl;
      TFile fileInput(filepathInput.c_str(),"READ");
      if(!fileInput.IsOpen()) {
        std::cout<<"\nWARNING! CANNOT OPEN FILE: "<<filepathInput<<std::endl;
        continue;
      }
      TList *listInput1D = Utils::getListObjFromFile<TH1F>(fileInput);
      TList *listInput2D = Utils::getListObjFromFile<TH2F>(fileInput);
      listInput1D->SetOwner(true);
      listInput2D->SetOwner(true);

      dynTable.setCurrentArg<0>(runnum);
      dynTable.setCurrentArg<1>(listInput2D);

      dynTable.fillTable();
      delete listInput1D;
      delete listInput2D;
      fileInput.Close();
    }
  }
  dynTable.print();
  dynTable.toCSV(filepathOutput,true);
  std::cout<<std::endl;
}

