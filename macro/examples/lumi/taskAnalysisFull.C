#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libboost_filesystem.so)
R__LOAD_LIBRARY(libAnalysisRUDA.so)
R__LOAD_LIBRARY(libCommonRUDA.so)
R__LOAD_LIBRARY(libTriggerRUDA.so)
#endif
#include "TObjectTable.h"
#include "AnalysisManagerBase.h"
#include "HistUtils.h"
#include "AnalysisUtils.h"
#include "TriggerClass.h"
#include "TrgRatioRawStruct.h"
struct TrgRatioStruct:TrgRatioRawStruct{
  TriggerClassManager mTrgClassManager;
  bool checkTrgClass(std::string trgClassName) const{
    //return mTrgClassManager.checkTriggerMask(trgClassName,mTriggerMask,0);
    return mTrgClassManager.checkTriggerMask(trgClassName,fTriggerMask,fTriggerMaskNext50);

  }
};
using EventCutID_t = EventCutID<30>;
using DataOutput_t = TH1F;
using Entry_t = TrgRatioStruct;

using Analysis_t = AnalysisManagerBase<Entry_t,EventCutID_t,DataOutput_t>;
using Data_t = Analysis_t::Data_t;
using DataOutputObject_t = Analysis_t::DataOutputObject_t;
template class EventCutID<30>;
template class AnalysisManagerBase<Entry_t,EventCutID_t,DataOutput_t>;
void taskAnalysisFull(std::vector<std::string>  vecPathInputData
                      ,std::string  pathOutput
                      ,std::string pathTrgClassesTree
                      ,unsigned int runNum
                      )
{



  gObjectTable->Print();
  //////////////////////////////////////////
  /// ANALYSIS TYPES ///////////////////////
  //////////////////////////////////////////
  ///

  //////////////////////////////////////////
  /// ANALYSIS I/O /////////////////////////
  //////////////////////////////////////////
  //Analysis input/output data paths
  Analysis_t analysis;
  analysis.setInputData(vecPathInputData);
  analysis.mFilepathResult = pathOutput.c_str();
  //////////////////////////////////////////
  /// PREPARING TRIGGER CLASSES ////////////
  //////////////////////////////////////////
  //Preparing triggers
  //std::string pathTrgClassesTree = "/home/deliner/work/logbook_alice/trees/TrgClasses/TrgClasses"+stYear+".root";
  TFile fileTrgClassesTree(pathTrgClassesTree.c_str(),"READ");
  TList *listTrgClassesTree = utilities::AnalysisUtils::getListObjFromFile<TTree>(fileTrgClassesTree);
  listTrgClassesTree->SetOwner(kTRUE);
  //AnalysisUtils::getTreesRecursively(&listTrgClassesTree,dynamic_cast<TList *>(fileTrgClassesTree.GetListOfKeys()));
  TTree *treeTrgClasses = (TTree *)listTrgClassesTree->First();
  analysis.mData.mTrgClassManager.init(runNum,treeTrgClasses);
  fileTrgClassesTree.Close();
  analysis.mData.mTrgClassManager.print();
  delete listTrgClassesTree;
  if(!analysis.mData.mTrgClassManager.isReady()) {
    std::cout<<"\nWARNING! TRIGGER CLASSES ARE NOT READY! ABORTING!\n";
    return;
  }
  
  //////////////////////////////////////////
  /// CUT BITS /////////////////////////////
  //////////////////////////////////////////
  //Cut bits
  auto cutNoCuts = [] (const Data_t &data)->bool {return true;};
  auto cutNoPileup = [] (const Data_t &data)->bool {return !data.fIsPileup;};
  auto cutNoPileupLowMult = [] (const Data_t &data)->bool {return !data.fIsPileupLowMult;};
  auto cutPhysSel = [] (const Data_t &data)->bool {return data.fIsPhysSel;};
  auto cutVertexTrack = [] (const Data_t &data)->bool { return (data.fNcontTrack>0)&&(data.fVertexTrack_Z>-30)&&(data.fVertexTrack_Z<30);return false;};
  auto cutVertexGlobal = [] (const Data_t &data)->bool {return (data.fNcontGlobal>0)&&(data.fVertexGlobal_Z>-10)&&(data.fVertexGlobal_Z<10);};
  auto cutVertexSPD = [] (const Data_t &data)->bool {return (data.fNcontSPD>0)&&(data.fVertexSPD_Z>-10)&&(data.fVertexSPD_Z<10);};
  auto cutV0_v1 = [](const Data_t &data)->bool {return (((data.fTimeV0A+data.fTimeV0C)>11.5) && ((data.fTimeV0A+data.fTimeV0C)<17.5) && ((data.fTimeV0A-data.fTimeV0C)>5.5) &&  ((data.fTimeV0A-data.fTimeV0C)<11.5));};
  auto cutV0_v2 = [](const Data_t &data)->bool {return (((data.fTimeV0A+data.fTimeV0C)>10) && ((data.fTimeV0A+data.fTimeV0C)<18) && ((data.fTimeV0A-data.fTimeV0C)>4) &&  ((data.fTimeV0A-data.fTimeV0C)<12));};
  auto cutC0TVX_CENTNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("C0TVX-B-NOPF-CENTNOTRD");};
  auto cutC0TVX_CENT = [] (const Data_t &data)->bool {return data.checkTrgClass("C0TVX-B-NOPF-CENT");};
  auto cutC0TVX_FAST = [] (const Data_t &data)->bool {return data.checkTrgClass("C0TVX-B-NOPF-FAST");};
  auto cutC0TVX_MUFAST = [] (const Data_t &data)->bool {return data.checkTrgClass("C0TVX-B-NOPF-MUFAST");};
  auto cutC0TVX_ALLNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("C0TVX-B-NOPF-ALLNOTRD");};
  //auto cutC0TVX_ALLNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("C0TVX-B-NOPF-ALLNOTRD")&&!data.checkTrgClass("CINT7-B-NOPF-ALLNOTRD");};
  auto cutCINT7_CENTNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("CINT7-B-NOPF-CENTNOTRD");};
  auto cutCINT7_CENT = [] (const Data_t &data)->bool {return data.checkTrgClass("CINT7-B-NOPF-CENT");};
  auto cutCINT7_FAST = [] (const Data_t &data)->bool {return data.checkTrgClass("CINT7-B-NOPF-FAST");};
  auto cutCINT7_MUFAST = [] (const Data_t &data)->bool {return data.checkTrgClass("CINT7-B-NOPF-MUFAST");};
  auto cutCINT7_ALLNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("CINT7-B-NOPF-ALLNOTRD");};

  analysis.mCutObjectManager.makeCutBit("noCuts","noCuts",cutNoCuts);
  analysis.mCutObjectManager.makeCutBit("noPileup","Excluded pileup from events",cutNoPileup);
  analysis.mCutObjectManager.makeCutBit("noPileupLowMult","Excluded pileup from events,IsPileupFromSPD()",cutNoPileupLowMult);
  analysis.mCutObjectManager.makeCutBit("PhysSel","Phys sel",cutPhysSel);

  analysis.mCutObjectManager.makeCutBit("vertexGlobal","-10cm<VertexGlobal<+10cm",cutVertexGlobal);
  analysis.mCutObjectManager.makeCutBit("vertexTrack","nCont>0 && |vertexTrack|<30",cutVertexTrack);
  analysis.mCutObjectManager.makeCutBit("vertexSPD","-10cm<VertexGlobal<+10cm",cutVertexSPD);

  analysis.mCutObjectManager.makeCutBit("V0_v1","(timeV0A+timeV0C)>11.5 && (timeV0A+timeV0C)<17.5 && (timeV0A-timeV0C)>5.5 && (timeV0A-timeV0C)<11.5",cutV0_v1);
  analysis.mCutObjectManager.makeCutBit("V0_v2","(timeV0A+timeV0C)>10 && (timeV0A+timeV0C)<18 && (timeV0A-timeV0C)>4 && (timeV0A-timeV0C)<12",cutV0_v2);

  analysis.mCutObjectManager.makeCutBit("cutC0TVX_CENTNOTRD","cutC0TVX_CENTNOTRD",cutC0TVX_CENTNOTRD);
  analysis.mCutObjectManager.makeCutBit("cutC0TVX_CENT","cutC0TVX_CENT",cutC0TVX_CENT);
  analysis.mCutObjectManager.makeCutBit("cutC0TVX_FAST","cutC0TVX_FAST",cutC0TVX_FAST);
  analysis.mCutObjectManager.makeCutBit("cutC0TVX_MUFAST","cutC0TVX_MUFAST",cutC0TVX_MUFAST);
  analysis.mCutObjectManager.makeCutBit("cutC0TVX_ALLNOTRD","cutC0TVX_ALLNOTRD",cutC0TVX_ALLNOTRD);

  analysis.mCutObjectManager.makeCutBit("cutCINT7_CENTNOTRD","cutCINT7_CENTNOTRD",cutCINT7_CENTNOTRD);
  analysis.mCutObjectManager.makeCutBit("cutCINT7_CENT","cutCINT7_CENT",cutCINT7_CENT);
  analysis.mCutObjectManager.makeCutBit("cutCINT7_FAST","cutCINT7_FAST",cutCINT7_FAST);
  analysis.mCutObjectManager.makeCutBit("cutCINT7_MUFAST","cutCINT7_MUFAST",cutCINT7_MUFAST);
  analysis.mCutObjectManager.makeCutBit("cutCINT7_ALLNOTRD","cutCINT7_ALLNOTRD",cutCINT7_ALLNOTRD);

  //////////////////////////////////////////
  /// ANALYSIS OUTPUT //////////////////////
  //////////////////////////////////////////
  ///
  /// Event class counters
  ///
  //////////////////////////////////////////
  //Vector of Event IDs
  std::vector<EventCutID_t> vecEventCutID = {
    analysis.mCutObjectManager.makeNamedEventCutID("noCuts")
    ,analysis.mCutObjectManager.makeNamedEventCutID("noPileup")
    ,analysis.mCutObjectManager.makeNamedEventCutID("noPileupLowMult")
    ,analysis.mCutObjectManager.makeNamedEventCutID("PhysSel")
    ,analysis.mCutObjectManager.makeNamedEventCutID("vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("vertexTrack")
    ,analysis.mCutObjectManager.makeNamedEventCutID("vertexSPD")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileup")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileupLowMult")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileup")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileupLowMult")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileup PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileup PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileupLowMult PhysSel vertexTrack")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileupLowMult PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_CENTNOTRD")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_CENT")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_FAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_MUFAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_ALLNOTRD")

    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_CENTNOTRD")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_CENT")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_FAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_MUFAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_ALLNOTRD")

  };
  //Trigger cluster names
  std::vector<std::string> vecTrgClusterNames = {
     "CENTNOTRD"
      ,"CENT"
      ,"FAST"
      ,"MUFAST"
    ,"ALLNOTRD"
  };
  //Prepare output for C0TVX trigger counter
  std::vector<std::string> vecCntTrgHistNamesC0TVX;
  for(const auto &trgClstName:vecTrgClusterNames) {
    std::string histNameDescriptor = "hCutC0TVX_";
    std::string histName = histNameDescriptor+trgClstName;
    std::string cutName = "cutC0TVX_" + trgClstName + " " + "cutCINT7_" + trgClstName;
    //std::string cutName = "cutC0TVX_" + trgClstName;
    vecCntTrgHistNamesC0TVX.push_back(histName);
    analysis.mDataOutputManager.makeDataOutput(histName,histName,vecEventCutID
                                               ,analysis.mCutObjectManager.makeNamedEventCutID(cutName));
  }
  //Prepare output for CINT7 trigger counter
  std::vector<std::string> vecCntTrgHistNamesCINT7;
  for(const auto &trgClstName:vecTrgClusterNames) {
    std::string histNameDescriptor = "hCutCINT7_";
    std::string histName = histNameDescriptor+trgClstName;
    std::string cutName = "cutCINT7_" + trgClstName;
    vecCntTrgHistNamesCINT7.push_back(histName);
    analysis.mDataOutputManager.makeDataOutput(histName,histName,vecEventCutID
                                               ,analysis.mCutObjectManager.makeNamedEventCutID(cutName));
  }
  analysis.mDataOutputManager.makeDataOutput("hCutVertexSPD","hCutVertexSPD",vecEventCutID
                                             ,analysis.mCutObjectManager.makeNamedEventCutID("vertexSPD"));
  analysis.mDataOutputManager.makeDataOutput("hNoCuts","hNoCuts",vecEventCutID
                                             ,analysis.mCutObjectManager.makeNamedEventCutID("noCuts"));
  //////////////////////////////////////////
  /// ANALYSIS OUTPUT //////////////////////
  //////////////////////////////////////////
  ///
  /// Differential ratios
  ///
  //////////////////////////////////////////
  //Preparing differential ratio
  struct OutputDataEntry {
    typedef std::function<void(const Data_t &,DataOutputObject_t *)> FuncFill_t;
    typedef helpers::hists::HistHelper<DataOutput_t>::HistParam HistParam;
    OutputDataEntry(std::string histNameTitle,Int_t nBinsX,Double_t lowBinX,Double_t upBinX
                    ,FuncFill_t funcFill,std::string cutName):
      mFuncFill(funcFill),mCutName(cutName)
    {
       mHistParam.mHistName=histNameTitle;
       mHistParam.mHistTitle=histNameTitle;
       mHistParam.mNBinsX=nBinsX;
       mHistParam.mLowBinX=lowBinX;
       mHistParam.mUpBinX=upBinX;
    }
    OutputDataEntry(HistParam histParam,FuncFill_t funcFill,std::string cutName):
      mHistParam(histParam),mFuncFill(funcFill),mCutName(cutName) {}
    HistParam mHistParam;
    FuncFill_t mFuncFill;
    std::string mCutName;
  };


  std::vector<OutputDataEntry> vecOutputRatioC0TVX;
  std::vector<OutputDataEntry> vecOutputRatioCINT7;
  //Strings of EventCutIDs
  std::string stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1 = "cutC0TVX_CENTNOTRD cutCINT7_CENTNOTRD V0_v1 noPileup PhysSel";
  std::string stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1 = "cutCINT7_CENTNOTRD V0_v1 noPileup PhysSel";

  std::string stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1 = "cutC0TVX_ALLNOTRD cutCINT7_ALLNOTRD V0_v1 noPileup PhysSel";
  std::string stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1 = "cutCINT7_ALLNOTRD V0_v1 noPileup PhysSel";

  std::string stCut_C0TVX_CENTNOTRD_FullCuts_v1 = "cutC0TVX_CENTNOTRD cutCINT7_CENTNOTRD V0_v1 noPileup PhysSel vertexGlobal";
  std::string stCut_CINT7_CENTNOTRD_FullCuts_v1 = "cutCINT7_CENTNOTRD V0_v1 noPileup PhysSel vertexGlobal";

  std::string stCut_C0TVX_ALLNOTRD_FullCuts_v1 = "cutC0TVX_ALLNOTRD cutCINT7_ALLNOTRD V0_v1 noPileup PhysSel vertexGlobal";
  std::string stCut_CINT7_ALLNOTRD_FullCuts_v1 = "cutCINT7_ALLNOTRD V0_v1 noPileup PhysSel vertexGlobal";

