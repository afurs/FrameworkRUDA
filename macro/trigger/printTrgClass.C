#include "TGrid.h"
#include <vector>
//#include "AliTriggerClass.h"
//#include "AliCDBManager.h"
//#include "AliTriggerConfiguration.h"

void printTrgClass(std::string pathToSrc, int runnum=294241) {
  TFile *fileSrc = new TFile(pathToSrc.c_str(),"READ");
  TTree *trTrgCls = (TTree *) fileSrc->Get("Tree_TrgClasses");
  std::cout<<"\nNum of entries: "<<trTrgCls->GetEntries()<<std::endl;
  TObjArray *brPtrArrayTrgClasses = new TObjArray;
  TObjArray *brPtrArrayTrgInputs = new TObjArray;
  unsigned int brRunNum;
  trTrgCls->SetBranchAddress("mRun",&brRunNum);
  trTrgCls->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);

  trTrgCls->SetBranchAddress("mTrgInputs",&brPtrArrayTrgInputs);
  std::set<std::string> setTrgInputs;
  std::set<std::string> setTrgClasses;
  ULong64_t brTrgMask,brTrgMaskNext50;
  TObjString brTrgClassName;
  //TObjString *trgClassName = new TObjString;

  for(int iEvent=0;iEvent<trTrgCls->GetEntries();iEvent++) {
    trTrgCls->GetEntry(iEvent);
    //if(brRunNum!=runnum) continue;
    cout<<"\n===============================================\n";
//    trTrgCls->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);

//    brTrTrgCls->SetBranchAddress("mTrgClassName",&trgClassName);
//    brTrTrgCls->SetBranchAddress("mTrgMask",&brTrgMask);
//    brTrTrgCls->SetBranchAddress("mTrgMaskNext50",&brTrgMaskNext50);
//    brTrTrgCls->SetBranchAddress("mDownscaleFactor",&brDownscaleFactor);
    //brPtrArrayTrgInputs->Print();
    std::cout<<"\nRunnum: "<<brRunNum<<std::endl;
    for(const auto *entry: (*brPtrArrayTrgInputs)) {
      const AliTriggerInput *aliTrgInput = dynamic_cast<const AliTriggerInput *>(entry);
      setTrgInputs.insert(std::string{aliTrgInput->GetInputName()});
      aliTrgInput->Print();
      //cout<<endl<<aliTrgInput->GetInputName()<<"|"<<std::hex<<aliTrgInput->GetMask()<<"|"<<static_cast<int>(aliTrgInput->GetLevel())<<std::dec<<endl;
    }
    cout<<"\n..............................................................................\n";
    for(const auto *entry: (*brPtrArrayTrgClasses))  {
      const AliTriggerClass *aliTrgClass = dynamic_cast<const AliTriggerClass *>(entry);
      if(aliTrgClass==nullptr) continue;aliTrgClass->Print("");
      setTrgClasses.insert(std::string{aliTrgClass->GetName()});
      brTrgClassName.SetString(aliTrgClass->GetName());
      //cout<<endl<<brTrgClassName.GetString()<<endl;
      brTrgMask=aliTrgClass->GetMask();
      brTrgMaskNext50=aliTrgClass->GetMaskNext50();
      cout<<endl<<brTrgClassName.GetString()<<"|"<<std::hex<<brTrgMask<<"|"<<brTrgMaskNext50<<std::dec<<endl;

    }


    cout<<"\n===============================================\n";
  }
  cout<<"\n============================================================================\n";
  for(const auto&entry: setTrgInputs) cout<<endl<<entry;
  cout<<"\n----------------------------------------------------------------------------\n";
  for(const auto&entry: setTrgClasses) cout<<endl<<entry;
  cout<<"\n============================================================================\n";
  delete trTrgCls;
  fileSrc->Close();
  delete fileSrc;
}
