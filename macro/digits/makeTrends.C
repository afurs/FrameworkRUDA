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

template<typename InputType,typename HistOutputType>
struct ParameterTrends {
  typedef InputType Input_t;
  typedef HistOutputType Hist_t;
  std::string srcHistname{};
  std::string dstHistname{};
  std::function<double(const Input_t &obj)> funcGetValue;
  std::function<double(const Input_t &obj)> funcGetError = [](const Input_t &obj)->double{return {};};
  bool useError{false};
  Hist_t *hist{nullptr};
  void init(TList *listOutput,std::size_t nBins) {
    hist=new Hist_t(dstHistname.c_str(),dstHistname.c_str(),nBins,0,nBins);
    listOutput->Add(hist);
  }
  void fill(unsigned int binPos,const Input_t* inputObj) {
    if(inputObj==nullptr) {
      std::cout<<"\nWARNING! CANNOT FIND OBJECT: "<<srcHistname<<std::endl;
      return;
    }
    hist->SetBinContent(static_cast<int>(binPos),funcGetValue(*inputObj));
    if(useError) {
      hist->SetBinError(static_cast<int>(binPos),funcGetError(*inputObj));
    }
  }
};
using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
void makeTrends(const std::string &pathToSrc) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("libCommonRUDA.so");

  //Input parameters
  using Hist_t = TH1F;
  std::vector<ParameterTrends<TH1F, Hist_t> > vecParamTrendsSrc1D = {
    {
      "hVertex_vrtTrg",
      "hVertexMean",
      [](const Hist_t &hist)->double {
         return hist.GetMean();
      }
    },
    {
      "hVertex_vrtTrg",
      "hVertexRMS",
      [](const Hist_t &hist)->double {
        return hist.GetRMS();
      }
    }
  };

  std::vector<ParameterTrends<TList, Hist_t> > vecParamTrendsSrcList;
  auto funcFractionSatellite = [](const TList &listInput)->double {
    TH2F *histVrtTrg = dynamic_cast<TH2F*>(listInput.FindObject("hCollisionTimeVsVertex_vrtTrg"));
    TH2F *histNoCut = dynamic_cast<TH2F*>(listInput.FindObject("hCollisionTimeVsVertex_noCut"));
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
  vecParamTrendsSrcList.push_back({"output","hSatteliteFractionToVrtTrg",funcFractionSatellite});

  const std::string filepathOutput="trends.root";
  //Get map run number->file
  const auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(pathToSrc);
  const auto nBins = mapRunToFilepaths.size();
  std::map<unsigned int,std::string> mapBinToRunNum{};
  {
    std::size_t bin=0;
    for(const auto &entry : mapRunToFilepaths) {
      bin++;
      mapBinToRunNum.insert({bin,std::to_string(entry.first)});
    }
  }
  // Preparing output hists
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  for(auto &entry: vecParamTrendsSrc1D) {
    entry.init(listOutput,nBins);
    HistHelper::makeHistBinNamed(entry.hist,mapBinToRunNum,0);
  }
  for(auto &entry: vecParamTrendsSrcList) {
    entry.init(listOutput,nBins);
    HistHelper::makeHistBinNamed(entry.hist,mapBinToRunNum,0);
  }
  //Processing
  for(const auto &entry: mapRunToFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    const auto &filepathInput = vecFilepaths[0];
    const unsigned int binPos= std::find_if(mapBinToRunNum.begin(),mapBinToRunNum.end(),[&runnum](const auto &en){return en.second==std::to_string(runnum);})->first;
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

    for(auto &trends: vecParamTrendsSrc1D) {
      Hist_t *hist = dynamic_cast<Hist_t *>(listInput1D->FindObject(trends.srcHistname.c_str()));
      trends.fill(binPos,hist);
    }
    for(auto &trends: vecParamTrendsSrcList) {
      trends.fill(binPos,listInput2D);
    }
    delete listInput1D;
    delete listInput2D;
    fileInput.Close();
  }
  const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(listOutput,filepathOutput);
  delete listOutput;
  std::cout<<std::endl;
}