//  std::string stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v2 = "cutC0TVX_CENTNOTRD cutCINT7_CENTNOTRD V0_v2 noPileupLowMult PhysSel";
//  std::string stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v2 = "cutCINT7_CENTNOTRD V0_v2 noPileupLowMult PhysSel";

//  std::string stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v2 = "cutC0TVX_ALLNOTRD cutCINT7_ALLNOTRD V0_v2 noPileupLowMult PhysSel";
//  std::string stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v2 = "cutCINT7_ALLNOTRD V0_v2 noPileupLowMult PhysSel";

  //Ratio as func of Orbit
  auto funcFillOrbit = [] (const Data_t &data,DataOutputObject_t *outputPtr) {outputPtr->mDataOutput.Fill(data.fOrbit);};
  vecOutputRatioC0TVX.emplace_back(
        "hOrbit_C0TVX_CENTNOTRD_FullCuts",18,0,18000000
        ,funcFillOrbit
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");

  vecOutputRatioC0TVX.emplace_back(
        "hOrbit_C0TVX_ALLNOTRD_FullCuts",18,0,18000000
        ,funcFillOrbit
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hOrbit_CINT7_CENTNOTRD_FullCuts",18,0,18000000
        ,funcFillOrbit
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");

  vecOutputRatioCINT7.emplace_back(
        "hOrbit_CINT7_ALLNOTRD_FullCuts",18,0,18000000
        ,funcFillOrbit
        ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");

  //Ratio as func of BC

  //C0TVX
  auto funcFillBC = [] (const Data_t &data,DataOutputObject_t *outputPtr) {outputPtr->mDataOutput.Fill(data.fBC);};
  vecOutputRatioC0TVX.emplace_back(
        "hBC_C0TVX_CENTNOTRD_FullCuts",3600,0,3600
        ,funcFillBC
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");

  vecOutputRatioC0TVX.emplace_back(
        "hBC_C0TVX_ALLNOTRD_FullCuts",3600,0,3600
        ,funcFillBC
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hBC_CINT7_CENTNOTRD_FullCuts",3600,0,3600
        ,funcFillBC
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");

  vecOutputRatioCINT7.emplace_back(
        "hBC_CINT7_ALLNOTRD_FullCuts",3600,0,3600
        ,funcFillBC
        ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1 + " vertexGlobal");
  //Preparing as output data

  //Ratio as func of global vertex
  auto funcFillVertexGlobal = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    if(data.fVertexGlobal_Z!=0)outputPtr->mDataOutput.Fill(data.fVertexGlobal_Z);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hVertex_C0TVX_CENTNOTRD_FullCuts",100,-50,50
        ,funcFillVertexGlobal
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hVertex_C0TVX_ALLNOTRD_FullCuts",100,-50,50
        ,funcFillVertexGlobal
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hVertex_CINT7_CENTNOTRD_FullCuts",100,-50,50
        ,funcFillVertexGlobal
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioCINT7.emplace_back(
       "hVertex_CINT7_ALLNOTRD_FullCuts",100,-50,50
       ,funcFillVertexGlobal
       ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1);

  //Ratio as func of track vertex
  auto funcFillVertexTrack = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    if(data.fNcontTrack>0)outputPtr->mDataOutput.Fill(data.fVertexTrack_Z);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hVertexTrk_C0TVX_CENTNOTRD_FullCuts",100,-50,50
        ,funcFillVertexTrack
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hVertexTrk_C0TVX_ALLNOTRD_FullCuts",100,-50,50
        ,funcFillVertexTrack
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hVertexTrk_CINT7_CENTNOTRD_FullCuts",100,-50,50
        ,funcFillVertexTrack
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioCINT7.emplace_back(
       "hVertexTrk_CINT7_ALLNOTRD_FullCuts",100,-50,50
       ,funcFillVertexTrack
       ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1);

  //Ratio as func of SPD vertex
  auto funcFillVertexSPD = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    if(data.fNcontSPD>0)outputPtr->mDataOutput.Fill(data.fVertexSPD_Z);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hVertexSPD_C0TVX_CENTNOTRD_FullCuts",100,-50,50
        ,funcFillVertexSPD
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hVertexSPD_C0TVX_ALLNOTRD_FullCuts",100,-50,50
        ,funcFillVertexSPD
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hVertexSPD_CINT7_CENTNOTRD_FullCuts",100,-50,50
        ,funcFillVertexSPD
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioCINT7.emplace_back(
        "hVertexSPD_CINT7_ALLNOTRD_FullCuts",100,-50,50
        ,funcFillVertexSPD
        ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1);

  //Ratio as func of SPD vertex contributors
  auto funcFillNContSPD = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    outputPtr->mDataOutput.Fill(data.fNcontSPD);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hNContSPD_C0TVX_CENTNOTRD_FullCuts",200,-100,100
        ,funcFillNContSPD
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hNContSPD_C0TVX_ALLNOTRD_FullCuts",200,-100,100
        ,funcFillNContSPD
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hNContSPD_CINT7_CENTNOTRD_FullCuts",200,-100,100
        ,funcFillNContSPD
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioCINT7.emplace_back(
        "hNContSPD_CINT7_ALLNOTRD_FullCuts",200,-100,100
        ,funcFillNContSPD
        ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1);

  //Ratio as func of T0 counter time bits
  /*
  auto funcFillTimeBitsT0 = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    for(int iCh=0;iCh<24;iCh++) {
      if(data.fBitsT0timeCounter->TestBitNumber(iCh)) outputPtr->mDataOutput.Fill(iCh);
    }
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_CENTNOTRD_FullCuts",24,0,24
        ,funcFillTimeBitsT0
        ,stCut_C0TVX_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_ALLNOTRD_FullCuts",24,0,24
        ,funcFillTimeBitsT0
        ,stCut_C0TVX_ALLNOTRD_FullCuts_noVertex_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_CENTNOTRD_FullCuts",24,0,24
        ,funcFillTimeBitsT0
        ,stCut_CINT7_CENTNOTRD_FullCuts_noVertex_v1);
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_ALLNOTRD_FullCuts",24,0,24
        ,funcFillTimeBitsT0
        ,stCut_CINT7_ALLNOTRD_FullCuts_noVertex_v1);
  */
   //Ratio as func of T0 counter time bits, side A
  auto funcFillTimeBitsT0_sideA = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    for(int iCh=12;iCh<24;iCh++) {
      if(data.fBitsT0timeCounter->TestBitNumber(iCh)) outputPtr->mDataOutput.Fill(iCh-12);
    }
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_CENTNOTRD_FullCuts_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA
        ,stCut_C0TVX_CENTNOTRD_FullCuts_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_ALLNOTRD_FullCuts_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA
        ,stCut_C0TVX_ALLNOTRD_FullCuts_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_CENTNOTRD_FullCuts_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA
        ,stCut_CINT7_CENTNOTRD_FullCuts_v1);
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_ALLNOTRD_FullCuts_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA
        ,stCut_CINT7_ALLNOTRD_FullCuts_v1);
  //Ratio as func of T0 counter time bits, side C
  auto funcFillTimeBitsT0_sideC = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    for(int iCh=0;iCh<12;iCh++) {
      if(data.fBitsT0timeCounter->TestBitNumber(iCh)) outputPtr->mDataOutput.Fill(iCh);
    }
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_CENTNOTRD_FullCuts_sideC",12,0,12
        ,funcFillTimeBitsT0_sideC
        ,stCut_C0TVX_CENTNOTRD_FullCuts_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_ALLNOTRD_FullCuts_sideC",12,0,12
        ,funcFillTimeBitsT0_sideC
        ,stCut_C0TVX_ALLNOTRD_FullCuts_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_CENTNOTRD_FullCuts_sideC",12,0,12
        ,funcFillTimeBitsT0_sideC
        ,stCut_CINT7_CENTNOTRD_FullCuts_v1);
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_ALLNOTRD_FullCuts_sideC",12,0,12
        ,funcFillTimeBitsT0_sideC
        ,stCut_CINT7_ALLNOTRD_FullCuts_v1);
///////////////////////////////////////
  //Ratio as func of T0 counter time bits, side A
  auto funcFillTimeBitsT0_sideA_sh6 = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    for(int iCh=12;iCh<24;iCh++) {
      if(data.fBitsT0timeCounter->TestBitNumber(iCh)) outputPtr->mDataOutput.Fill((iCh-12+6)%12);
    }
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_CENTNOTRD_FullCuts_sh6_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA_sh6
        ,stCut_C0TVX_CENTNOTRD_FullCuts_v1);
  vecOutputRatioC0TVX.emplace_back(
        "hTimeBitsT0_C0TVX_ALLNOTRD_FullCuts_sh6_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA_sh6
        ,stCut_C0TVX_ALLNOTRD_FullCuts_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_CENTNOTRD_FullCuts_sh6_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA
        ,stCut_CINT7_CENTNOTRD_FullCuts_v1);
  vecOutputRatioCINT7.emplace_back(
        "hTimeBitsT0_CINT7_ALLNOTRD_FullCuts_sh6_sideA",12,0,12
        ,funcFillTimeBitsT0_sideA
        ,stCut_CINT7_ALLNOTRD_FullCuts_v1);
  //Ratio as func of T0 counter time bits, side C
  auto funcFillTimeBitsT0_sideC_sh6 = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    for(int iCh=0;iCh<12;iCh++) {
      if(data.fBitsT0timeCounter->TestBitNumber(iCh)) outputPtr->mDataOutput.Fill((iCh+6)%12);
    }
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
       "hTimeBitsT0_C0TVX_CENTNOTRD_FullCuts_sh6_sideC",12,0,12
       ,funcFillTimeBitsT0_sideC_sh6
       ,stCut_C0TVX_CENTNOTRD_FullCuts_v1);
  vecOutputRatioC0TVX.emplace_back(
       "hTimeBitsT0_C0TVX_ALLNOTRD_FullCuts_sh6_sideC",12,0,12
       ,funcFillTimeBitsT0_sideC_sh6
       ,stCut_C0TVX_ALLNOTRD_FullCuts_v1);
  //CINT7
  vecOutputRatioCINT7.emplace_back(
       "hTimeBitsT0_CINT7_CENTNOTRD_FullCuts_sh6_sideC",12,0,12
       ,funcFillTimeBitsT0_sideC_sh6
       ,stCut_CINT7_CENTNOTRD_FullCuts_v1);
  vecOutputRatioCINT7.emplace_back(
       "hTimeBitsT0_CINT7_ALLNOTRD_FullCuts_sh6_sideC",12,0,12
       ,funcFillTimeBitsT0_sideC_sh6
       ,stCut_CINT7_ALLNOTRD_FullCuts_v1);
