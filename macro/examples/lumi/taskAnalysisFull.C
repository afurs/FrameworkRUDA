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
using EventCutID_t = EventCutID<50>;
using DataOutput_t = TH1F;
using DataOutput2D_t = TH2F;

using Entry_t = TrgRatioStruct;

using Analysis_t = AnalysisManagerBase<Entry_t,EventCutID_t,DataOutput_t,DataOutput2D_t>;
using Data_t = Analysis_t::Data_t;
using DataOutputObject_t = Analysis_t::DataOutputObject_t;
using DataOutputObject2D_t = typename std::tuple_element<0,Analysis_t::DataOutputTuple_t>::type::DataOutput_t;

template class EventCutID<50>;
template class AnalysisManagerBase<Entry_t,EventCutID_t,DataOutput_t>;
void taskAnalysisFull(/*  vecPathInputData
                      ,std::string  pathOutput
                      ,std::string pathTrgClassesTree
                      ,unsigned int runNum*/
                      std::tuple<std::vector<std::string>,std::string,std::string,unsigned int> tupleArgs)
{
  auto vecPathInputData = std::get<0>(tupleArgs);
  auto pathOutput = std::get<1>(tupleArgs);
  auto pathTrgClassesTree = std::get<2>(tupleArgs);
  auto runNum = std::get<3>(tupleArgs);

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
  analysis.mData.mTrgClassManager.mRunNum = runNum;
  analysis.mData.mTrgClassManager.mFilepath = pathTrgClassesTree;
  analysis.mData.mTrgClassManager.init();
  //analysis.mData.mTrgClassManager.print();
  std::map<std::string,std::string> mapName2TrgInputs = {
    {"0EMC-B-NOPF-CENT",".*EMC.*-B-NOPF-CENT"}
    ,{"0EMC-B-NOPF-CENTNOTRD",".*EMC.*-B-NOPF-CENTNOTRD"}
    ,{"0EMC-B-NOPF-ALLNOTRD",".*EMC.*-B-NOPF-ALLNOTRD"}
    ,{"0EMC-B-NOPF-FAST",".*EMC.*-B-NOPF-FAST"}
    ,{"0EMC-B-NOPF-MUFAST",".*EMC.*-B-NOPF-MUFAST"}

    ,{"0MSL-B-NOPF-CENT",".*MSL.*-B-NOPF-CENT"}
    ,{"0MSL-B-NOPF-CENTNOTRD",".*MSL.*-B-NOPF-CENTNOTRD"}
    ,{"0MSL-B-NOPF-ALLNOTRD",".*MSL.*-B-NOPF-ALLNOTRD"}
    ,{"0MSL-B-NOPF-FAST",".*MSL.*-B-NOPF-FAST"}
    ,{"0MSL-B-NOPF-MUFAST",".*MSL.*-B-NOPF-MUFAST"}

    ,{"0MUL-B-NOPF-CENT",".*MUL.*-B-NOPF-CENT"}
    ,{"0MUL-B-NOPF-CENTNOTRD",".*MUL.*-B-NOPF-CENTNOTRD"}
    ,{"0MUL-B-NOPF-ALLNOTRD",".*MUL.*-B-NOPF-ALLNOTRD"}
    ,{"0MUL-B-NOPF-FAST",".*MUL.*-B-NOPF-FAST"}
    ,{"0MUL-B-NOPF-MUFAST",".*MUL.*-B-NOPF-MUFAST"}

    ,{"INT7-INPUT",".*INT7.*-B-.*"}
    ,{"0TVX-INPUT",".*0TVX.*-B-.*"}
    ,{"0EMC-INPUT",".*EMC.*-B-.*"}
    ,{"0MSL-INPUT",".*MSL.*-B-.*"}
    ,{"0MUL-INPUT",".*MUL.*-B-.*"}
    ,{"0V0H-INPUT",".*V0H.*-B-.*"}
    ,{"0V0M-INPUT",".*V0M.*-B-.*"}
    ,{"0VHM-INPUT",".*VHM.*-B-.*"}
    ,{"VHMV0M-INPUT",".*VHMV0M.*-B-.*"}
  };
  for(const auto& entry: mapName2TrgInputs) {
    auto nTrgs = analysis.mData.mTrgClassManager.defineTrgSumReg(entry.first,entry.first,entry.second);
  }
  analysis.mData.mTrgClassManager.print();
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
  auto cutV0_v1 = [](const Data_t &data)->bool {return (((data.fTimeV0A+data.fTimeV0C)>11.5) && ((data.fTimeV0A+data.fTimeV0C)<17.5)
                                                        && ((data.fTimeV0A-data.fTimeV0C)>5.5) &&  ((data.fTimeV0A-data.fTimeV0C)<11.5));};
  auto cutV0_v2 = [](const Data_t &data)->bool {return (((data.fTimeV0A+data.fTimeV0C)>10) && ((data.fTimeV0A+data.fTimeV0C)<18)
                                                        && ((data.fTimeV0A-data.fTimeV0C)>4) &&  ((data.fTimeV0A-data.fTimeV0C)<12));};
  auto cutV0Ellipse = [](const Data_t &data)->bool {
    return (pow((data.fTimeV0A-10.7)/0.9,2)+pow((data.fTimeV0C-2.8)/0.8,2))<1;
  };
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

  auto cut0EMC_CENTNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("0EMC-B-NOPF-CENTNOTRD");};
  auto cut0EMC_CENT = [] (const Data_t &data)->bool {return data.checkTrgClass("0EMC-B-NOPF-CENT");};
  auto cut0EMC_FAST = [] (const Data_t &data)->bool {return data.checkTrgClass("0EMC-B-NOPF-FAST");};
  auto cut0EMC_MUFAST = [] (const Data_t &data)->bool {return data.checkTrgClass("0EMC-B-NOPF-MUFAST");};
  auto cut0EMC_ALLNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("0EMC-B-NOPF-ALLNOTRD");};

  auto cut0MSL_CENTNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("0MSL-B-NOPF-CENTNOTRD");};
  auto cut0MSL_CENT = [] (const Data_t &data)->bool {return data.checkTrgClass("0MSL-B-NOPF-CENT");};
  auto cut0MSL_FAST = [] (const Data_t &data)->bool {return data.checkTrgClass("0MSL-B-NOPF-FAST");};
  auto cut0MSL_MUFAST = [] (const Data_t &data)->bool {return data.checkTrgClass("0MSL-B-NOPF-MUFAST");};
  auto cut0MSL_ALLNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("0MSL-B-NOPF-ALLNOTRD");};


  auto cut0MUL_CENTNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("0MUL-B-NOPF-CENTNOTRD");};
  auto cut0MUL_CENT = [] (const Data_t &data)->bool {return data.checkTrgClass("0MUL-B-NOPF-CENT");};
  auto cut0MUL_FAST = [] (const Data_t &data)->bool {return data.checkTrgClass("0MUL-B-NOPF-FAST");};
  auto cut0MUL_MUFAST = [] (const Data_t &data)->bool {return data.checkTrgClass("0MUL-B-NOPF-MUFAST");};
  auto cut0MUL_ALLNOTRD = [] (const Data_t &data)->bool {return data.checkTrgClass("0MUL-B-NOPF-ALLNOTRD");};

  auto cutINT7_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("INT7-INPUT");};
  auto cut0TVX_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0TVX-INPUT");};
  auto cut0EMC_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0EMC-INPUT");};
  auto cut0MSL_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0MSL-INPUT");};
  auto cut0MUL_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0MUL-INPUT");};
  auto cut0V0H_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0V0H-INPUT");};
  auto cut0V0M_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0V0M-INPUT");};
  auto cut0VHM_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("0VHM-INPUT");};
  auto cutVHMV0M_INPUT = [] (const Data_t &data)->bool {return data.checkTrgClass("VHMV0M-INPUT");};
  analysis.mCutObjectManager.makeCutBit("noCuts","noCuts",cutNoCuts);
  analysis.mCutObjectManager.makeCutBit("noPileup","Excluded pileup from events",cutNoPileup);
  analysis.mCutObjectManager.makeCutBit("noPileupLowMult","Excluded pileup from events,IsPileupFromSPD()",cutNoPileupLowMult);
  analysis.mCutObjectManager.makeCutBit("PhysSel","Phys sel",cutPhysSel);

  analysis.mCutObjectManager.makeCutBit("vertexGlobal","-10cm<VertexGlobal<+10cm",cutVertexGlobal);
  analysis.mCutObjectManager.makeCutBit("vertexTrack","nCont>0 && |vertexTrack|<30",cutVertexTrack);
  analysis.mCutObjectManager.makeCutBit("vertexSPD","-10cm<VertexGlobal<+10cm",cutVertexSPD);

  analysis.mCutObjectManager.makeCutBit("V0_v1","(timeV0A+timeV0C)>11.5 && (timeV0A+timeV0C)<17.5 && (timeV0A-timeV0C)>5.5 && (timeV0A-timeV0C)<11.5",cutV0_v1);
  analysis.mCutObjectManager.makeCutBit("V0_v2","(timeV0A+timeV0C)>10 && (timeV0A+timeV0C)<18 && (timeV0A-timeV0C)>4 && (timeV0A-timeV0C)<12",cutV0_v2);

  analysis.mCutObjectManager.makeCutBit("cutV0Ellipse","cutV0Ellipse",cutV0Ellipse);
  //10 cuts
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

  analysis.mCutObjectManager.makeCutBit("cut0EMC_CENTNOTRD","cut0EMC_CENTNOTRD",cut0EMC_CENTNOTRD);
  analysis.mCutObjectManager.makeCutBit("cut0EMC_CENT","cut0EMC_CENT",cut0EMC_CENT);
  analysis.mCutObjectManager.makeCutBit("cut0EMC_FAST","cut0EMC_FAST",cut0EMC_FAST);
  analysis.mCutObjectManager.makeCutBit("cut0EMC_MUFAST","cut0EMC_MUFAST",cut0EMC_MUFAST);
  analysis.mCutObjectManager.makeCutBit("cut0EMC_ALLNOTRD","cut0EMC_ALLNOTRD",cut0EMC_ALLNOTRD);

  analysis.mCutObjectManager.makeCutBit("cut0MSL_CENTNOTRD","cut0MSL_CENTNOTRD",cut0MSL_CENTNOTRD);
  analysis.mCutObjectManager.makeCutBit("cut0MSL_CENT","cut0MSL_CENT",cut0MSL_CENT);
  analysis.mCutObjectManager.makeCutBit("cut0MSL_FAST","cut0MSL_FAST",cut0MSL_FAST);
  analysis.mCutObjectManager.makeCutBit("cut0MSL_MUFAST","cut0MSL_MUFAST",cut0MSL_MUFAST);
  analysis.mCutObjectManager.makeCutBit("cut0MSL_ALLNOTRD","cut0MSL_ALLNOTRD",cut0MSL_ALLNOTRD);

  analysis.mCutObjectManager.makeCutBit("cut0MUL_CENTNOTRD","cut0MUL_CENTNOTRD",cut0MUL_CENTNOTRD);
  analysis.mCutObjectManager.makeCutBit("cut0MUL_CENT","cut0MUL_CENT",cut0MUL_CENT);
  analysis.mCutObjectManager.makeCutBit("cut0MUL_FAST","cut0MUL_FAST",cut0MUL_FAST);
  analysis.mCutObjectManager.makeCutBit("cut0MUL_MUFAST","cut0MUL_MUFAST",cut0MUL_MUFAST);
  analysis.mCutObjectManager.makeCutBit("cut0MUL_ALLNOTRD","cut0MUL_ALLNOTRD",cut0MUL_ALLNOTRD);

  analysis.mCutObjectManager.makeCutBit("cutINT7_INPUT","cutINT7_INPUT",cutINT7_INPUT);
  analysis.mCutObjectManager.makeCutBit("cut0TVX_INPUT","cut0TVX_INPUT",cut0TVX_INPUT);
  analysis.mCutObjectManager.makeCutBit("cut0EMC_INPUT","cut0EMC_INPUT",cut0EMC_INPUT);
  analysis.mCutObjectManager.makeCutBit("cut0MSL_INPUT","cut0MSL_INPUT",cut0MSL_INPUT);
  analysis.mCutObjectManager.makeCutBit("cut0MUL_INPUT","cut0MUL_INPUT",cut0MUL_INPUT);

  analysis.mCutObjectManager.makeCutBit("cut0V0H_INPUT","cut0V0H_INPUT",cut0V0H_INPUT);
  analysis.mCutObjectManager.makeCutBit("cut0V0M_INPUT","cut0V0M_INPUT",cut0V0M_INPUT);
  analysis.mCutObjectManager.makeCutBit("cut0VHM_INPUT","cut0VHM_INPUT",cut0VHM_INPUT);
  analysis.mCutObjectManager.makeCutBit("cutVHMV0M_INPUT","cutVHMV0M_INPUT",cutVHMV0M_INPUT);

  
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
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 PhysSel")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileup")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileupLowMult")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 PhysSel")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileup PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileup PhysSel vertexTrack")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileupLowMult PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileupLowMult PhysSel vertexTrack")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileup PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileup PhysSel vertexTrack")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileupLowMult PhysSel vertexGlobal")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v2 noPileupLowMult PhysSel vertexTrack")
    ,analysis.mCutObjectManager.makeNamedEventCutID("V0_v1 noPileup PhysSel vertexTrack cutV0Ellipse")
