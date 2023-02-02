#include <algorithm>
#include <vector>
#include <map>
#include <functional>
#include <set>
#include <string>

#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>
#include <TSystem.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/DynamicTable.h"

//#include "TrgMaps.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
void decomposeHists(const std::string &filepathToSrc,const std::set<std::string> &setHistNames,const std::string &filepathOutput="decomposedHists.root", int axis=0,bool useBinNameAsSuffix=false) {
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
  TList *listInput1D = Utils::getListObjFromFile<TH1F>(fileInput);
  TList *listInput2D = Utils::getListObjFromFile<TH2F>(fileInput);
  listInput1D->SetOwner(true);
  listInput2D->SetOwner(true);
  for(const auto &histName: setHistNames) {
    TH2F *hist = (TH2F *) listInput2D->FindObject(histName.c_str());
    std::cout<<"\nDecomposing hist: "<<histName<<std::endl;
    if(hist==nullptr) {
      std::cout<<"\nHist "<<histName<<" doesn't exist\n";
      continue;
    }
    TList *listDecomposed = HistHelper::decomposeHist<TH1F>(hist,false,axis,useBinNameAsSuffix);
    listDecomposed->SetOwner(true);
    listDecomposed->SetName(std::string{"listDecomposed_"+histName}.c_str());
    listDecomposed->Print();
    listOutput->Add(listDecomposed);
  }
  listOutput->Print();

  delete listInput1D;
  delete listInput2D;
  fileInput.Close();

  const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(listOutput,filepathOutput);
  delete listOutput;
  std::cout<<std::endl;
}