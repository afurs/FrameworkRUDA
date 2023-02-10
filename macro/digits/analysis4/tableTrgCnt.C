#include <algorithm>
#include <vector>
#include <map>
#include <memory>
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

#include "Triggers.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using Trigger = triggers::Trigger;
constexpr static double sRateTF = 88.92459623;
constexpr static std::size_t sNBC=3564;
void tableTrgCnt(const std::string &pathToSrc) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Default runnum
  std::function<unsigned int(unsigned int)>  getRunnum = [](unsigned int runnum) { return runnum; };
  std::function<unsigned int(unsigned int)>  getValue = [](unsigned int getValue) { return getValue; };

  //Input parameters
  //Default runnum
  std::function<double(const TH1F *)>  getDurationSec = [](const TH1F *hist) { 
    return hist->GetBinContent(1) / sRateTF;
  };

  std::function<double(const TH1F *)>  getNTF = [](const TH1F *hist) { 
    return hist->GetBinContent(1);
  };

  std::function<std::vector<double>(const TH2F *)> getTrgCountersPair = [](const TH2F *hist) {
    std::vector<double> vecResult{};
    for(int iBinY=1;iBinY<hist->GetYaxis()->GetNbins()+1;iBinY++) {
      for(int iBinX=1;iBinX<hist->GetXaxis()->GetNbins()+1;iBinX++) {
        const auto val = hist->GetBinContent(iBinX,iBinY);
        vecResult.push_back(val);
      }
    }
    return vecResult;
  };

  std::function<std::vector<double>(const TH2F *)> getTrgCounters = [](const TH2F *hist) {
    std::vector<double> vecResult{};
    std::unique_ptr<TH1D> histProj(hist->ProjectionY());
    for(int iBinX=1;iBinX<31;iBinX++) {
      const auto val = histProj->GetBinContent(iBinX);
      vecResult.push_back(val);
    }
    return vecResult;
  };

  auto makeHeaderTrgCnt = [](const std::string &detName,std::vector<std::string> &vecHeaders) {
    for(int iTrg = 0;iTrg<30; iTrg++) {
      std::string beamMask,trgBit;
      Trigger::getBeamMaskTrgBitNamesFT0(iTrg,beamMask,trgBit);
      const std::string fieldName = detName+std::string{" "}+beamMask+std::string{"_"}+trgBit;
      vecHeaders.push_back(fieldName);
    }
  };
  auto makeHeaderTrgCntPair = [](const std::string &detNameFirst,const std::string &detNameSecond,std::vector<std::string> &vecHeaders) {
    for(int iTrgSecond = 0;iTrgSecond<30; iTrgSecond++) {
      std::string beamMaskSecond,trgBitSecond;
      Trigger::getBeamMaskTrgBitNamesFT0(iTrgSecond,beamMaskSecond,trgBitSecond);
      for(int iTrgFirst = 0;iTrgFirst<30; iTrgFirst++) {
        std::string beamMaskFirst,trgBitFirst;
        Trigger::getBeamMaskTrgBitNamesFT0(iTrgFirst,beamMaskFirst,trgBitFirst);
        std::string fieldName = detNameFirst+std::string{" "}+beamMaskFirst+std::string{"_"}+trgBitFirst;
        fieldName += std::string{" + "};
        fieldName += detNameSecond+std::string{" "}+beamMaskSecond+std::string{"_"}+trgBitSecond;
        vecHeaders.push_back(fieldName);
      }
    }
  };


  const std::string filepathOutputTrgCntFIT="tableTrgCntFIT.csv";
  auto dynTableTrgCntFIT = common::DynamicTable(getRunnum, getNTF,getDurationSec, getTrgCounters,getTrgCounters,getTrgCounters);
  dynTableTrgCntFIT.mArrFieldNames.push_back("Runnum");
  dynTableTrgCntFIT.mArrFieldNames.push_back("NTFs");
  dynTableTrgCntFIT.mArrFieldNames.push_back("Duration [sec]");
  makeHeaderTrgCnt("FDD",dynTableTrgCntFIT.mArrFieldNames);
  makeHeaderTrgCnt("FT0",dynTableTrgCntFIT.mArrFieldNames);
  makeHeaderTrgCnt("FV0",dynTableTrgCntFIT.mArrFieldNames);

  const std::string filepathOutputTrgCnt_FDD_FT0="tableTrgCnt_FDD_FT0.csv";
  auto dynTableTrgCnt_FDD_FT0 = common::DynamicTable(getRunnum, getNTF,getDurationSec,getTrgCountersPair );
  dynTableTrgCnt_FDD_FT0.mArrFieldNames.push_back("Runnum");
  dynTableTrgCnt_FDD_FT0.mArrFieldNames.push_back("NTFs");
  dynTableTrgCnt_FDD_FT0.mArrFieldNames.push_back("Duration [sec]");
  makeHeaderTrgCntPair("FDD","FT0",dynTableTrgCnt_FDD_FT0.mArrFieldNames);

  const std::string filepathOutputTrgCnt_FDD_FV0="tableTrgCnt_FDD_FV0.csv";
  auto dynTableTrgCnt_FDD_FV0 = common::DynamicTable(getRunnum, getNTF,getDurationSec,getTrgCountersPair );
  dynTableTrgCnt_FDD_FV0.mArrFieldNames.push_back("Runnum");
  dynTableTrgCnt_FDD_FV0.mArrFieldNames.push_back("NTFs");
  dynTableTrgCnt_FDD_FV0.mArrFieldNames.push_back("Duration [sec]");
  makeHeaderTrgCntPair("FDD","FV0",dynTableTrgCnt_FDD_FV0.mArrFieldNames);

  const std::string filepathOutputTrgCnt_FT0_FV0="tableTrgCnt_FT0_FV0.csv";
  auto dynTableTrgCnt_FT0_FV0 = common::DynamicTable(getRunnum, getNTF,getDurationSec,getTrgCountersPair );
  dynTableTrgCnt_FT0_FV0.mArrFieldNames.push_back("Runnum");
  dynTableTrgCnt_FT0_FV0.mArrFieldNames.push_back("NTFs");
  dynTableTrgCnt_FT0_FV0.mArrFieldNames.push_back("Duration [sec]");
  makeHeaderTrgCntPair("FT0","FV0",dynTableTrgCnt_FT0_FV0.mArrFieldNames);

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

      listInput1D->SetOwner(true);
      listInput2D->SetOwner(true);
      TH1F *hTF = (TH1F *) listInput1D->FindObject("hTF");

      TH2F *hTrgAndBeamMaskFDD_perBC = (TH2F *) listInput2D->FindObject("hTrgAndBeamMaskFDD_perBC");
      TH2F *hTrgAndBeamMaskFT0_perBC = (TH2F *) listInput2D->FindObject("hTrgAndBeamMaskFT0_perBC");
      TH2F *hTrgAndBeamMaskFV0_perBC = (TH2F *) listInput2D->FindObject("hTrgAndBeamMaskFV0_perBC");

      TH2F *hTrgAndBeamMask_FDD_FT0 = (TH2F *) listInput2D->FindObject("hTrgAndBeamMask_FDD_FT0");
      TH2F *hTrgAndBeamMask_FDD_FV0 = (TH2F *) listInput2D->FindObject("hTrgAndBeamMask_FDD_FV0");
      TH2F *hTrgAndBeamMask_FT0_FV0 = (TH2F *) listInput2D->FindObject("hTrgAndBeamMask_FT0_FV0");

      dynTableTrgCntFIT.setCurrentArg<0>(runnum);
      dynTableTrgCntFIT.setCurrentArg<1>(hTF);
      dynTableTrgCntFIT.setCurrentArg<2>(hTF);
      dynTableTrgCntFIT.setCurrentArg<3>(hTrgAndBeamMaskFDD_perBC);
      dynTableTrgCntFIT.setCurrentArg<4>(hTrgAndBeamMaskFT0_perBC);
      dynTableTrgCntFIT.setCurrentArg<5>(hTrgAndBeamMaskFV0_perBC);

      dynTableTrgCnt_FDD_FT0.setCurrentArg<0>(runnum);
      dynTableTrgCnt_FDD_FT0.setCurrentArg<1>(hTF);
      dynTableTrgCnt_FDD_FT0.setCurrentArg<2>(hTF);
      dynTableTrgCnt_FDD_FT0.setCurrentArg<3>(hTrgAndBeamMask_FDD_FT0);

      dynTableTrgCnt_FDD_FV0.setCurrentArg<0>(runnum);
      dynTableTrgCnt_FDD_FV0.setCurrentArg<1>(hTF);
      dynTableTrgCnt_FDD_FV0.setCurrentArg<2>(hTF);
      dynTableTrgCnt_FDD_FV0.setCurrentArg<3>(hTrgAndBeamMask_FDD_FV0);

      dynTableTrgCnt_FT0_FV0.setCurrentArg<0>(runnum);
      dynTableTrgCnt_FT0_FV0.setCurrentArg<1>(hTF);
      dynTableTrgCnt_FT0_FV0.setCurrentArg<2>(hTF);
      dynTableTrgCnt_FT0_FV0.setCurrentArg<3>(hTrgAndBeamMask_FT0_FV0);

      dynTableTrgCntFIT.fillTable();
      dynTableTrgCnt_FDD_FT0.fillTable();
      dynTableTrgCnt_FDD_FV0.fillTable();
      dynTableTrgCnt_FT0_FV0.fillTable();

      delete listInput1D;
      delete listInput2D;
      fileInput.Close();
    }
  }
  dynTableTrgCntFIT.toCSV(filepathOutputTrgCntFIT,true);
  dynTableTrgCnt_FDD_FT0.toCSV(filepathOutputTrgCnt_FDD_FT0,true);
  dynTableTrgCnt_FDD_FV0.toCSV(filepathOutputTrgCnt_FDD_FV0,true);
  dynTableTrgCnt_FT0_FV0.toCSV(filepathOutputTrgCnt_FT0_FV0,true);
  std::cout<<std::endl;
}

