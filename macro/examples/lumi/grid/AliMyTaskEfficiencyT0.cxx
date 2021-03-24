#include "AliMyTaskEfficiencyT0.h"

ClassImp(AliMyTaskEfficiencyT0)
/*******************************************************************************************************************/
AliMyTaskEfficiencyT0::AliMyTaskEfficiencyT0(const char *name):AliAnalysisTaskSE(name)
  ,fESD(0x0), fOutputList(0x0), fT0OutTree(0x0)
  ,fTimeV0A(-9999),fTimeV0C(-9999),fIsNullPtrV0(kFALSE)
  ,fIsPileup(kFALSE),fIsPileupLowMult(kFALSE)
  ,fIsPhysSel(kFALSE),fIsPhysSel_v2(kFALSE),fIsNullPtrInputHandler(kFALSE)
  ,fNcontSPD(-1),fVertexSPD_X(-9999),fVertexSPD_Y(-9999),fVertexSPD_Z(-9999),fIsNullPtrVtxSPD(kFALSE),fVtxSPDresZ(0)
  ,fNcontTrack(-1),fVertexTrack_Z(-9999),fVertexTrack_Y(-9999),fVertexTrack_X(-9999),fIsNullPtrVtxTrack(kFALSE),fVtxTrackResZ(0)
  ,fNcontGlobal(-1),fVertexGlobal_Z(-9999),fIsNullPtrVtxGlobal(kFALSE),fVtxGlobalResZ(0)
  ,fOrbit(0),fBC(0)
  ,fCentralityV0M(300)
  ,fTriggerT0(0),fIsT0fired(kFALSE),fBitsT0timeCounter(24)
  ,fTrigger(0),fTriggerMask(0),fTriggerMaskNext50(0)
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
AliMyTaskEfficiencyT0::~AliMyTaskEfficiencyT0()
{
// Destructor
	if (fOutputList)	delete fOutputList;
}
/*******************************************************************************************************************/
Bool_t AliMyTaskEfficiencyT0::UserNotify()	{
///
/// Calls the mother class Notify()
///
	return AliAnalysisTaskSE::UserNotify();
}
/*******************************************************************************************************************/
void AliMyTaskEfficiencyT0::NotifyRun()
{
  //for (int iCh=0; iCh<24; iCh++) fTimePeak[iCh] = 8000;
	//AliESDEvent* evESD = dynamic_cast<AliESDEvent*>(InputEvent());

  auto runNum = InputEvent()->GetRunNumber();
	TGrid::Connect("alien://");
	AliCDBManager *man = AliCDBManager::Instance();
	man->SetDefaultStorage("raw://");
  man->SetRun(runNum);
  AliCDBEntry *entryCDB = AliCDBManager::Instance()->Get("T0/Calib/TimeDelay");
  AliT0CalibTimeEq *clb = dynamic_cast<AliT0CalibTimeEq*>(entryCDB->GetObject());
  if(clb==nullptr) return;
  for (int iCh=0; iCh<24; iCh++) {
    fTimePeak[iCh] = clb->GetCFDvalue(iCh,0);
  }

  /*
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
  */
}
/*******************************************************************************************************************/
void AliMyTaskEfficiencyT0::UserCreateOutputObjects()	{
  /// Create histograms
  /// Called once
	fOutputList = new TList();
	fOutputList->SetOwner(); /// Will delete the histos on cleanup
	fT0OutTree = new TTree("t0tree","None here");
  //V0 timing
	fT0OutTree->Branch("fTimeV0A", &fTimeV0A);
	fT0OutTree->Branch("fTimeV0C", &fTimeV0C);
  //fT0OutTree->Branch("fIsNullPtrV0",&fIsNullPtrV0);
  //Pileup
	fT0OutTree->Branch("fIsPileup", &fIsPileup);
	fT0OutTree->Branch("fIsPileupLowMult", &fIsPileupLowMult);
  //Physics selection
  fT0OutTree->Branch("fIsPhysSel",&fIsPhysSel);
  //fT0OutTree->Branch("fIsPhysSel_v2",&fIsPhysSel_v2);
  //fT0OutTree->Branch("fIsNullPtrInputHandler",&fIsNullPtrInputHandler);
  //Vertex SPD
  fT0OutTree->Branch("fNcontSPD",&fNcontSPD);
  fT0OutTree->Branch("fVertexSPD_Z",&fVertexSPD_Z);
  fT0OutTree->Branch("fVertexSPD_Y",&fVertexSPD_Y);
  fT0OutTree->Branch("fVertexSPD_X",&fVertexSPD_X);

  //fT0OutTree->Branch("fIsNullPtrVtxSPD",&fIsNullPtrVtxSPD);
  //fT0OutTree->Branch("fVtxSPDresZ",&fVtxSPDresZ);
  //Vertex Track
  fT0OutTree->Branch("fNcontTrack",&fNcontTrack);
  fT0OutTree->Branch("fVertexTrack_Z",&fVertexTrack_Z);
  fT0OutTree->Branch("fVertexTrack_Y",&fVertexTrack_Y);
  fT0OutTree->Branch("fVertexTrack_X",&fVertexTrack_X);
  //fT0OutTree->Branch("fIsNullPtrVtxTrack",&fIsNullPtrVtxTrack);
  //fT0OutTree->Branch("fVtxTrackResZ",&fVtxTrackResZ);
  //Vertex Global
  fT0OutTree->Branch("fNcontGlobal",&fNcontGlobal);
  fT0OutTree->Branch("fVertexGlobal_Z",&fVertexGlobal_Z);
  //fT0OutTree->Branch("fIsNullPtrVtxGlobal",&fIsNullPtrVtxGlobal);
  //fT0OutTree->Branch("fVtxGlobalResZ",&fVtxGlobalResZ);
  //Event ID
  fT0OutTree->Branch("fOrbit", &fOrbit);
  fT0OutTree->Branch("fBC", &fBC);
  //Centrality (not used)
//	fT0OutTree->Branch("fCentralityV0M", &fCentralityV0M);
  //T0 trigger
  fT0OutTree->Branch("fTriggerT0", &fTriggerT0);
  //fT0OutTree->Branch("fIsT0fired", &fIsT0fired);
  //fBitsT0timeCounter = TBits(10);
  fT0OutTree->Branch("fBitsT0timeCounter",&fBitsT0timeCounter);
  //CTP trigger
  fT0OutTree->Branch("fTriggerMask", &fTriggerMask);
  fT0OutTree->Branch("fTriggerMaskNext50", &fTriggerMaskNext50);

	fOutputList->Add(fT0OutTree);
	PostData(1, fOutputList);
}
/*******************************************************************************************************************/
void AliMyTaskEfficiencyT0::UserExec(Option_t *)	{
  /// Main loop
  /// Called for each event
  fESD = dynamic_cast<AliESDEvent*>(InputEvent());
  if(fESD!=nullptr)	{
    AliAnalysisManager *man=AliAnalysisManager::GetAnalysisManager();
    AliInputEventHandler* inputHandler = dynamic_cast<AliInputEventHandler*>(man->GetInputEventHandler());
    //V0 timing
    fTimeV0A = -9999;
    fTimeV0C = -9999;
    fIsNullPtrV0 = kTRUE;
    AliESDVZERO* esdV0 = dynamic_cast<AliESDVZERO*>(fESD->GetVZEROData());
    if(esdV0!=nullptr)	{
      fTimeV0C = esdV0->GetV0CTime();
      fTimeV0A = esdV0->GetV0ATime();
      fIsNullPtrV0 = kFALSE;
    }
    //Pileup
    fIsPileup = kFALSE;
    fIsPileup = fESD->IsPileupFromSPDInMultBins();
    fIsPileupLowMult = kFALSE;
    fIsPileupLowMult = fESD->IsPileupFromSPD(3.,0.8,3.,2.,5.);
    //Physics selection
    fIsPhysSel=kFALSE;
    fIsPhysSel_v2=kFALSE;
    fIsNullPtrInputHandler=kTRUE;
    if(inputHandler!=nullptr) {
      fIsNullPtrInputHandler=kFALSE;
      fIsPhysSel = inputHandler->IsEventSelected();
      if(inputHandler->GetEventSelection()) fIsPhysSel_v2 = inputHandler->IsEventSelected();
    }
    //Event ID
    fOrbit=0;
    fBC=0;
    fOrbit=fESD->GetOrbitNumber();
    fBC=fESD->GetBunchCrossNumber();
    //Global vertex
    fVertexGlobal_Z = -9999;
    fNcontGlobal = -9999;
    fIsNullPtrVtxGlobal=kTRUE;
    const AliESDVertex *esdVertexGlobal = dynamic_cast<const AliESDVertex *>(fESD->GetPrimaryVertex());
    if(esdVertexGlobal!=nullptr)	{
        fNcontGlobal = esdVertexGlobal->GetNContributors();
        fVertexGlobal_Z=esdVertexGlobal->GetZ();
        fIsNullPtrVtxGlobal=kFALSE;
        fVtxGlobalResZ = esdVertexGlobal->GetZRes();
    }
    //Track vertex
    fVertexTrack_Z = -9999;
    fNcontTrack = -9999;
    fIsNullPtrVtxTrack=kTRUE;
    const AliESDVertex *esdVertexTrack = dynamic_cast<const AliESDVertex *>(fESD->GetPrimaryVertexTracks());
    if(esdVertexTrack!=nullptr)	{
        fNcontTrack = esdVertexTrack->GetNContributors();
        fVertexTrack_Z=esdVertexTrack->GetZ();

        if(esdVertexTrack->IsFromVertexer3D()) {
          fVertexTrack_X=esdVertexTrack->GetX();
          fVertexTrack_Y=esdVertexTrack->GetY();
        }
        fIsNullPtrVtxTrack=kFALSE;
        fVtxTrackResZ = esdVertexTrack->GetZRes();
    }
    //SPD vertex
    fVertexSPD_X = -9999;
    fVertexSPD_Y = -9999;
    fVertexSPD_Z = -9999;
    fNcontSPD = -9999;
    fIsNullPtrVtxSPD=kTRUE;
    const AliESDVertex *esdVertexSPD = dynamic_cast<const AliESDVertex *>(fESD->GetPrimaryVertexSPD());
    if(esdVertexSPD!=nullptr)	{
        fNcontSPD = esdVertexSPD->GetNContributors();
        fVertexSPD_Z=esdVertexSPD->GetZ();
        if(esdVertexSPD->IsFromVertexer3D()) {
          fVertexSPD_X=esdVertexSPD->GetX();
          fVertexSPD_Y=esdVertexSPD->GetY();
        }
        fIsNullPtrVtxSPD=kFALSE;
        fVtxSPDresZ = esdVertexSPD->GetZRes();
    }
    //CTP triggers
    fTrigger.SetString("");
		fTrigger.SetString(fESD->GetFiredTriggerClasses());
		fTriggerMask = 0;
		fTriggerMaskNext50 = 0;
    fTriggerMask = fESD->GetTriggerMask();
		fTriggerMaskNext50 = fESD->GetTriggerMaskNext50();
    TString stTrigger = fTrigger.GetString();
    //T0 trigger
		fTriggerT0 = 0;

    fIsT0fired=kFALSE;
    fIsT0fired = fESD->GetHeader()->IsTriggerInputFired("0TVX");
    //const AliESDTZERO *tzeroESD = fESD->GetESDTZERO();
    const AliESDTZERO* esdT0= (AliESDTZERO*) fESD->GetESDTZERO();
    fBitsT0timeCounter.ResetAllBits();
    if(esdT0!=nullptr)	{
      fTriggerT0 = esdT0->GetT0Trig();
      const Double32_t *timeT0=esdT0->GetT0time();
      for(int iCh=0;iCh<24;iCh++) {
        if((timeT0[iCh]>(fTimePeak[iCh]-sWindow))
           &&(timeT0[iCh]<(fTimePeak[iCh]+sWindow))) fBitsT0timeCounter.SetBitNumber(iCh);
      }
		}
		/*
		fCentralityV0M = 300;
		AliMultSelectionBase *MultSelection = 0x0;
		MultSelection = (AliMultSelectionBase * )fESD->FindListObject("MultSelection");
		if(MultSelection)	{
			fCentralityV0M = MultSelection->GetMultiplicityPercentile("V0M");
    }
    */
    if(stTrigger.Contains("CINT7-B-")||stTrigger.Contains("C0TVX-B-")||stTrigger.Contains("CADAND-B-"))	fT0OutTree->Fill();
  }
  else {
    printf("ERROR: fESD not available\n");
    return;
  }
	PostData(1, fOutputList);
}      
/*******************************************************************************************************************/
void AliMyTaskEfficiencyT0::Terminate(Option_t *)	{
  // Draw result to the screen
  // Called once at the end of the query

  /* fOutputList = dynamic_cast<TList*> (GetOutputData(1));
  if (!fOutputList) {
    printf("ERROR: Output list not available\n");
    return;
  }
    */  
}
