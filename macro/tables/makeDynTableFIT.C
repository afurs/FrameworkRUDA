#include <algorithm>
#include <vector>
#include <map>
#include <functional>
#include <string>

#include <TH1.h>
#include <TH2.h>
#include <TTree.h>
#include <TFile.h>
#include <TSystem.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/DynamicTable.h"

#include "FunctorField.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using namespace functors;
constexpr static std::size_t sNBC=3564;
void makeDynTable(const std::string &pathToSrc) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Input parameters
  //Default runnum
  const std::string filepathOutput="tableTrgFT0.csv";


  auto dynTable = common::DynamicTable(getRunnum, getVecTrgVals, getVecOutCollBC, getSpotFraction);
  dynTable.mArrFieldNames.push_back("runnum");
  dynTable.mArrFieldNames.push_back("OrA_FT0");
  dynTable.mArrFieldNames.push_back("OrC_FT0");
  dynTable.mArrFieldNames.push_back("SemiCentral_FT0");
  dynTable.mArrFieldNames.push_back("Central_FT0");
  dynTable.mArrFieldNames.push_back("Trigger_FT0");
  dynTable.mArrFieldNames.push_back("All_Events_FT0");

  dynTable.mArrFieldNames.push_back("out-of-coll(OrA)");
  dynTable.mArrFieldNames.push_back("out-of-coll(OrC)");
  dynTable.mArrFieldNames.push_back("out-of-coll(SemiCentral)");
  dynTable.mArrFieldNames.push_back("out-of-coll(Central)");
  dynTable.mArrFieldNames.push_back("out-of-coll(Trigger)");
  dynTable.mArrFieldNames.push_back("out-of-coll(All Events)");
  dynTable.mArrFieldNames.push_back("spotFraction");


  //Get map run number->file
  const auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(pathToSrc);
  //Processing
  for(const auto &entry: mapRunToFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    for(const auto &filepathInput : vecFilepaths) {
      //if(filepathInput) continue;
      if(filepathInput.find("_AmpBC") != std::string::npos || filepathInput.find("_TimeBC") != std::string::npos) continue;

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
      listInput1D->SetOwner(true);
      listInput2D->SetOwner(true);
      dynTable.setCurrentArg<0>(runnum);
//================================================================
      TH1F *hTrgRates = (TH1F *) listInput1D->FindObject("hTrgRates");
//      TH2F *hTriggersVsBC_inColl = (TH2F *) listInput2D->FindObject("hTriggersVsBC_inColl");
      TH2F *hTriggersVsBC = (TH2F *) listInput2D->FindObject("hTriggersVsBC");
      TH2F *hTriggersVsBC_outColl = (TH2F *) listInput2D->FindObject("hTriggersVsBC_outColl");

      TH2F *hCollisionTimeVsVertex_noCut = (TH2F *) listInput2D->FindObject("hCollisionTimeVsVertex_noCut");
      TH2F *hCollisionTimeVsVertex_vrtTrg = (TH2F *) listInput2D->FindObject("hCollisionTimeVsVertex_vrtTrg");
      
      if(hTrgRates==nullptr||hTriggersVsBC==nullptr||hTriggersVsBC_outColl==nullptr||hCollisionTimeVsVertex_noCut==nullptr||hCollisionTimeVsVertex_vrtTrg==nullptr) {
        std::cout<<"\n No hists!\n";
        continue;
      }
//================================================================
      dynTable.setCurrentArg<1>(hTrgRates);
      dynTable.setCurrentArg<2>(hTriggersVsBC_outColl, hTriggersVsBC);
      dynTable.setCurrentArg<3>(hCollisionTimeVsVertex_noCut, hCollisionTimeVsVertex_vrtTrg);
      
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

