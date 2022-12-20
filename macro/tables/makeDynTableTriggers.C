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


  const std::string filepathOutput="trends.csv";
  auto dynTable = common::DynamicTable(getRunnum, ratioTrgs,ratioTrgs,ratioTrgs,ratioTrgs,getCnt,getCnt);
  
  dynTable.mArrFieldNames.resize(7);
  dynTable.mArrFieldNames[0]="runnum";
  dynTable.mArrFieldNames[1]="vertexFDD/vertexFT0";
  
  dynTable.mArrFieldNames[2]="centFT0/vertexFT0";
  dynTable.mArrFieldNames[3]="semicentFT0/vertexFT0";

  dynTable.mArrFieldNames[4]="centFT0/semicentFT0";
  dynTable.mArrFieldNames[5]="cntVertexFDD";
  dynTable.mArrFieldNames[6]="cntVertexFT0";


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
      TList *inputList = dynamic_cast<TList *>(fileInput.Get("output"));
      TList *listInput1D = new TList();
      TList *listInput2D = new TList();
      Utils::getObjectRecursively<TH1F>(listInput1D,inputList);
      Utils::getObjectRecursively<TH2F>(listInput2D,inputList);
//      TList *listInput1D = Utils::getListObjFromFile<TH1F>(fileInput);
//      TList *listInput2D = Utils::getListObjFromFile<TH2F>(fileInput);
      listInput1D->SetOwner(true);
      listInput2D->SetOwner(true);
      
      TH2F *hTriggerFDD_BC = (TH2F *) listInput2D->FindObject("hTriggerFDD_BC");
      TH2F *hTriggerFT0_BC = (TH2F *) listInput2D->FindObject("hTriggerFT0_BC");
      TH2F *hTriggerFV0_BC = (TH2F *) listInput2D->FindObject("hTriggerFV0_BC");
      TH2F *hTriggerFDD_FT0 = (TH2F *) listInput2D->FindObject("hTriggerFV0_BC");
    

      dynTable.setCurrentArg<0>(runnum);
      dynTable.setCurrentArg<1>(hTriggerFDD_BC,hTriggerFT0_BC,5,5);
      dynTable.setCurrentArg<2>(hTriggerFT0_BC,hTriggerFT0_BC,4,5);
      dynTable.setCurrentArg<3>(hTriggerFT0_BC,hTriggerFT0_BC,3,5);
      dynTable.setCurrentArg<4>(hTriggerFT0_BC,hTriggerFT0_BC,4,3);
      dynTable.setCurrentArg<5>(hTriggerFDD_BC,5);
      dynTable.setCurrentArg<6>(hTriggerFT0_BC,5);
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