///////////////////////////////////////
  //Ratio as func of diffV0
  auto funcFillDiffAmpV0 = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    outputPtr->mDataOutput.Fill(data.fTimeV0A-data.fTimeV0C);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hDiffAmpV0_C0TVX_CENTNOTRD_FullCuts",600,-30,30
        ,funcFillDiffAmpV0
        ,"cutC0TVX_CENTNOTRD cutCINT7_CENTNOTRD noPileup PhysSel");
  vecOutputRatioC0TVX.emplace_back(
        "hDiffAmpV0_C0TVX_ALLNOTRD_FullCuts",600,-30,30
        ,funcFillDiffAmpV0
        ,"cutC0TVX_ALLNOTRD cutCINT7_ALLNOTRD noPileup PhysSel");
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hDiffAmpV0_CINT7_CENTNOTRD_FullCuts",600,-30,30
        ,funcFillDiffAmpV0
        ,"cutCINT7_CENTNOTRD noPileup PhysSel");
  vecOutputRatioCINT7.emplace_back(
        "hDiffAmpV0_CINT7_ALLNOTRD_FullCuts",600,-30,30
        ,funcFillDiffAmpV0
        ,"cutCINT7_ALLNOTRD noPileup PhysSel");


  //Ratio as func of sumV0
  auto funcFillSumAmpV0 = [](const Data_t &data,DataOutputObject_t *outputPtr) {
    outputPtr->mDataOutput.Fill(data.fTimeV0A+data.fTimeV0C);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hSumAmpV0_C0TVX_CENTNOTRD_FullCuts",600,-30,30
        ,funcFillSumAmpV0
        ,"cutC0TVX_CENTNOTRD cutCINT7_CENTNOTRD noPileup PhysSel");
  vecOutputRatioC0TVX.emplace_back(
        "hSumAmpV0_C0TVX_ALLNOTRD_FullCuts",600,-30,30
        ,funcFillSumAmpV0
        ,"cutC0TVX_ALLNOTRD cutCINT7_ALLNOTRD noPileup PhysSel");
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hSumAmpV0_CINT7_CENTNOTRD_FullCuts",600,-30,30
        ,funcFillSumAmpV0
        ,"cutCINT7_CENTNOTRD noPileup PhysSel");
  vecOutputRatioCINT7.emplace_back(
        "hSumAmpV0_CINT7_ALLNOTRD_FullCuts",600,-30,30
        ,funcFillSumAmpV0
        ,"cutCINT7_ALLNOTRD noPileup PhysSel");

  //Prepare ratio names for calculations
  std::vector<OutputDataEntry> vecOutputRatioAll;
  std::copy(vecOutputRatioC0TVX.begin(),vecOutputRatioC0TVX.end(),std::back_inserter(vecOutputRatioAll));
  std::copy(vecOutputRatioCINT7.begin(),vecOutputRatioCINT7.end(),std::back_inserter(vecOutputRatioAll));
  for(const auto &entry:vecOutputRatioAll) {
    //auto histName = entry.mHistName;
    analysis.mDataOutputManager.makeDataOutput(
          DataOutput_t{entry.mHistParam.mHistName.c_str(),entry.mHistParam.mHistTitle.c_str()
                       ,entry.mHistParam.mNBinsX,entry.mHistParam.mLowBinX,entry.mHistParam.mUpBinX}
          ,entry.mFuncFill
          ,analysis.mCutObjectManager.makeNamedEventCutID(entry.mCutName));
    /*
    if(histName.find("hBC_")!=std::string::npos) {
      analysis.mDataOutputManager.makeDataOutput(
            DataOutput_t{entry.mHistName.c_str(),entry.mHistTitle.c_str(),3600,0,3600}
            ,entry.mFuncFill
            ,analysis.mCutObjectManager.makeNamedEventCutID(entry.mCutName));
    }
    */
  }
  //////////////////////////////////////////
  /// RUNNING ANALYSIS /////////////////////
  //////////////////////////////////////////
  analysis.run();
  analysis.saveResult();
  //////////////////////////////////////////
  /// SAVING RESULTS ///////////////////////
  //////////////////////////////////////////
  //analysis.saveResult();

}
