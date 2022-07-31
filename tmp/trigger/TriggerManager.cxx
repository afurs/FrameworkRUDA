#include "TriggerManager.h"

/*******************************************************************************************************************/
//TriggerManager
/*******************************************************************************************************************/
TriggerManager::TriggerManager()	{
  //fV = new T0TreeVariables2;
  //fRunNum = RunNum;
  std::cout<<"\n////////////////////////////////////////////////////////////////";
  std::cout<<"\n/Initializating object TriggerManager...";
  //cout<<"\n/Type: "<<RunType;
  //cout<<"\n/Number of hits: "<<NHits;
  //cout<<"\n/Run number: "<<RunNum;
  std::cout<<"\n////////////////////////////////////////////////////////////////";
//	fTriggerConfig = NULL;
  std::cout<<"\n/Initialization complete!";
  std::cout<<"\n////////////////////////////////////////////////////////////////\n";

}
/*******************************************************************************************************************/
ULong64_t TriggerManager::getTriggerMask(TString triggerClass,Double_t *downscaleFactor,Bool_t *isNext50)	{
  TString stTrig;
  Int_t startChar=0;
  TList *listTrig = new TList();
  ///Proccesing trigger class string
  for(int iChar=0;iChar<triggerClass.Length();iChar++)	{
    stTrig = triggerClass(startChar,iChar-startChar);
    if(stTrig.BeginsWith(" "))	{
      startChar++;
      continue;
    }
    if((stTrig.EndsWith(" ")||iChar==triggerClass.Length()-1))	{
      stTrig = triggerClass(startChar,iChar-startChar-1);
      if(iChar==triggerClass.Length()-1)stTrig = triggerClass(startChar,iChar-startChar+1);
      listTrig->Add(new TObjString(stTrig));
      startChar = iChar;
    }
  }
  return getTriggerMask(listTrig,downscaleFactor,isNext50);
}
/*******************************************************************************************************************
void TriggerManager::InitTriggerConfig()	{
  AliCentralTrigger aCTP;
  TString configstr("");
  if (!aCTP.LoadConfiguration(configstr))	{
    cout<<"\n#Problem to load CTP configuration! RunNum: "<<fRunNum<<endl;
    cout<<"################################################################";
    return 0;
  }
  fTriggerConfig = aCTP.GetConfiguration();

}
*******************************************************************************************************************/

