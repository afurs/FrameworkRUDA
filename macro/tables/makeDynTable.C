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
constexpr static std::size_t sNBC=3564;
void makeDynTable(const std::string &pathToSrc) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Input parameters
  //Default runnum
  std::function<unsigned int(unsigned int)>  getRunnum = [](unsigned int runnum) { return runnum; };

//=========================================================================
  std::function<double(const TH2F *, const TH2F *,int,int)> ratioTrgs = [](const TH2F *h1,const TH2F *h2,int binPos1,int binPos2) {
    const double val1 = h1->Integral(1,3564, binPos1, binPos1);
    const double val2 = h2->Integral(1,3564, binPos2, binPos2);
    if(val2==0) {
      return val2;
    }
    return val1/val2;
  };
  std::function<double(const TH2F *,int)> getCnt = [](const TH2F *h1,int binPos1) {
    const double val1 = h1->Integral(1,3564, binPos1, binPos1);
    return val1;
  };

  std::function<double(const TH1F *)> getMean = [](const TH1F *hist) {
    return static_cast<double>(hist->GetMean());
  };
  std::function<double(const TH1F *)> getRMS = [](const TH1F *hist) {
    return static_cast<double>(hist->GetRMS());
  };


  std::function<double(const TH1F *)> getIntegral = [](const TH1F *hist) {
    return static_cast<double>(hist->Integral());
  };
//=========================================================================
  const std::string filepathOutput="table.csv";


  auto dynTable = common::DynamicTable(getRunnum, getMean,getMean,getRMS,getRMS,getIntegral,getIntegral);
  dynTable.mArrFieldNames[0]="runnum";
  dynTable.mArrFieldNames[1]="meanTimeA";
  dynTable.mArrFieldNames[2]="meanTimeC";
  dynTable.mArrFieldNames[3]="rmsTimeA";
  dynTable.mArrFieldNames[4]="rmsTimeC";
  dynTable.mArrFieldNames[5]="entriesMeanTimeA";
  dynTable.mArrFieldNames[6]="entriesMeanTimeC";

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
      if(filepathInput.find("_AmpBC") != std::string::npos || filepathInput.find("_TimeBC") != std::string::npos) continue;

      TList *inputList = dynamic_cast<TList *>(fileInput.Get("output"));
      TList *listInput1D = new TList();
      TList *listInput2D = new TList();
      Utils::getObjectRecursively<TH1F>(listInput1D,inputList);
      Utils::getObjectRecursively<TH2F>(listInput2D,inputList);
      listInput1D->SetOwner(true);
      listInput2D->SetOwner(true);
      dynTable.setCurrentArg<0>(runnum);
//================================================================
      TH1F *hMeanTimeA = (TH1F *) listInput1D->FindObject("hMeanTimeA");
      TH1F *hMeanTimeC = (TH1F *) listInput1D->FindObject("hMeanTimeC");
      if(hMeanTimeA->GetEntries()==0) continue;
//================================================================
      dynTable.setCurrentArg<1>(hMeanTimeA);
      dynTable.setCurrentArg<2>(hMeanTimeC);
      dynTable.setCurrentArg<3>(hMeanTimeA);
      dynTable.setCurrentArg<4>(hMeanTimeC);
      dynTable.setCurrentArg<5>(hMeanTimeA);
      dynTable.setCurrentArg<6>(hMeanTimeC);
      
//================================================================
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

