#include <algorithm>
#include <vector>
#include <map>
#include <functional>
#include <set>
#include <string>
#include <tuple>

#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TSystem.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/DynamicTable.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
void divideHists(const std::string &filepathToSrc,const std::string &filepathOutput,const std::vector<std::tuple<std::string,std::string,std::string>> &vecHistNames) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  TH1::AddDirectory(kFALSE);
  // Preparing output hists
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");

  std::cout<<"\nProcessing file: "<<filepathToSrc<<std::endl;
  TFile fileInput(filepathToSrc.c_str(),"READ");
  if(!fileInput.IsOpen()) {
    std::cout<<"\nWARNING! CANNOT OPEN FILE: "<<filepathToSrc<<std::endl;
    return;
  }
//  TList *listInput1D = Utils::getListObjFromFile<TH1F>(fileInput);
//  TList *listInput2D = Utils::getListObjFromFile<TH2F>(fileInput);
  TList *listInput1D = new TList();
  TList *listInput = dynamic_cast<TList *>(fileInput.Get("output"));
  Utils::getObjectRecursively<TH1F>(listInput1D,listInput);
  listInput->SetOwner(true);
//  listInput1D->SetOwner(true);

  TList *listRatio = HistHelper::makeDividedHists<TH1F>(*listInput1D,vecHistNames,"b",false);
  listRatio->SetOwner(false);
  listRatio->SetName("ratio");
  listOutput->AddAll(listRatio);

  listOutput->Print();

  delete listInput1D;
  delete listInput;
  delete listRatio;
  fileInput.Close();

  const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(listOutput,filepathOutput);
  delete listOutput;
  std::cout<<std::endl;
}