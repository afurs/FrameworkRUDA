#ifndef AliMyTaskEfficiencyT0_cxx
#define AliMyTaskEfficiencyT0_cxx

#include "TTree.h"
#include "TList.h"
#include "TObjString.h"
#include "TGrid.h"
#include "TChain.h"
#include "TBits.h"
#include "AliAnalysisManager.h"
#include "AliESDEvent.h"
#include "AliESDInputHandler.h"


#include "AliAnalysisTaskSE.h"
#include "AliESDTZERO.h"
#include "AliESDVZERO.h"
#include "AliESDVertex.h"

#include "AliCDBEntry.h"
#include "AliCDBManager.h"
#include "AliT0CalibTimeEq.h"

class AliMyTaskEfficiencyT0 : public AliAnalysisTaskSE {
	public:
		AliMyTaskEfficiencyT0():AliAnalysisTaskSE()
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
    {}

		AliMyTaskEfficiencyT0(const char *name);
		virtual ~AliMyTaskEfficiencyT0();
    virtual void   UserCreateOutputObjects();
		virtual void   UserExec(Option_t *option);
    virtual void   NotifyRun();
		virtual void   Terminate(Option_t *);
		Bool_t UserNotify();
  private:
		AliESDEvent *fESD;    //! ESD object
    //Data container
		TList *fOutputList; //! Output list
		TTree *fT0OutTree;  // output tree
    //V0 timing
    Bool_t fIsNullPtrV0;
		Float_t fTimeV0A;
		Float_t fTimeV0C;
    //Pileup
		Bool_t fIsPileup;
		Bool_t fIsPileupLowMult;
    //Physics selection
    Bool_t fIsNullPtrInputHandler;
    Bool_t fIsPhysSel;
    Bool_t fIsPhysSel_v2;
    //SPD vertex variables
    Int_t fNcontSPD;
    Float_t fVertexSPD_X;
		Float_t fVertexSPD_Y;
		Float_t fVertexSPD_Z;
    Double_t fVtxSPDresZ;
    Bool_t fIsNullPtrVtxSPD;
    //Track vertex variables
    Int_t fNcontTrack;
    Float_t fVertexTrack_X;
    Float_t fVertexTrack_Y;
    Float_t fVertexTrack_Z;
    Double_t fVtxTrackResZ;
    Bool_t fIsNullPtrVtxTrack;
    //Global vertex variables
    Int_t fNcontGlobal;
    Float_t fVertexGlobal_Z;
    Double_t fVtxGlobalResZ;
    Bool_t fIsNullPtrVtxGlobal;
    //EventID
    ULong64_t fOrbit;
    unsigned int fBC;
    //Centrality(not used)
    Float_t fCentralityV0M;
    //T0 trigger
    UShort_t fTriggerT0;
    Bool_t fIsT0fired;
    TBits fBitsT0timeCounter;
    constexpr static Float_t sWindow = 70;
    Float_t fTimePeak[24];
    //CTP trigger
		TObjString fTrigger;
		ULong64_t fTriggerMask;
    ULong64_t fTriggerMaskNext50;
		AliMyTaskEfficiencyT0(const AliMyTaskEfficiencyT0&); // not implemented
		AliMyTaskEfficiencyT0& operator=(const AliMyTaskEfficiencyT0&); // not implemented
		ClassDef(AliMyTaskEfficiencyT0, 1); // example of analysis
};
#endif