/*******************************************************************************************************************/
ULong64_t TriggerManager::getTriggerMask(TList *listTrig,Double_t *downscaleFactor,Bool_t *isNext50)	{
  std::cout<<"\n####################GETTING TRIGGER MASK########################\n";
  if(listTrig==NULL) {

    std::cout<<"\n#No such trigger list!\n";
    std::cout<<"\n################################################################";
    return 0;
  }
  if(!listTrig->GetSize()) {
    std::cout<<"\n#No entries in trigger list!\n";
    std::cout<<"\n################################################################";
    return 0;
  }
  connectOCDB(mRunNum);
  TString stTrig,stTrigBuf;
  TObjString *stObjTrig;
  TIter iterTrig(listTrig);
  //if(!fTriggerConfig) this->InitTriggerConfig();

  AliCentralTrigger aCTP;
  TString configstr("");
  if (!aCTP.LoadConfiguration(configstr))	{
    std::cout<<"\n#Problem to load CTP configuration! RunNum: "<<mRunNum;
    std::cout<<"\n################################################################";
    return 0;
  }
  AliTriggerConfiguration *TriggerConfig = aCTP.GetConfiguration();
  const TObjArray& classesArray = TriggerConfig->GetClasses();
  //classesArray->Print();
  AliTriggerClass *TrigClass;
  ULong64_t result = 0;
  Double_t ds = 0;
  TObjArrayIter iterClasses(&classesArray);
  TrigClass = (AliTriggerClass *)iterClasses.Next();
  //for(int iClass=0;iClass<classesArray.GetSize();iClass++)	{
  while(TrigClass)	{
    //TrigClass = (AliTriggerClass *)classesArray.At(iClass);
    stTrig = TrigClass->GetName();
    stObjTrig = (TObjString *)iterTrig.Next();
    //cout<<endl<<"Trig "<<classesArray.GetSize()<<" :"<<stTrig<<endl;
    while(stObjTrig)	{
      stTrigBuf = stObjTrig->GetString();
      if(stTrig.EqualTo(stTrigBuf))	{
        result = result|TrigClass->GetMask();
        if(TrigClass->GetMaskNext50())	{
          std::cout<<"\n#Warning! Trigger mask bit is higher than 50! Trigger:"<<TrigClass->GetName()<<" |Run: "<<mRunNum<<" |Bit:"<<TrigClass->GetMaskNext50()<<std::endl;
          //result = result|TrigClass->GetMaskNext50();
          TrigClass->GetDownscaleFactor(ds);
          if(isNext50)	*isNext50=kTRUE;
            return TrigClass->GetMaskNext50();
        }
        TrigClass->GetDownscaleFactor(ds);
        //cout<<endl<<"DS:"<<ds;
      }
      stObjTrig = (TObjString *)iterTrig.Next();
    }
    iterTrig.Reset();
    TrigClass = (AliTriggerClass *)iterClasses.Next();
    //if(result) break;
  }
  std::cout<<"\n#Triggers: ";
  listTrig->Print();
  std::cout<<"\n#Triggers mask: "<<result<<std::endl;
  //if(downscaleFactor)	{
    *downscaleFactor = ds;
    std::cout<<"#Downscale factor: "<<ds<<std::endl;
  //}
  if(!result) std::cout<<"#Warning! Trigger mask is zero!\n";
  std::cout<<"################################################################";
  return result;
}
/*******************************************************************************************************************/
AliCDBManager* TriggerManager::connectOCDB(unsigned int runNum)	{
  if(!gGrid)TGrid::Connect("alien://");
  if((AliCDBManager::Instance())->GetRun()==runNum) return AliCDBManager::Instance();
  AliCDBManager *man = AliCDBManager::Instance();
  man->SetDefaultStorage("raw://");
  if((AliCDBManager::Instance())->GetRun()!=runNum)	{
    //cout<<endl<<"Connecting..."<<endl;
    man->SetRun(runNum);
  }
  return man;
}
/*******************************************************************************************************************/
void TriggerManager::makeTreeTrgClasses(std::vector<unsigned int> &vecRunNums, TTree &treeTrgClasses) {
  if(vecRunNums.size()==0) {
    std::cout<<"\nWARNING! VECTOR WITH RUNNUMS IS EMPTY!\n";
    return;
  }
  unsigned int bufRunNum;
  treeTrgClasses.Branch("mRun",&bufRunNum);
  TObjArray objArrayBuf;
  treeTrgClasses.Branch("mTrgClasses",&objArrayBuf);

  TGrid::Connect("alien://");
  AliCDBManager *man = AliCDBManager::Instance();
  man->SetDefaultStorage("raw://");

  int count=0;
  for(const auto &runNum: vecRunNums)   {
    count++;
//    if(count>10) break;

    bufRunNum = runNum;
    std::cout<<"\nRUN: "<<bufRunNum<<std::endl;
    man->SetRun(bufRunNum);
    AliCentralTrigger aCTP;
    TString configstr("");
    try {
      aCTP.LoadConfiguration(configstr);
    }
    catch(const std::runtime_error& error) {
      std::cout<<"\n#Problem to load CTP configuration! RunNum: "<<bufRunNum;
      std::cout<<"\n################################################################\n";
      continue;
    }
    AliTriggerConfiguration *TriggerConfig = aCTP.GetConfiguration();
    const TObjArray& classesArray = TriggerConfig->GetClasses();
    TObjArray *objArrayTmp = (TObjArray *)classesArray.Clone();
    treeTrgClasses.SetBranchAddress("mTrgClasses",&objArrayTmp);
    treeTrgClasses.Fill();
    delete objArrayTmp;
  }
}
/*******************************************************************************************************************/
void TriggerManager::fillVecTrgClass(unsigned int runnum,TTree *treeTrgClasses)  {
  if(treeTrgClasses==nullptr) return;
  std::vector<TriggerClass> vecTrgClasses;
  mVecTrgClasses.clear();
  mVecTrgNames.clear();
  TObjArray *brPtrArrayTrgClasses = new TObjArray;
  unsigned int brRunNum;
  treeTrgClasses->SetBranchAddress("mRun",&brRunNum);
  treeTrgClasses->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);
  for(int iEvent=0;iEvent<treeTrgClasses->GetEntries();iEvent++) {
    treeTrgClasses->GetEntry(iEvent);
    if(runnum==brRunNum) {
      for(const auto *entry: (*brPtrArrayTrgClasses))  {
        const AliTriggerClass *aliTrgClass = dynamic_cast<const AliTriggerClass *>(entry);
        if(aliTrgClass==nullptr) continue;
        TriggerClass trgClass{std::string{aliTrgClass->GetName()},std::string{aliTrgClass->GetName()},
                              1,aliTrgClass->GetMask(),aliTrgClass->GetMaskNext50(),brRunNum};
        double ds=0;
        aliTrgClass->GetDownscaleFactor(ds);
        if(ds!=0) trgClass.mDownscaleFactor = ds;
        mVecTrgClasses.push_back(trgClass);
        mVecTrgNames.push_back(std::string{aliTrgClass->GetName()});
        mVecAllTrgNames.push_back(std::string{aliTrgClass->GetName()});
      }
    }
  }
}
/*******************************************************************************************************************/
void TriggerManager::reprocessTree(std::string pathToSrc, std::string pathToDest) {
  TFile *fileSrc = new TFile(pathToSrc.c_str(),"READ");
  TTree *trTrgCls = (TTree *) fileSrc->Get("Tree_TrgClasses");
  TObjArray *brPtrArrayTrgClasses = new TObjArray;
  unsigned int brRunNum;
  trTrgCls->SetBranchAddress("mRun",&brRunNum);
  trTrgCls->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);

  TTree *trTrgClsDest = new TTree("TreeTrgClasses");
  TTree *brTrTrgCls = new TTree("mTreeTrgClasses");
  trTrgClsDest->SetBranchAddress("mRun",&brRunNum);
  trTrgClsDest->SetBranchAddress("mTreeTrgClasses",&brTrTrgCls);
  ULong64_t brTrgMask,brTrgMaskNext50;
  TObjString brTrgClassName;
  double brDownscaleFactor;
  brTrTrgCls->SetBranchAddress("mTrgClassName",&brTrgClassName);
  brTrTrgCls->SetBranchAddress("mTrgMask",&brTrgMask);
  brTrTrgCls->SetBranchAddress("mTrgMaskNext50",&brTrgMaskNext50);
  brTrTrgCls->SetBranchAddress("mDownscaleFactor",&brDownscaleFactor);
  for(int iEvent=0;iEvent<trTrgCls->GetEntries();iEvent++) {
    trTrgCls->GetEntry(iEvent);
    trTrgCls->SetBranchAddress("mTrgClasses",&brPtrArrayTrgClasses);
    for(const auto *entry: (*brPtrArrayTrgClasses))  {
      const AliTriggerClass *aliTrgClass = dynamic_cast<const AliTriggerClass *>(entry);
      if(aliTrgClass==nullptr) continue;
      brTrgClassName.SetString(aliTrgClass->GetName());
      brTrgMask=aliTrgClass->GetMask();
      brTrgMaskNext50=aliTrgClass->GetMaskNext50();
      //Downscale factor
      brDownscaleFactor=0.;
      aliTrgClass->GetDownscaleFactor(brDownscaleFactor);
      if(brDownscaleFactor==0.) brDownscaleFactor=1;

      brTrTrgCls->Fill();
    }
    trTrgClsDest->Fill();
    brTrTrgCls->Reset();
  }
  delete trTrgCls;
  fileSrc->Close();
  delete fileSrc;
  TFile fileResult(pathToDest.c_str(),"RECREATE");
  fileResult.WriteObject(trTrgClsDest,trTrgClsDest.GetName(),"SingleKey");
  fileResult.Close();
  delete trTrgClsDest;
}
/*******************************************************************************************************************/
TriggerManager::~TriggerManager()	{
  std::cout<<"\n////////////////////////////////////////////////////////////////";
  std::cout<<"\n/Deleting object TriggerManager...";

  std::cout<<"\n////////////////////////////////////////////////////////////////";
  std::cout<<"\n/Deleting completed!";
  std::cout<<"\n////////////////////////////////////////////////////////////////\n";
}
/*******************************************************************************************************************/
