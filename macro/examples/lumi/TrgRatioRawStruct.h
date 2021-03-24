#ifndef TrgRatioRawStruct_h
#define TrgRatioRawStruct_h
struct TrgRatioRawStruct_nonPOD {
  TrgRatioRawStruct_nonPOD():fBitsT0timeCounter(&fBitsT0timeCounterVar) {}
  TrgRatioRawStruct_nonPOD(const TrgRatioRawStruct_nonPOD& other):fBitsT0timeCounter(&fBitsT0timeCounterVar)
  ,fBitsT0timeCounterVar(other.fBitsT0timeCounterVar)
  {}
  TrgRatioRawStruct_nonPOD(const TrgRatioRawStruct_nonPOD&& other):fBitsT0timeCounter(&fBitsT0timeCounterVar)
  ,fBitsT0timeCounterVar(std::move(other.fBitsT0timeCounterVar))
  {}
  TrgRatioRawStruct_nonPOD& operator=(const TrgRatioRawStruct_nonPOD& other) {
    if(this!=&other) {
      fBitsT0timeCounterVar=other.fBitsT0timeCounterVar;
    }
    return *this;
  }
  TBits fBitsT0timeCounterVar;
  TBits *fBitsT0timeCounter;
};

struct TrgRatioRawStruct:TrgRatioRawStruct_nonPOD {
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
  //CTP trigger
  TObjString fTrigger;
  ULong64_t fTriggerMask;
  ULong64_t fTriggerMaskNext50;
//  TBits fBitsT0timeCounterVar;
//  TBits *fBitsT0timeCounter=&fBitsT0timeCounterVar;
  void connectTree(TTree *treeInput) {
    //V0 timing
    treeInput->SetBranchAddress("fTimeV0A", &fTimeV0A);
    treeInput->SetBranchAddress("fTimeV0C", &fTimeV0C);
    treeInput->SetBranchAddress("fIsNullPtrV0",&fIsNullPtrV0);
    //Pileup
    treeInput->SetBranchAddress("fIsPileup", &fIsPileup);
    treeInput->SetBranchAddress("fIsPileupLowMult", &fIsPileupLowMult);
    //Physics selection
    treeInput->SetBranchAddress("fIsPhysSel",&fIsPhysSel);
    treeInput->SetBranchAddress("fIsPhysSel_v2",&fIsPhysSel_v2);
    treeInput->SetBranchAddress("fIsNullPtrInputHandler",&fIsNullPtrInputHandler);
    //Vertex SPD
    treeInput->SetBranchAddress("fNcontSPD",&fNcontSPD);
    treeInput->SetBranchAddress("fVertexSPD_Z",&fVertexSPD_Z);
    treeInput->SetBranchAddress("fVertexSPD_Y",&fVertexSPD_Y);
    treeInput->SetBranchAddress("fVertexSPD_X",&fVertexSPD_X);
    treeInput->SetBranchAddress("fIsNullPtrVtxSPD",&fIsNullPtrVtxSPD);
    treeInput->SetBranchAddress("fVtxSPDresZ",&fVtxSPDresZ);
    //Vertex Track
    treeInput->SetBranchAddress("fNcontTrack",&fNcontTrack);
    treeInput->SetBranchAddress("fVertexTrack_Z",&fVertexTrack_Z);
    treeInput->SetBranchAddress("fIsNullPtrVtxTrack",&fIsNullPtrVtxTrack);
    treeInput->SetBranchAddress("fVtxTrackResZ",&fVtxTrackResZ);
    //Vertex Global
    treeInput->SetBranchAddress("fNcontGlobal",&fNcontGlobal);
    treeInput->SetBranchAddress("fVertexGlobal_Z",&fVertexGlobal_Z);
    treeInput->SetBranchAddress("fIsNullPtrVtxGlobal",&fIsNullPtrVtxGlobal);
    treeInput->SetBranchAddress("fVtxGlobalResZ",&fVtxGlobalResZ);
    //Event ID
    treeInput->SetBranchAddress("fOrbit", &fOrbit);
    treeInput->SetBranchAddress("fBC", &fBC);
    //Centrality (not used)
  //	treeInput->SetBranchAddress("fCentralityV0M", &fCentralityV0M);
    //T0 trigger
    treeInput->SetBranchAddress("fTriggerT0", &fTriggerT0);
    treeInput->SetBranchAddress("fIsT0fired", &fIsT0fired);
    treeInput->SetBranchAddress("fBitsT0timeCounter",&fBitsT0timeCounter);
    //CTP trigger
    treeInput->SetBranchAddress("fTriggerMask", &fTriggerMask);
    treeInput->SetBranchAddress("fTriggerMaskNext50", &fTriggerMaskNext50);
  }
  void makeTree(TTree *treeInput) {
    //V0 timing
    treeInput->Branch("fTimeV0A", &fTimeV0A);
    treeInput->Branch("fTimeV0C", &fTimeV0C);
    treeInput->Branch("fIsNullPtrV0",&fIsNullPtrV0);
    //Pileup
    treeInput->Branch("fIsPileup", &fIsPileup);
    treeInput->Branch("fIsPileupLowMult", &fIsPileupLowMult);
    //Physics selection
    treeInput->Branch("fIsPhysSel",&fIsPhysSel);
    treeInput->Branch("fIsPhysSel_v2",&fIsPhysSel_v2);
    treeInput->Branch("fIsNullPtrInputHandler",&fIsNullPtrInputHandler);
    //Vertex SPD
    treeInput->Branch("fNcontSPD",&fNcontSPD);
    treeInput->Branch("fVertexSPD_X",&fVertexSPD_X);
    treeInput->Branch("fVertexSPD_Y",&fVertexSPD_Y);
    treeInput->Branch("fVertexSPD_Z",&fVertexSPD_Z);
    treeInput->Branch("fIsNullPtrVtxSPD",&fIsNullPtrVtxSPD);
    treeInput->Branch("fVtxSPDresZ",&fVtxSPDresZ);
    //Vertex Track
    treeInput->Branch("fNcontTrack",&fNcontTrack);
    treeInput->Branch("fVertexTrack_Z",&fVertexTrack_Z);
    treeInput->Branch("fIsNullPtrVtxTrack",&fIsNullPtrVtxTrack);
    treeInput->Branch("fVtxTrackResZ",&fVtxTrackResZ);
    //Vertex Global
    treeInput->Branch("fNcontGlobal",&fNcontGlobal);
    treeInput->Branch("fVertexGlobal_Z",&fVertexGlobal_Z);
    treeInput->Branch("fIsNullPtrVtxGlobal",&fIsNullPtrVtxGlobal);
    treeInput->Branch("fVtxGlobalResZ",&fVtxGlobalResZ);
    //Event ID
    treeInput->Branch("fOrbit", &fOrbit);
    treeInput->Branch("fBC", &fBC);
    //Centrality (not used)
  //	treeInput->Branch("fCentralityV0M", &fCentralityV0M);
    //T0 trigger
    treeInput->Branch("fTriggerT0", &fTriggerT0);
    treeInput->Branch("fIsT0fired", &fIsT0fired);
    treeInput->Branch("fBitsT0timeCounter",&fBitsT0timeCounter);
    //CTP trigger
    treeInput->Branch("fTriggerMask", &fTriggerMask);
    treeInput->Branch("fTriggerMaskNext50", &fTriggerMaskNext50);
  }

};
#endif
