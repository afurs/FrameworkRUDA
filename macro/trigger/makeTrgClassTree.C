#include "TGrid.h"
#include <vector>
//#include "AliTriggerClass.h"
//#include "AliCDBManager.h"
//#include "AliTriggerConfiguration.h"

void makeTrgClassTree(std::string pathToSrc, std::string pathToDest) {
  TFile *fileSrc = new TFile(pathToSrc.c_str(),"READ");
  TTree *trTrgCls = (TTree *) fileSrc->Get("Tree_TrgClasses");
  std::cout<<"\nNum of entries: "<<trTrgCls->GetEntries()<<std::endl;
  TObjArray *brPtrArrayTrgClasses = new TObjArray;
  TObjArray *brPtrArrayTrgInputs = new TObjArray;
  unsigned int brRunNum;
  trTrgCls->SetBranchAddress("mRun",&brRunNum);
  trTrgCls->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);

  trTrgCls->SetBranchAddress("mTrgInputs",&brPtrArrayTrgInputs);

  TTree *trTrgClsDest = new TTree("TreeTrgClasses","TreeTrgClasses");
  trTrgClsDest->SetDirectory(0);
  TTree *brTrTrgCls = new TTree("mTreeTrgClasses","mTreeTrgClasses");
  brTrTrgCls->SetDirectory(0);
  TTree *brTrTrgInputs = new TTree("mTreeTrgInputs","mTreeTrgInputs");
  brTrTrgInputs->SetDirectory(0);
  trTrgClsDest->Branch("mRun",&brRunNum);
  trTrgClsDest->Branch("mTreeTrgClasses",&brTrTrgCls);
  trTrgClsDest->Branch("mTreeTrgInputs",&brTrTrgInputs);
  ULong64_t brTrgMask,brTrgMaskNext50;
  TObjString brTrgClassName;
  //TObjString *trgClassName = new TObjString;

  double brDownscaleFactor;
  brTrTrgCls->Branch("mTrgClassName",&brTrgClassName);
  brTrTrgCls->Branch("mTrgMask",&brTrgMask);
  brTrTrgCls->Branch("mTrgMaskNext50",&brTrgMaskNext50);
  brTrTrgCls->Branch("mDownscaleFactor",&brDownscaleFactor);
  for(int iEvent=0;iEvent<trTrgCls->GetEntries();iEvent++) {
    trTrgCls->GetEntry(iEvent);
    cout<<"\n===============================================\n";
//    trTrgCls->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);

//    brTrTrgCls->SetBranchAddress("mTrgClassName",&trgClassName);
//    brTrTrgCls->SetBranchAddress("mTrgMask",&brTrgMask);
//    brTrTrgCls->SetBranchAddress("mTrgMaskNext50",&brTrgMaskNext50);
//    brTrTrgCls->SetBranchAddress("mDownscaleFactor",&brDownscaleFactor);
    //brPtrArrayTrgInputs->Print();
    for(const auto *entry: (*brPtrArrayTrgInputs)) {
      const AliTriggerInput *aliTrgInput = dynamic_cast<const AliTriggerInput *>(entry);
      aliTrgInput->Print();
      //cout<<endl<<aliTrgInput->GetInputName()<<"|"<<std::hex<<aliTrgInput->GetMask()<<"|"<<static_cast<int>(aliTrgInput->GetLevel())<<std::dec<<endl;
    }
    cout<<"\n..............................................................................\n";
    for(const auto *entry: (*brPtrArrayTrgClasses))  {
      const AliTriggerClass *aliTrgClass = dynamic_cast<const AliTriggerClass *>(entry);
      if(aliTrgClass==nullptr) continue;aliTrgClass->Print("");
      brTrgClassName.SetString(aliTrgClass->GetName());
      //cout<<endl<<brTrgClassName.GetString()<<endl;
      brTrgMask=aliTrgClass->GetMask();
      brTrgMaskNext50=aliTrgClass->GetMaskNext50();
      cout<<endl<<brTrgClassName.GetString()<<"|"<<std::hex<<brTrgMask<<"|"<<brTrgMaskNext50<<std::dec<<endl;
      //Downscale factor
      brDownscaleFactor=0.;
      aliTrgClass->GetDownscaleFactor(brDownscaleFactor);
      if(brDownscaleFactor==0.) brDownscaleFactor=1;
      brTrTrgCls->Fill();
    }
    trTrgClsDest->Fill();
    std::cout<<"\nRunnum: "<<brRunNum<<" | Num of entries: "<<brTrTrgCls->GetEntries()<<std::endl;
    brTrTrgCls->Reset();
    cout<<"\n===============================================\n";
  }
  delete trTrgCls;
  fileSrc->Close();
  delete fileSrc;
  TFile fileResult(pathToDest.c_str(),"RECREATE");
  fileResult.WriteObject(trTrgClsDest,trTrgClsDest->GetName(),"SingleKey");
  fileResult.Close();
  delete trTrgClsDest;
}
