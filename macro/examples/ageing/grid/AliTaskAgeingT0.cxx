#include "AliTaskAgeingT0.h"

ClassImp(AliTaskAgeingT0)
/*******************************************************************************************************************/
AliTaskAgeingT0::AliTaskAgeingT0(const char *name):AliAnalysisTaskSE(name)
  ,fESD(0x0), fOutputList(0x0),mEventStruct{}
  ,mVecHists_v0{}
  ,mVecHistsC0TVX_v0{},mVecHistsCINT7_v0{}
  ,mVecHistsC0TVX_v1{},mVecHistsCINT7_v1{}
  ,mVecHistsC0TVX_v2{},mVecHistsCINT7_v2{}
  ,mVecHistsC0TVX_v3{},mVecHistsCINT7_v3{}
  ,mVecHistsC0TVX_v4{},mVecHistsCINT7_v4{}

{
  /// Constructor

  /// Define input and output slots here
  /// Input slot #0 works with a TChain
	DefineInput(0, TChain::Class());
  /// Output slot #0 id reserved by the base class for AOD
  /// Output slot #1 writes into a TH1 container
	DefineOutput(1, TList::Class());
}
/*******************************************************************************************************************/
AliTaskAgeingT0::~AliTaskAgeingT0()
{
// Destructor
	if (fOutputList)	delete fOutputList;
}
/*******************************************************************************************************************/
Bool_t AliTaskAgeingT0::UserNotify()	{
///
/// Calls the mother class Notify()
///
	return AliAnalysisTaskSE::UserNotify();
}
/*******************************************************************************************************************
void AliT0TreeTaskMy::NotifyRun()
{
	
	//AliESDEvent* evESD = dynamic_cast<AliESDEvent*>(InputEvent());
	fRunNum = InputEvent()->GetRunNumber();
	TGrid::Connect("alien://");
	AliCDBManager *man = AliCDBManager::Instance();
	man->SetDefaultStorage("raw://");
	man->SetRun(fRunNum);
	AliCentralTrigger aCTP;
	TString configstr("");
	if (!aCTP.LoadConfiguration(configstr))	{
	    cout<<"\n#Problem to load CTP configuration! RunNum: "<<fRunNum<<endl;
	    cout<<"################################################################";
	}
	AliTriggerConfiguration *config = aCTP.GetConfiguration();
	const TObjArray& classesArray = config->GetClasses();
	AliTriggerClass *TrigClass;
	TString stTrig;
	TObjArrayIter iterObjArray(&classesArray);
	while(TrigClass = (AliTriggerClass *)iterObjArray())	{
	//for(int iClass=0;iClass<classesArray.GetSize();iClass++)	{
	    //TrigClass = (AliTriggerClass *)classesArray.At(iClass);
	    stTrig = TrigClass->GetName();
	    if(stTrig.Contains("CADAND-B"))	{
		fMaskCADAND_B = fMaskCADAND_B|TrigClass->GetMask();
	    }
	}
	
}
*******************************************************************************************************************/
void AliTaskAgeingT0::PrepareOutput(TString nameList,const std::vector<Hist_t *> &vecHists) {
  if(fOutputList==nullptr) return;
  TList *listBuf = dynamic_cast<TList *> (fOutputList->FindObject(nameList));
  if(listBuf == nullptr) {
    listBuf = new TList();
    listBuf->SetName(nameList);
    listBuf->SetOwner(kTRUE);
    fOutputList->Add(listBuf);
  }
  for(const auto &entry:vecHists) {
    //entry->Print();
    listBuf->Add(entry);
  }
}
/*******************************************************************************************************************/
void AliTaskAgeingT0::MakeHistOutput(std::vector<Hist_t *> &vecHists
                                     , std::string nameHist
                                     , std::string titleHist
                                     , const std::vector<HistParameters> &vecParams)
{
    std::vector<std::string> vecNameDescr{"hAmp_","hAmpNew_","hTime_"};
    for(int iHit=0;iHit<5;iHit++) {
      vecNameDescr.push_back(Form("hTimeFull_hit%i_",iHit+1));
      vecNameDescr.push_back(Form("hTimeFull_zoom_hit%i_",iHit+1));
      vecNameDescr.push_back(Form("hTrg_zoom_hit%i_",iHit+1));
    }
    std::vector<std::string> vecTitleDescr{"Amp, ","AmpNew, ","Time, "};
    for(int iHit=0;iHit<5;iHit++) {
      vecTitleDescr.push_back(Form("Time full hit%i,",iHit+1));
      vecTitleDescr.push_back(Form("Time full(zoom) hit%i,",iHit+1));
      vecTitleDescr.push_back(Form("Trigger spectrum hit%i,",iHit+1));

    }
    for(std::size_t iDescr=0;iDescr<vecNameDescr.size();iDescr++)  {
      vecHists.push_back(new Hist_t((vecNameDescr[iDescr]+nameHist).c_str()
                                    ,(vecTitleDescr[iDescr]+titleHist).c_str()
                                    ,vecParams[iDescr].nBinsX,vecParams[iDescr].lowBinX,vecParams[iDescr].upBinX
                                    ,vecParams[iDescr].nBinsY,vecParams[iDescr].lowBinY,vecParams[iDescr].upBinY));

    }
}
/*******************************************************************************************************************/
void AliTaskAgeingT0::UserCreateOutputObjects()	{
  /// Create histograms
  /// Called once
	fOutputList = new TList();
  fOutputList->SetOwner(kTRUE); /// Will delete the histos on cleanup
  HistParameters paramsAmp{24,0,24,4000,0,4000};
  HistParameters paramsAmpNew{24,0,24,4000,0,400};
  HistParameters paramsTime{24,0,24,4000,8000,12000};
  HistParameters paramsTimeFull{24,0,24,4000,-10000,10000};
  HistParameters paramsTimeFull_zoom{24,0,24,10000,-50,50};
  HistParameters paramsTrg_zoom{3,0,3,10000,-50,50};

  std::vector<HistParameters> vecParams{paramsAmp,paramsAmpNew,paramsTime};
  for(int iHit=0;iHit<5;iHit++) {
    vecParams.push_back(paramsTimeFull);
    vecParams.push_back(paramsTimeFull_zoom);
    vecParams.push_back(paramsTrg_zoom);
  }

  //No cuts

  MakeHistOutput(mVecHists_v0
                 ,"_v0"
                 ,";Channel"
                 ,vecParams);
  PrepareOutput("hists_v0",mVecHists_v0);
  //C0TVX+CINT7
  MakeHistOutput(mVecHistsC0TVX_v0
                 ,"C0TVX_v0"
                 ,"C0TVX+CINT7;Channel"
                 ,vecParams);
  PrepareOutput("hists_v0",mVecHistsC0TVX_v0);
  //CINT7
  MakeHistOutput(mVecHistsCINT7_v0
                 ,"CINT7_v0"
                 ,"CINT7;Channel"
                 ,vecParams);
  PrepareOutput("hists_v0",mVecHistsCINT7_v0);

  //Version 1
  //C0TVX+CINT7
  MakeHistOutput(mVecHistsC0TVX_v1
                 ,"C0TVX_v1"
                 ,"C0TVX+CINT7, vtxTrk(-+30cm, cont>1),pileupLowMultSPD,physSel,V0time_v2;Channel"
                 ,vecParams);
  PrepareOutput("hists_v1",mVecHistsC0TVX_v1);
  //CINT7
  MakeHistOutput(mVecHistsCINT7_v1
                 ,"CINT7_v1"
                 ,"CINT7, vtxTrk(-+30cm, cont>1),pileupLowMultSPD,physSel,V0time_v2;Channel"
                 ,vecParams);
  PrepareOutput("hists_v1",mVecHistsCINT7_v1);

  //Version 2
  //C0TVX+CINT7
  MakeHistOutput(mVecHistsC0TVX_v2
                 ,"C0TVX_v2"
                 ,"C0TVX+CINT7, vtxTrk(-+30cm, cont>1),pileupSPD,physSel,V0time_v2"
                 ,vecParams);
  PrepareOutput("hists_v2",mVecHistsC0TVX_v2);
  //CINT7
  MakeHistOutput(mVecHistsCINT7_v2
                 ,"CINT7_v2"
                 ,"CINT7, vrtTrk(-+30cm, cont>1),pileupSPD,physSel,V0time_v2"
                 ,vecParams);
  PrepareOutput("hists_v2",mVecHistsCINT7_v2);
  //Version 3
  //C0TVX+CINT7
  MakeHistOutput(mVecHistsC0TVX_v3
                 ,"C0TVX_v3"
                 ,"C0TVX+CINT7, vtxGlobal(-+10cm, cont>0),pileupSPD,physSel,V0time_v2"
                 ,vecParams);
  PrepareOutput("hists_v3",mVecHistsC0TVX_v3);
  //CINT7
  MakeHistOutput(mVecHistsCINT7_v3
                 ,"CINT7_v3"
                 ,"CINT7, vtxGlobal(-+10cm, cont>0),pileupSPD,physSel,V0time_v2"
                 ,vecParams);
  PrepareOutput("hists_v3",mVecHistsCINT7_v3);
  //Version 4
  //C0TVX+CINT7
  MakeHistOutput(mVecHistsC0TVX_v4
                 ,"C0TVX_v4"
                 ,"C0TVX+CINT7, vtxGlobal(-+10cm, cont>0),pileupSPD,physSel,V0time_v1"
                 ,vecParams);
  PrepareOutput("hists_v4",mVecHistsC0TVX_v4);
  //CINT7
  MakeHistOutput(mVecHistsCINT7_v4
                 ,"CINT7_v4"
                 ,"CINT7, vtxGlobal(-+10cm, cont>0),pileupSPD,physSel,V0time_v1"
                 ,vecParams);
  PrepareOutput("hists_v4",mVecHistsCINT7_v4);



	PostData(1, fOutputList);
}
/*******************************************************************************************************************/
void AliTaskAgeingT0::FillT0data(const std::vector<Hist_t *> &vecHists) {
  const AliESDTZERO* esdT0= dynamic_cast<const AliESDTZERO*> (fESD->GetESDTZERO());
  if(esdT0!=nullptr)	{
    int timeFullID = 3;
    const Double32_t *time=esdT0->GetT0time();
    const  Double32_t *amp=esdT0->GetT0amplitude();
    const  Double32_t *ampNew=esdT0->GetT0NewAmplitude();
    for(int iHit=0;iHit<5;iHit++)  {

      Double_t orA = esdT0->GetOrA(iHit);
      Double_t orC = esdT0->GetOrC(iHit);
      Double_t tvdc = esdT0->GetTVDC(iHit);

      if(orA) vecHists[3*iHit+2+timeFullID]->Fill(0.,orA);
      if(orC) vecHists[3*iHit+2+timeFullID]->Fill(1.,orC);
      if(tvdc) vecHists[3*iHit+2+timeFullID]->Fill(2.,tvdc);
    }
    for(int iCh=0;iCh<24;iCh++) {
      if(amp[iCh]>0)vecHists[0]->Fill(iCh,amp[iCh]);
      if(ampNew[iCh]>0)vecHists[1]->Fill(iCh,ampNew[iCh]);
      if(time[iCh]>0) vecHists[2]->Fill(iCh,time[iCh]);
      for(int iHit=0;iHit<5;iHit++)  {
        Float_t timeFull=esdT0->GetTimeFull(iCh, iHit);

        //vecHists[2*iHit+timeFullID]->Fill(iCh,timeFull);
        //vecHists[2*iHit+1+timeFullID]->Fill(iCh,timeFull);
        /*if(timeFull!=0.)*/ vecHists[3*iHit+timeFullID]->Fill(iCh,timeFull);
        /*if(timeFull!=-9999&&timeFull!=0.)*/
        if(timeFull!=0.) vecHists[3*iHit+1+timeFullID]->Fill(iCh,timeFull);

      }
    }
  }
}
/*******************************************************************************************************************/
void AliTaskAgeingT0::UserExec(Option_t *)	{
  /// Main loop
  /// Called for each event
  fESD = dynamic_cast<AliESDEvent*>(InputEvent());
  if(fESD!=nullptr)	{
    AliAnalysisManager *man=AliAnalysisManager::GetAnalysisManager();
    AliInputEventHandler* inputHandler = dynamic_cast<AliInputEventHandler*>(man->GetInputEventHandler());
    mEventStruct.clear();
    //V0 timing
    AliESDVZERO* esdV0 = dynamic_cast<AliESDVZERO*>(fESD->GetVZEROData());

    if(esdV0!=nullptr)	{
       mEventStruct.fTimeV0C = esdV0->GetV0CTime();
       mEventStruct.fTimeV0A = esdV0->GetV0ATime();
    }

    auto cutV0_v1 = [](const EventStruct &data)->bool {
      return ((data.fTimeV0A+data.fTimeV0C)>11.5) && ((data.fTimeV0A+data.fTimeV0C)<17.5)
              && ((data.fTimeV0A-data.fTimeV0C)>5.5) &&  ((data.fTimeV0A-data.fTimeV0C)<11.5);
    };
    auto cutV0_v2 = [](const EventStruct &data)->bool {
      return ((data.fTimeV0A+data.fTimeV0C)>10) && ((data.fTimeV0A+data.fTimeV0C)<18)
              && ((data.fTimeV0A-data.fTimeV0C)>4) &&  ((data.fTimeV0A-data.fTimeV0C)<12);
    };

    //Pileup
    mEventStruct.fIsPileup = fESD->IsPileupFromSPDInMultBins();
    mEventStruct.fIsPileupLowMult = fESD->IsPileupFromSPD(3.,0.8,3.,2.,5.);

    auto cutNoPileup = [] (const EventStruct &data)->bool {return !data.fIsPileup;};
    auto cutNoPileupLowMult = [] (const EventStruct &data)->bool {return !data.fIsPileupLowMult;};

    //Physics selection
    if(inputHandler!=nullptr) {
      mEventStruct.fIsPhysSel = inputHandler->IsEventSelected();
    }
    auto cutPhysSel = [] (const EventStruct &data)->bool {return data.fIsPhysSel;};

    //Event ID
    mEventStruct.fOrbit=fESD->GetOrbitNumber();
    mEventStruct.fBC=fESD->GetBunchCrossNumber();
    //Global vertex

    const AliESDVertex *esdVertexGlobal = dynamic_cast<const AliESDVertex *>(fESD->GetPrimaryVertex());
    if(esdVertexGlobal!=nullptr)	{
        mEventStruct.fNcontGlobal = esdVertexGlobal->GetNContributors();
        mEventStruct.fVertexGlobal_Z=esdVertexGlobal->GetZ();
        mEventStruct.fVtxGlobalResZ = esdVertexGlobal->GetZRes();
    }
    auto cutVertexGlobal = [] (const EventStruct &data)->bool {return (data.fNcontGlobal>0)
          &&(data.fVertexGlobal_Z>-10)
          &&(data.fVertexGlobal_Z<10);
    };


    //Track vertex
    const AliESDVertex *esdVertexTrack = dynamic_cast<const AliESDVertex *>(fESD->GetPrimaryVertexTracks());
    if(esdVertexTrack!=nullptr)	{
        mEventStruct.fNcontTrack = esdVertexTrack->GetNContributors();
        mEventStruct.fVertexTrack_Z=esdVertexTrack->GetZ();
        mEventStruct.fVtxTrackResZ = esdVertexTrack->GetZRes();
    }
    auto cutVertexTrack = [] (const EventStruct &data)->bool { return (data.fNcontTrack>1)
          &&(data.fVertexTrack_Z>-30)
          &&(data.fVertexTrack_Z<30);
    };
    //CTP triggers
    mEventStruct.fTrigger.SetString(fESD->GetFiredTriggerClasses());
    mEventStruct.fTriggerMask = fESD->GetTriggerMask();
    mEventStruct.fTriggerMaskNext50 = fESD->GetTriggerMaskNext50();
    auto cutIsC0TVX = [](const EventStruct &data)->bool {
      TString trg = data.fTrigger.GetString();
      return (trg.Contains("C0TVX-B")&&trg.Contains("CINT7-B"));
    };
    auto cutIsCINT7 = [](const EventStruct &data)->bool {
      TString trg = data.fTrigger.GetString();
      return trg.Contains("CINT7-B");
    };
    //T0 trigger
    mEventStruct.fIsT0fired = fESD->GetHeader()->IsTriggerInputFired("0TVX");
    //Filling data
    bool isVersion1 = cutVertexTrack(mEventStruct)&&cutNoPileupLowMult(mEventStruct)
        &&cutPhysSel(mEventStruct)&&cutV0_v2(mEventStruct);
    bool isVersion2 = cutVertexTrack(mEventStruct)&&cutNoPileup(mEventStruct)
        &&cutPhysSel(mEventStruct)&&cutV0_v2(mEventStruct);
    bool isVersion3 = cutVertexGlobal(mEventStruct)&&cutNoPileup(mEventStruct)
        &&cutPhysSel(mEventStruct)&&cutV0_v2(mEventStruct);
    bool isVersion4 = cutVertexGlobal(mEventStruct)&&cutNoPileup(mEventStruct)
        &&cutPhysSel(mEventStruct)&&cutV0_v1(mEventStruct);
    FillT0data(mVecHists_v0);
    if(cutIsC0TVX(mEventStruct))  {
      FillT0data(mVecHistsC0TVX_v0);
      if(isVersion1) FillT0data(mVecHistsC0TVX_v1);
      if(isVersion2) FillT0data(mVecHistsC0TVX_v2);
      if(isVersion3) FillT0data(mVecHistsC0TVX_v3);
      if(isVersion4) FillT0data(mVecHistsC0TVX_v4);
    }
    if(cutIsCINT7(mEventStruct))  {
      FillT0data(mVecHistsCINT7_v0);
      if(isVersion1) FillT0data(mVecHistsCINT7_v1);
      if(isVersion2) FillT0data(mVecHistsCINT7_v2);
      if(isVersion3) FillT0data(mVecHistsCINT7_v3);
      if(isVersion4) FillT0data(mVecHistsCINT7_v4);
    }
  }
  else {
    printf("ERROR: fESD not available\n");
    return;
  }
	PostData(1, fOutputList);
}      
/*******************************************************************************************************************/
void AliTaskAgeingT0::Terminate(Option_t *)	{
  // Draw result to the screen
  // Called once at the end of the query

  /* fOutputList = dynamic_cast<TList*> (GetOutputData(1));
  if (!fOutputList) {
    printf("ERROR: Output list not available\n");
    return;
  }
    */  
}
