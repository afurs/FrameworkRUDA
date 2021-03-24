#ifndef AliTaskAgeingT0_cxx
#define AliTaskAgeingT0_cxx

#include <iostream>

#include "TList.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TChain.h"
#include "EventStruct.h"

#include "AliAnalysisManager.h"
#include "AliInputEventHandler.h"
#include "AliAnalysisTaskSE.h"
#include "AliESDEvent.h"
#include "AliESDTZERO.h"
#include "AliESDVZERO.h"
#include "AliMultiplicity.h"

class AliTaskAgeingT0 : public AliAnalysisTaskSE {
  public:
    AliTaskAgeingT0():AliAnalysisTaskSE()
    ,fESD(0x0), fOutputList(0x0),mEventStruct{}
    ,mVecHists_v0{}
    ,mVecHistsC0TVX_v0{},mVecHistsCINT7_v0{}
    ,mVecHistsC0TVX_v1{},mVecHistsCINT7_v1{}
    ,mVecHistsC0TVX_v2{},mVecHistsCINT7_v2{}
    ,mVecHistsC0TVX_v3{},mVecHistsCINT7_v3{}
    ,mVecHistsC0TVX_v4{},mVecHistsCINT7_v4{}
    {}
    AliTaskAgeingT0(const char *name);
    virtual ~AliTaskAgeingT0();
    typedef TH2F Hist_t;
    virtual void   UserCreateOutputObjects();
		virtual void   UserExec(Option_t *option);
		//void   NotifyRun();
		virtual void   Terminate(Option_t *);
		Bool_t UserNotify();

  private:
    struct HistParameters {
      Int_t nBinsX;
      Double_t lowBinX;
      Double_t upBinX;
      Int_t nBinsY;
      Double_t lowBinY;
      Double_t upBinY;
    };

    void PrepareOutput(TString nameList,const std::vector<Hist_t *> &vecHists) ;
    void MakeHistOutput(std::vector<Hist_t *> &vecHists
                        ,std::string nameHist,std::string titleHist
                        ,const std::vector<HistParameters> &vecParams);
    void FillT0data(const std::vector<Hist_t *> &vecHists);
		AliESDEvent *fESD;    //! ESD object
    //Data container
		TList *fOutputList; //! Output list
    //Event struct
    EventStruct mEventStruct;
    std::vector<Hist_t *> mVecHists_v0;
    
    //No cuts
    std::vector<Hist_t *> mVecHistsC0TVX_v0;
    std::vector<Hist_t *> mVecHistsCINT7_v0;
    //Track vertex cut(30cm, cont>1),pileupLowMultSPD,physSel,V0time_v2
    std::vector<Hist_t *> mVecHistsC0TVX_v1;
    std::vector<Hist_t *> mVecHistsCINT7_v1;
    //Track vertex cut(30cm, cont>1),pileupSPD,physSel,V0time_v2
    std::vector<Hist_t *> mVecHistsC0TVX_v2;
    std::vector<Hist_t *> mVecHistsCINT7_v2;
    //Global vertex cut(10cm, cont>0),pileupSPD,physSel,V0time_v2
    std::vector<Hist_t *> mVecHistsC0TVX_v3;
    std::vector<Hist_t *> mVecHistsCINT7_v3;
    //Global vertex cut(10cm, cont>0),pileupSPD,physSel,V0time_v1
    std::vector<Hist_t *> mVecHistsC0TVX_v4;
    std::vector<Hist_t *> mVecHistsCINT7_v4;
    AliTaskAgeingT0(const AliTaskAgeingT0&); // not implemented
    AliTaskAgeingT0& operator=(const AliTaskAgeingT0&); // not implemented
    ClassDef(AliTaskAgeingT0, 1); // example of analysis
};
#endif
