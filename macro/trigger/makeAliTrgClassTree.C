#include "TGrid.h"
#include <vector>
//#include "AliTriggerClass.h"
//#include "AliCDBManager.h"
//#include "AliTriggerConfiguration.h"

void makeAliTrgClassTree() {
  TTree *ttreeRunNums=new TTree;
  std::string filepathRunNums = "input/RunPhys2016.csv";
  std::string filepathResult = "TrgClasses2016.root";
  
  ttreeRunNums->ReadFile(filepathRunNums.c_str());
  ttreeRunNums->GetListOfLeaves()->Print();
  unsigned int bufRunNum;
  ttreeRunNums->SetBranchAddress("run",&bufRunNum);
  std::vector<unsigned int> vecRunNums;
  for(int iEvent=0;iEvent<ttreeRunNums->GetEntries();iEvent++) {
    ttreeRunNums->GetEntry(iEvent);
    vecRunNums.push_back(bufRunNum);
  }
  delete ttreeRunNums;
  std::sort(vecRunNums.begin(),vecRunNums.end());
  for(const auto &el:vecRunNums) {
    cout<<endl<<el<<endl;
  }
  //cout<<endl<<"hello"<<endl;
  
  TFile fileResult(filepathResult.c_str(),"RECREATE");
  TTree treeResult("Tree_TrgClasses","Tree_TrgClasses");

  treeResult.Branch("mRun",&bufRunNum);
  TObjArray objArrayBuf;
  treeResult.Branch("mTrgClasses",&objArrayBuf);
  
  TGrid::Connect("alien://");
  AliCDBManager *man = AliCDBManager::Instance();
  man->SetDefaultStorage("raw://");
  
  int count=0;
  for(const auto &runNum: vecRunNums)	{
    count++;
//    if(count>10) break;
    
    bufRunNum = runNum;
    cout<<endl<<"RUN: "<<bufRunNum<<endl;
    man->SetRun(bufRunNum);
    AliCentralTrigger aCTP;
    TString configstr("");
    try {
      aCTP.LoadConfiguration(configstr);
    }
    catch(const runtime_error& error) {
      cout<<"\n#Problem to load CTP configuration! RunNum: "<<bufRunNum<<endl;
      cout<<"################################################################\n";
      continue;
    }
    /*
    if (!aCTP.LoadConfiguration(configstr))	{
      cout<<"\n#Problem to load CTP configuration! RunNum: "<<bufRunNum<<endl;
      cout<<"################################################################";
      continue;
    }
    */
    AliTriggerConfiguration *TriggerConfig = aCTP.GetConfiguration();
    const TObjArray& classesArray = TriggerConfig->GetClasses();
    TObjArray *objArrayTmp = (TObjArray *)classesArray.Clone();
    treeResult.SetBranchAddress("mTrgClasses",&objArrayTmp);
    treeResult.Fill();
    delete objArrayTmp;
 
  }
//  treeResult.Write();
  fileResult.WriteObject(&treeResult,treeResult.GetName(),"SingleKey");
  fileResult.Close();
}