/*    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_CENTNOTRD")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_CENT")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_FAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_MUFAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutC0TVX_ALLNOTRD")

    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_CENTNOTRD")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_CENT")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_FAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_MUFAST")
    ,analysis.mCutObjectManager.makeNamedEventCutID("cutCINT7_ALLNOTRD")
*/
  };
  //Trigger cluster names
  std::vector<std::string> vecTrgNames = {
    "cutC0TVX"
    ,"cutCINT7"
    ,"cut0EMC"
    ,"cut0MSL"
    ,"cut0MUL"
    ,"cut0V0H"
    ,"cut0V0M"
    ,"cut0VHM"
  };
  std::vector<std::string> vecTrgClusterNames = {
     "CENTNOTRD"
      ,"CENT"
      ,"FAST"
      ,"MUFAST"
    ,"ALLNOTRD"
  };

  //Making triggers
  std::map<std::string,std::string> mapCutTrgs;
  std::cout<<"\n======Hist to trigger cuts=====";
  for(const auto& trgNames: vecTrgNames) {
    for(const auto& trgCluster: vecTrgClusterNames) {
      std::string trgCutName = trgNames+"_"+trgCluster;
      std::string trgHistName = "hist_" + trgNames+"_"+trgCluster;
      if(trgCutName.find("cutCINT7")==std::string::npos) {
        trgCutName+=std::string{" cutCINT7_"+trgCluster};
      }
      std::cout<<"\n"<<trgHistName<<"|"<<trgCutName;
      mapCutTrgs.insert({trgHistName,trgCutName});
    }
  }

  mapCutTrgs.insert({"hist_cutInputINT7","cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0TVX","cut0TVX_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0EMC","cut0EMC_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0MSL","cut0MSL_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0MUL","cut0MUL_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0V0H","cut0V0H_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0V0M","cut0V0M_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0VHM","cut0VHM_INPUT"});
  mapCutTrgs.insert({"hist_cutInputVHMV0M","cutVHMV0M_INPUT"});

  mapCutTrgs.insert({"hist_cutInput0TVX_AND_INT7","cut0TVX_INPUT cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0EMC_AND_INT7","cut0EMC_INPUT cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0MSL_AND_INT7","cut0MSL_INPUT cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0MUL_AND_INT7","cut0MUL_INPUT cutINT7_INPUT"});

  mapCutTrgs.insert({"hist_cutInput0V0H_AND_INT7","cut0V0H_INPUT cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0V0M_AND_INT7","cut0V0M_INPUT cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInput0VHM_AND_INT7","cut0VHM_INPUT cutINT7_INPUT"});
  mapCutTrgs.insert({"hist_cutInputVHMV0M_AND_INT7","cutVHMV0M_INPUT cutINT7_INPUT"});
  std::cout<<"\n=================================\n";
  for(const auto& entry:mapCutTrgs) {
    std::string histName = entry.first;
    std::string cutName = entry.second;
    analysis.mDataOutputManager.makeDataOutput(histName
                                               ,histName
                                               ,vecEventCutID
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
    typedef std::function<void(const Data_t &,DataOutputObject2D_t *)> FuncFill_t;
    typedef helpers::hists::HistHelper<DataOutput2D_t>::HistParam HistParam;
    OutputDataEntry(std::string histNameTitle,Int_t nBinsX,Double_t lowBinX,Double_t upBinX
                    ,Int_t nBinsY,Double_t lowBinY,Double_t upBinY
                    ,FuncFill_t funcFill,std::string cutName):
      mFuncFill(funcFill),mCutName(cutName)
    {
       mHistParam.mHistName=histNameTitle;
       mHistParam.mHistTitle=histNameTitle;
       mHistParam.mNBinsX=nBinsX;
       mHistParam.mLowBinX=lowBinX;
       mHistParam.mUpBinX=upBinX;
       mHistParam.mNBinsY=nBinsY;
       mHistParam.mLowBinY=lowBinY;
       mHistParam.mUpBinY=upBinY;
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

  //////////////////////////////////////////////////////////////
  //
  auto funcFillV0AV0C = [](const Data_t &data,DataOutputObject2D_t *outputPtr) {
    outputPtr->mDataOutput.Fill(data.fTimeV0A,data.fTimeV0C);
  };
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hV0AV0C_0TVX_FullCuts",2000,0,20,2000,0,20
        ,funcFillV0AV0C
        ,"V0_v1 noPileup PhysSel vertexTrack cutINT7_INPUT cut0TVX_INPUT");
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hV0AV0C_INT7_FullCuts",2000,0,20,2000,0,20
        ,funcFillV0AV0C
        ,"V0_v1 noPileup PhysSel vertexTrack cutINT7_INPUT");

  //Ellipse
  //C0TVX
  vecOutputRatioC0TVX.emplace_back(
        "hV0AV0C_0TVX_FullCuts_el",2000,0,20,2000,0,20
        ,funcFillV0AV0C
        ,"V0_v1 noPileup PhysSel vertexTrack cutINT7_INPUT cut0TVX_INPUT cutV0Ellipse");
  //CINT7
  vecOutputRatioCINT7.emplace_back(
        "hV0AV0C_INT7_FullCuts_el",2000,0,20,2000,0,20
        ,funcFillV0AV0C
        ,"V0_v1 noPileup PhysSel vertexTrack cutINT7_INPUT cutV0Ellipse");
  //////////////////////////////////////////////////////////////
  //Prepare ratio names for calculations
  std::vector<OutputDataEntry> vecOutputRatioAll;
  std::copy(vecOutputRatioC0TVX.begin(),vecOutputRatioC0TVX.end(),std::back_inserter(vecOutputRatioAll));
  std::copy(vecOutputRatioCINT7.begin(),vecOutputRatioCINT7.end(),std::back_inserter(vecOutputRatioAll));
  for(const auto &entry:vecOutputRatioAll) {
    //auto histName = entry.mHistName;
    std::get<0>(analysis.mDataOutputTuple).makeDataOutput(
          DataOutput2D_t{entry.mHistParam.mHistName.c_str(),entry.mHistParam.mHistTitle.c_str()
                       ,entry.mHistParam.mNBinsX,entry.mHistParam.mLowBinX,entry.mHistParam.mUpBinX
                       ,entry.mHistParam.mNBinsY,entry.mHistParam.mLowBinY,entry.mHistParam.mUpBinY}
          ,entry.mFuncFill
          ,analysis.mCutObjectManager.makeNamedEventCutID(entry.mCutName));
  }
  //////////////////////////////////////////
  /// RUNNING ANALYSIS /////////////////////
  //////////////////////////////////////////
  analysis.run();
  //////////////////////////////////////////
  /// SAVING RESULTS ///////////////////////
  //////////////////////////////////////////
  analysis.saveResult();

}
