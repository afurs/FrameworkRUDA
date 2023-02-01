#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/LookUpTable.h"
// #include "DataFormatsParameters/GRPLHCIFData.h"
#include "FT0Base/Geometry.h"

#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include "ROOT/TProcessExecutor.hxx"

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/InputFileManager.h"
#include "CommonRUDA/HelperHists.h"
#include "CommonRUDA/OutputHistManager.h"

#include "O2_RUDA/CCDB.h"
#include "O2_RUDA/DetectorFIT.h"
#include "O2_RUDA/EventChDataParams.h"

#include <bitset>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <regex>
#include <utility>
#include <algorithm>
#include <numeric>
#include <cmath>
/*
HINTS:

Trigger getters
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/common/include/DataFormatsFIT/Triggers.h#L59-L71

PM bits
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/FT0/include/DataFormatsFT0/ChannelData.h#L37-L44

*/

//Choose detector type
using Detector = detectorFIT::DetectorFT0;
//Detector parameters
using Digit = Detector::Digit_t;
using ChannelData = Detector::ChannelData_t;
const int sNchannelsA = Detector::sNchannelsA;
const int sNchannelsC = Detector::sNchannelsC;
const int sNchannelsAC = Detector::sNchannelsAC;
const char* sDigitBranchName = Detector::sDigitBranchName;
const char* sChannelDataBranchName = Detector::sChannelDataBranchName;

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using LUT = o2::fit::LookupTableBase<>;
using Axis = helpers::hists::Axis;
using EventTimeAmp = digits::EventChDataParamsFIT<Detector>;
using EntryCCDB = utilities::ccdb::EntryCCDB;

const float sNSperTimeChannel=o2::ft0::Geometry::ChannelWidth*1e-3; // nanoseconds per time channel, 0.01302
const float sNS2Cm = 29.97; // light NS to Centimetres
const int sNBC = 3564; // Number of BCs per Orbit
const int sOrbitPerTF=128;
const int sNBCperTF = sNBC*sOrbitPerTF;
const int sTFrate = 88;
const double sTFLengthSec = 1./sTFrate;
const int sNBCperSec = sNBC*sOrbitPerTF*sTFrate;


const int sNchannelsInnerA = 32;
const int sOrGate = 153; // in TDC units

void runAnalysisFull(const std::vector<std::string> &vecPathToSrc, const std::string &pathToDst="hists",std::size_t nParallelJobs=7,std::size_t nChunksPerRun=2, const std::set<unsigned int> &setRunnum={});
void processDigits(unsigned int runnum, const std::vector<std::string> &vecFilepathInput, const std::string &filepathOutput,const EntryCCDB &entryCCDB)
{
  //Load libraries and define types
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Constants
  const std::string treeName="o2sim";
   const std::map<unsigned int, std::string> mMapTrgNames = {
    {o2::fit::Triggers::bitA+1, "OrA"}
    , {o2::fit::Triggers::bitC+1, "OrC"}
    , {o2::fit::Triggers::bitCen+1, "Central"}
    , {o2::fit::Triggers::bitSCen+1, "Semicentral"}
    , {o2::fit::Triggers::bitVertex+1, "Vertex"}
/*
    , {o2::fit::Triggers::bitLaser+1, "Laser"},
    , {o2::fit::Triggers::bitOutputsAreBlocked+1,"OutputAreBlocked"},
    , {o2::fit::Triggers::bitDataIsValid+1, "DataIsValid"}*/
    };

//  enum ETriggers {kOrA,kOrC,kCent,kSemiCent,kVertex,kAllEvents};

  std::map<unsigned int, std::string> mMapAllTrgNames = mMapTrgNames;
  const unsigned int mBitAllEvents = mMapTrgNames.size();
  mMapAllTrgNames.insert({mBitAllEvents+1,"No trg cut"});

  const std::map<unsigned int, std::string> mMapPMbits = {
    {o2::ft0::ChannelData::kNumberADC+1, "NumberADC" },
    {o2::ft0::ChannelData::kIsDoubleEvent+1, "IsDoubleEvent" },
    {o2::ft0::ChannelData::kIsTimeInfoNOTvalid+1, "IsTimeInfoNOTvalid" },
    {o2::ft0::ChannelData::kIsCFDinADCgate+1, "IsCFDinADCgate" },
    {o2::ft0::ChannelData::kIsTimeInfoLate+1, "IsTimeInfoLate" },
    {o2::ft0::ChannelData::kIsAmpHigh+1, "IsAmpHigh" },
    {o2::ft0::ChannelData::kIsEventInTVDC+1, "IsEventInTVDC" },
    {o2::ft0::ChannelData::kIsTimeInfoLost+1, "IsTimeInfoLost" }};
  const std::array<uint8_t, 8> bitValues{1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7};
  const std::array<std::vector<uint8_t>, 256> arrBitValues;
  //Orbit limits for timeseries
  const uint32_t firstOrbit = entryCCDB.mFirstOrbit;
  const uint32_t lastOrbit = entryCCDB.mLastOrbit;
  auto orbit2Sec = [](uint32_t orbit) {
    uint64_t sec = orbit/(sOrbitPerTF * sTFrate);
    return sec;
  };
  const uint64_t firstTF = firstOrbit/sOrbitPerTF - 1;
  const uint64_t lastTF = lastOrbit/sOrbitPerTF + 1;
  const uint64_t firstSec = orbit2Sec(firstOrbit);
  const uint64_t lastSec = orbit2Sec(lastOrbit);


  //PM bits
  const uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  const uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);
  const uint8_t pmBitsToCheck = pmBitsGood | pmBitsBad; //All except kNumberADC

  //Collision schema
  std::bitset<sNBC> collBC{}, collBC_A{}, collBC_C{}, collBC_E{};
  enum EBeamMask {
    kEmpty,
    kBeam,
    kBeamA, // beamA = beam 0,
    kBeamC, // beamC = beam 1
    kAny
  };
  std::map<unsigned int,std::string> mMapBeamMask{{EBeamMask::kEmpty+1,"Empty"},
                                                          {EBeamMask::kBeam+1,"Beam"},
                                                          {EBeamMask::kBeamA+1,"BeamA(0)"},
                                                          {EBeamMask::kBeamC+1,"BeamC(1)"},
                                                          {EBeamMask::kAny+1,"Any beam mask"}};
  const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData = entryCCDB.mGRPLHCIFData;
  if(ptrGRPLHCIFData!=nullptr) {
    const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }
  std::array<int,sNBC> prevCollisionBCperBC{};
  std::array<int,sNBC> arrBeamMask{};
  {
    int prevBC=-1;
    for(int iBC=0;iBC<sNBC;iBC++) {
      prevCollisionBCperBC[iBC] = prevBC;
      if(collBC.test(iBC) ){
        prevBC=iBC;
        prevCollisionBCperBC[iBC] = -1;
        arrBeamMask[iBC] = EBeamMask::kBeam;
      }
      else if(collBC_A.test(iBC)) {
        arrBeamMask[iBC] = EBeamMask::kBeamA;
      }
      else if(collBC_C.test(iBC)) {
        arrBeamMask[iBC] = EBeamMask::kBeamC;
      }
      else {
        arrBeamMask[iBC] = EBeamMask::kEmpty;
      }
    }
  }
  //Map all beam + trg
  std::map<unsigned int,std::string> mMapTrgAndBeamMask{};
  //Functor hashed beamMask-TrgBit
  const int nTrgBits = mMapAllTrgNames.size(); // 5 trg bits + all event(no trg cut) bit
  auto getBeamMaskAndTrg = [&nTrgBits] (int beamMask, int trgBitPos) {
    return nTrgBits * beamMask + trgBitPos;
  };
  //
  for(auto const &entryBeamMask : mMapBeamMask) {
    for(const auto &entryTrg: mMapAllTrgNames) {
      const auto iBin = getBeamMaskAndTrg(entryBeamMask.first - 1, entryTrg.first - 1) + 1;
      const std::string binName = entryBeamMask.second + std::string{" + "} + entryTrg.second;
      mMapTrgAndBeamMask.insert({iBin,binName});
    }
  }
  //Setttings for afterpulse and reflection BCs
  std::array<int, sNchannelsAC> arrAfterpulseShiftBC{};
  std::array<int, sNchannelsAC> arrReflectionShiftBC{};
  std::array<std::bitset<sNBC>, sNchannelsAC> arrAfterpulse_BC{};
  std::array<std::bitset<sNBC>, sNchannelsAC> arrReflection_BC{};
  const uint16_t bcSingle=1000;
  for(int iCh=0;iCh<sNchannelsAC;iCh++) {
    arrAfterpulseShiftBC[iCh] = 1;
    if(iCh<sNchannelsA) {
      arrReflectionShiftBC[iCh] = 11;
    }
    else {
      arrReflectionShiftBC[iCh] = 10;
    }
    for(int iBC=0;iBC<bcSingle; iBC++) {
      if(collBC.test(iBC)) {
        arrAfterpulse_BC[iCh].set(iBC+arrAfterpulseShiftBC[iCh]);
        arrReflection_BC[iCh].set(iBC+arrReflectionShiftBC[iCh]);
      }
    }
  }
  //Vectors with channel numbers per side
  std::vector<int> vecChannelsA(sNchannelsA);
  std::iota(vecChannelsA.begin(), vecChannelsA.end(), 0);
  std::vector<int> vecChannelsC(sNchannelsC);
  std::iota(vecChannelsC.begin(), vecChannelsC.end(), sNchannelsA);
  //Output manager
  utilities::OutputHistManager outputManager(utilities::OutputManager::getFilepathPrefix(filepathOutput)+std::string{"_"});
  //Functors for hist per side creation
  auto makeNameTitle = [](const std::vector<int> &vecIDs,const std::string &name,const std::string &title) {
    std::vector< std::pair <std::string, std::string> > vecNameTitle{};
    for(const auto& num: vecIDs) {
      vecNameTitle.emplace_back(std::string{Form(name.c_str(),num)},std::string{Form(title.c_str(),num)});
    }
    return vecNameTitle;
  };
  auto make2DHistsPerSide = [&makeNameTitle,&outputManager,&vecChannelsA,&vecChannelsC](const std::string &filename, const std::string &name,const std::string &title,const Axis &axisX, const Axis &axisY)->std::array<TH2F*,sNchannelsAC> {
    outputManager.registerCurrentList(Form(filename.c_str(),"A"));
    auto vecHistsSideA = outputManager.registerHist<TH2F>(makeNameTitle(vecChannelsA,name,title),axisX,axisY);
    outputManager.registerCurrentList(Form(filename.c_str(),"C"));
    auto vecHistsSideC = outputManager.registerHist<TH2F>(makeNameTitle(vecChannelsC,name,title),axisX,axisY);
    std::array<TH2F*,sNchannelsAC> arrHists{};
    for(int i=0;i<vecHistsSideA.size();i++) {
      const auto chID = vecChannelsA[i];
      arrHists[chID] = vecHistsSideA[i];
    }
    for(int i=0;i<vecHistsSideC.size();i++) {
      const auto chID = vecChannelsC[i];
//      std::cout<<std::endl<<i<<"|"<<chID<<std::endl;
      arrHists[chID] = vecHistsSideC[i];
    }
    return arrHists;
  };
  //Axes
  const Axis axisChannels(sNchannelsAC,0,sNchannelsAC);
  const Axis axisChannelsA(sNchannelsA,0,sNchannelsA);
  const Axis axisChannelsC(sNchannelsC,0,sNchannelsC);

  const Axis axisBC(sNBC,0,sNBC,"BC");
  const Axis axisBCdistance(sNBC,0,sNBC,"BC distance");
  const Axis axisTimeseriesSec(lastSec-firstSec,firstSec,lastSec,"Time since SOR [seconds]");

  const Axis axisTriggers(mMapTrgNames.size(),0,mMapTrgNames.size(),"Triggers",mMapTrgNames);
  const Axis axisBeam(mMapBeamMask.size(),0,mMapBeamMask.size(),"Beam mask",mMapBeamMask);
  const Axis axisBeamAndTriggers(mMapTrgAndBeamMask.size(),0,mMapTrgAndBeamMask.size(),"Beam mask and triggers",mMapTrgAndBeamMask);

  const Axis axisVertex(400,-200.,200.);
  const Axis axisMeanTimeNoCut(4000,-2000.,2000.,"Mean time, no cuts [TDC]");
  const Axis axisMeanTimeNoTimeCut(4000,-2000.,2000.,"Mean time, no time window cut [TDC]");

  const Axis axisMeanTime(4000,-2000.,2000.,"Mean time [TDC]");

  const Axis axisTimeFull(4000,-2000.,2000.,"Time [TDC]");
  const Axis axisAmplitudeFull(4100,0.,4100.,"Amp [ADC]");

  const Axis axisTimeLowBins(200,-2000.,2000.);
  const Axis axisAmplitudeLowBins(205,0.,4100.);

  const Axis axisTimeLowBins4Corr(1000,-2000.,2000.);
  const Axis axisLowAmplitudes(100,0.,100.);
  const Axis axisLowAmplitudes2(500,0.,500.);

  const Axis axisTimeRange500(1000,-500.,500.);
  const Axis axisAmplitudeBins2(2050,0.,4100.);

  const Axis axisAmplitudeBins4(1025,0.,4100.);

  const Axis axisSumAmplBins10(1200,0.,12000.);
  const Axis axisSumAmplitude(10000,0.,100000.);

  const Axis axisSumAmplitudeInnerSideA(2000,0.,100000.,"Sum amp, inner side-A [ADC]");
  const Axis axisSumAmplitudeOuterSideA(2000,0.,100000.,"Sum amp, outer side-A [ADC]");

  const Axis axisSumAmplitudeSideA(2000,0.,200000., "Sum amp, side-A [ADC]");
  const Axis axisSumAmplitudeSideC(2000,0.,200000., "Sum amp, side-C [ADC]");


  //Amplitude vs Time
//  auto hArrLowAmpVsTimeSingleMCP = make2DHistsPerSide("LowAmpVsTimeSingleMCP_side%s","hLowAmpVsTime%i","Low amplitude vs time, channelID %i (collision BC, vertex, good PM bits, single channel within MCP);Amp [ADC];Time [TDC]",axisLowAmplitudes,axisTimeRange500);

//  auto arrAmpVsTime = make2DHistsPerSide("AmpVsTime_side%s","hAmpVsTime%i","Amplitude vs time, channelID %i;Amp [ADC];Time [TDC]",axisAmplitudeLowBins,axisTimeLowBins);
  auto hArrAmpVsTime = make2DHistsPerSide("AmpVsTime_side%s","hAmpVsTime%i","Amplitude vs time, channelID %i (collisionBC + vertex trigger + good PM bits);Amp [ADC];Time [TDC]",axisAmplitudeBins2,axisTimeRange500);
//*  auto arrAmpVsSumAmpMCP = make2DHistsPerSide("AmpVsSumAmpMCP_side%s","hAmpVsSumAmpMCP%i","Amplitude vs sum amp within MCP(except given chID), channelID %i (collisionBC + vertex trigger + good PM bits);Amp [ADC];SumAmp [ADC]",axisAmplitudeBins4,axisSumAmplBins10);

//  auto arrTimeVsVertex = make2DHistsPerSide("TimeVsVertex_side%s","hTimeVsVertex%i","Vertex vs time, channelID %i;Vertex[cm];Time [TDC]",axisVertex,axisTimeRange500);

  //Spectra list
  outputManager.registerCurrentList("Spectra");

  auto hAmpPerChannelNoCuts = outputManager.registerHist<TH2F>("hAmpPerChannelNoCuts","Amplitude, w/o any cuts;ChannelID",axisChannels,axisAmplitudeFull);
  //auto hAmpPerChannel = outputManager.registerHist<TH2F>("hAmpPerChannel","Amplitude(collision BC + vertex trigger + good PM bits);ChannelID",axisChannels,axisAmplitudeFull);

  std::vector<TH2F *> vecHistAmpPerChannelTrg{};
  for(const auto &en: mMapTrgNames) {
    const std::string name = "hAmpPerChannelTrg" + en.second;
    const std::string title = "Amplitude(collision BC + trigger " + en.second + " + good PM bits)";
    vecHistAmpPerChannelTrg.push_back(outputManager.registerHist<TH2F>(name,title,axisChannels,axisAmplitudeFull));
  }
  auto hSumAmpInnerVsOuter_sideA = outputManager.registerHist<TH2F>("hSumAmpInnerVsOuter_sideA","Sum amplitude inner vs outer part at side-A,(collision BC + vertex trigger + good PM bits);",axisSumAmplitudeInnerSideA,axisSumAmplitudeOuterSideA);
  auto hSumAmpInnerSideA_vs_SideC = outputManager.registerHist<TH2F>("hSumAmpInnerSideA_vs_SideC","Sum amplitude inner side-A vs side-C,(collision BC + vertex trigger + good PM bits)",axisSumAmplitudeInnerSideA,axisSumAmplitudeSideA);
  auto hSumAmpOuterSideA_vs_SideC = outputManager.registerHist<TH2F>("hSumAmpOuterSideA_vs_SideC","Sum amplitude outer side-A vs side-C,(collision BC + vertex trigger + good PM bits)",axisSumAmplitudeOuterSideA,axisSumAmplitudeSideA);
  auto hSumAmpSideA_vs_SideC = outputManager.registerHist<TH2F>("hSumAmpSideA_vs_SideC","Sum amplitude outer side-A vs side-C,(collision BC + vertex trigger + good PM bits)",axisSumAmplitudeOuterSideA,axisSumAmplitudeSideC);

  auto hNchSideA_vs_SideC = outputManager.registerHist<TH2F>("hNchSideA_vs_SideC","Number of channels side-A vs side-C (collision BC + vertex trigger + good PM bits);Nchannels side-A;Nchannels side-C",axisChannels,axisChannels);
  auto hNchSideAinner_vs_SideC = outputManager.registerHist<TH2F>("hNchSideAinner_vs_SideC","Number of channels inner side-A vs side-C (collision BC + vertex trigger + good PM bits);Nchannels inner side-A;Nchannels side-C",axisChannels,axisChannels);
  auto hNchSideAouter_vs_SideC = outputManager.registerHist<TH2F>("hNchSideAouter_vs_SideC","Number of channels outer side-A vs side-C (collision BC + vertex trigger + good PM bits);Nchannels outer side-A;Nchannels side-C",axisChannels,axisChannels);
  auto hNchSideAinner_vs_SideAouter = outputManager.registerHist<TH2F>("hNchSideAinner_vs_SideAouter","Number of channels inner side-A vs outer side-A (collision BC + vertex trigger + good PM bits);Nchannels inner side-A;Nchannels outer side-C",axisChannels,axisChannels);


  auto hMeanTimeA_cuts_vs_noCuts = outputManager.registerHist<TH2F>("hMeanTimeA_cuts_vs_noCuts","Mean time side-A, with time window cut(abs(time)<153 TDC ) vs no cuts",axisMeanTime,axisMeanTimeNoCut);
  auto hMeanTimeA_cuts_vs_noTimeCut = outputManager.registerHist<TH2F>("hMeanTimeA_cuts_vs_noTimeCut","Mean time side-A, with time window cut(abs(time)<153 TDC ) vs w/o time window cut",axisMeanTime,axisMeanTimeNoTimeCut);
  auto hMeanTimeA_cuts_vs_noCuts_vrtTrg = outputManager.registerHist<TH2F>("hMeanTimeA_cuts_vs_noCuts_vrtTrg","Mean time side-A, with time window cut(abs(time)<153 TDC ) vs no cuts, in collision BC + vertex trigger",axisMeanTime,axisMeanTimeNoCut);
  auto hMeanTimeA_cuts_vs_noTimeCut_vrtTrg = outputManager.registerHist<TH2F>("hMeanTimeA_cuts_vs_noTimeCut_vrtTrg","Mean time side-A, with time window cut(abs(time)<153 TDC ) vs w/o time window cut, in collision BC + vertex trigger",axisMeanTime,axisMeanTimeNoTimeCut);

  auto hMeanTimeC_cuts_vs_noCuts = outputManager.registerHist<TH2F>("hMeanTimeC_cuts_vs_noCuts","Mean time side-C, with time window cut(abs(time)<153 TDC ) vs no cuts",axisMeanTime,axisMeanTimeNoCut);
  auto hMeanTimeC_cuts_vs_noTimeCut = outputManager.registerHist<TH2F>("hMeanTimeC_cuts_vs_noTimeCut","Mean time side-C, with time window cut(abs(time)<153 TDC ) vs w/o time window cut",axisMeanTime,axisMeanTimeNoTimeCut);
  auto hMeanTimeC_cuts_vs_noCuts_vrtTrg = outputManager.registerHist<TH2F>("hMeanTimeC_cuts_vs_noCuts_vrtTrg","Mean time side-C, with time window cut(abs(time)<153 TDC ) vs no cuts, in collision BC + vertex trigger",axisMeanTime,axisMeanTimeNoCut);
  auto hMeanTimeC_cuts_vs_noTimeCut_vrtTrg = outputManager.registerHist<TH2F>("hMeanTimeC_cuts_vs_noTimeCut_vrtTrg","Mean time side-C, with time window cut(abs(time)<153 TDC ) vs w/o time window cut, in collision BC + vertex trigger",axisMeanTime,axisMeanTimeNoTimeCut);

  auto hNchA_cuts_vs_noCuts = outputManager.registerHist<TH2F>("hNchA_cuts_vs_noCuts","Nchannels side-A, with time window cut(abs(time)<153 TDC ) vs no cuts;N channel, cut;N channel,no cut",axisChannels,axisChannels);
  auto hNchA_cuts_vs_noTimeCut = outputManager.registerHist<TH2F>("hNchA_cuts_vs_noTimeCut","Nchannels side-A, with time window cut(abs(time)<153 TDC ) vs w/o time window cut;N channel, cut;N channel,no cut",axisChannels,axisChannels);
  auto hNchA_cuts_vs_noCuts_vrtTrg = outputManager.registerHist<TH2F>("hNchA_cuts_vs_noCuts_vrtTrg","Nchannels time side-A, with time window cut(abs(time)<153 TDC ) vs no cuts, in collision BC + vertex trigger;N channel, cut;N channel,no cut",axisChannels,axisChannels);
  auto hNchA_cuts_vs_noTimeCut_vrtTrg = outputManager.registerHist<TH2F>("hNchA_cuts_vs_noTimeCut_vrtTrg","Nchannels side-A, with time window cut(abs(time)<153 TDC ) vs w/o time window cut, in collision BC + vertex trigger;N channel, cut;N channel,no cut",axisChannels,axisChannels);

  auto hNchC_cuts_vs_noCuts = outputManager.registerHist<TH2F>("hNchC_cuts_vs_noCuts","Nchannels side-C, with time window cut(abs(time)<153 TDC ) vs no cuts;N channel, cut;N channel,no cut",axisChannels,axisChannels);
  auto hNchC_cuts_vs_noTimeCut = outputManager.registerHist<TH2F>("hNchC_cuts_vs_noTimeCut","Nchannels side-C, with time window cut(abs(time)<153 TDC ) vs w/o time window cut;N channel, cut;N channel,no cut",axisChannels,axisChannels);
  auto hNchC_cuts_vs_noCuts_vrtTrg = outputManager.registerHist<TH2F>("hNchC_cuts_vs_noCuts_vrtTrg","Nchannels side-C, with time window cut(abs(time)<153 TDC ) vs no cuts, in collision BC + vertex trigger;N channel, cut;N channel,no cut",axisChannels,axisChannels);
  auto hNchC_cuts_vs_noTimeCut_vrtTrg = outputManager.registerHist<TH2F>("hNchC_cuts_vs_noTimeCut_vrtTrg","Nchannels side-C, with time window cut(abs(time)<153 TDC ) vs w/o time window cut, in collision BC + vertex trigger;N channel, cut;N channel,no cut",axisChannels,axisChannels);

  auto hMeanTimeA_diff_noCuts = outputManager.registerHist<TH1F>("hMeanTimeA_diff_noCuts","Difference of mean time side-A, with time window cut(abs(time)<153 TDC ) and no cuts",axisTimeFull);
  auto hMeanTimeA_diff_noTimeCuts = outputManager.registerHist<TH1F>("hMeanTimeA_diff_noTimeCut","Difference of mean time side-A, with time window cut(abs(time)<153 TDC ) and w/o time window cut",axisMeanTime);
  auto hMeanTimeA_diff_noCuts_vrtTrg = outputManager.registerHist<TH1F>("hMeanTimeA_diff_noCuts_vrtTrg","Difference of mean time side-A, with time window cut(abs(time)<153 TDC ) and no cuts, in collision BC + vertex trigger",axisMeanTime);
  auto hMeanTimeA_diff_noTimeCuts_vrtTrg = outputManager.registerHist<TH1F>("hMeanTimeA_diff_vs_noTimeCut_vrtTrg","Difference of mean time side-A, with time window cut(abs(time)<153 TDC ) and w/o time window cut, in collision BC + vertex trigger",axisMeanTime);

  auto hMeanTimeC_diff_noCuts = outputManager.registerHist<TH1F>("hMeanTimeC_diff_noCuts","Difference of mean time side-C, with time window cut(abs(time)<153 TDC ) and no cuts",axisMeanTime);
  auto hMeanTimeC_diff_noTimeCuts = outputManager.registerHist<TH1F>("hMeanTimeC_diff_noTimeCut","Difference of mean time side-C, with time window cut(abs(time)<153 TDC ) and w/o time window cut",axisMeanTime);
  auto hMeanTimeC_diff_noCuts_vrtTrg = outputManager.registerHist<TH1F>("hMeanTimeC_diff_noCuts_vrtTrg","Difference of mean time side-C, with time window cut(abs(time)<153 TDC ) and no cuts, in collision BC + vertex trigger",axisMeanTime);
  auto hMeanTimeC_diff_noTimeCuts_vrtTrg = outputManager.registerHist<TH1F>("hMeanTimeC_diff_vs_noTimeCut_vrtTrg","Difference of mean time side-C, with time window cut(abs(time)<153 TDC ) and w/o time window cut, in collision BC + vertex trigger",axisMeanTime);


  outputManager.registerCurrentList("SpectraPerBC");
  auto hSumAmpA_perBC = outputManager.registerHist<TH2F>("hSumAmpA_perBC","Sum of amplitudes per BC, side A, time in ADC gate;BC;Sum amp [ADC]",axisBC,axisAmplitudeFull);
  auto hSumAmpC_perBC = outputManager.registerHist<TH2F>("hSumAmpC_perBC","Sum of amplitudes per BC, side C, time in ADC gate;BC;Sum amp [ADC]",axisBC,axisAmplitudeFull);

  auto hMeanTimeA_perBC = outputManager.registerHist<TH2F>("hMeanTimeA_perBC","Mean time per BC, side A, time in ADC gate;BC;Mean time [TDC]",axisBC,axisMeanTime);
  auto hMeanTimeC_perBC = outputManager.registerHist<TH2F>("hMeanTimeC_perBC","Mean time per BC, side C, time in ADC gate;BC;Mean time [TDC]",axisBC,axisMeanTime);

  auto hLowSumAmpA_perBCmask = outputManager.registerHist<TH2F>("hLowSumAmpA_perBCmask","Sum of amplitudes per BC mask, side A, time in ADC gate;BC mask;Sum amp [ADC]",axisAmplitudeFull,axisBeamAndTriggers);
  auto hLowSumAmpC_perBCmask = outputManager.registerHist<TH2F>("hLowSumAmpC_perBCmask","Sum of amplitudes per BC mask, side C, time in ADC gate;BC mask;Sum amp [ADC]",axisAmplitudeFull,axisBeamAndTriggers);

  auto hSumAmpA_perBCmask = outputManager.registerHist<TH2F>("hSumAmpA_perBCmask","Sum of amplitudes per BC mask, side A, time in ADC gate;BC mask;Sum amp [ADC]",axisSumAmplitude,axisBeamAndTriggers);
  auto hSumAmpC_perBCmask = outputManager.registerHist<TH2F>("hSumAmpC_perBCmask","Sum of amplitudes per BC mask, side C, time in ADC gate;BC mask;Sum amp [ADC]",axisSumAmplitude,axisBeamAndTriggers);

  auto hBC_perBCmask = outputManager.registerHist<TH2F>("hBC_perBCmask","BC vs BC mask and triggers",axisBC,axisBeamAndTriggers);
//  auto hSumAmpA_distBC = outputManager.registerHist<TH2F>("hSumAmpA_distBC","Sum of amplitudes per distance to collision BC(current BC is not collision), side A, time in ADC gate;Distance to collision BC;Sum amp(non-collision BC)[ADC]",axisBCdistance,axisLowAmplitudes2);
//  auto hSumAmpC_distBC = outputManager.registerHist<TH2F>("hSumAmpC_distBC","Sum of amplitudes per distance to collision BC(current BC is not collision), side C, time in ADC gate;Distance to collision BC;Sum amp(non-collision BC)[ADC]",axisBCdistance,axisLowAmplitudes2);

//  auto hAmpSingleMCP = outputManager.registerHist<TH2F>("hAmpSingleMCP","Amplitude(only single channel within MCP + collision + vertex trigger + good PM bits + abs(time)<153 TDC);ChannelID;Amplitude [ADC]",axisChannels,axisAmplitudeFull);
//  auto hTimeSingleMCP = outputManager.registerHist<TH2F>("hTimeSingleMCP","Time(only single channel within MCP);ChannelID;Time [TDC]",axisChannels,axisTimeFull);
  //Trigger list
  outputManager.registerCurrentList("Triggers");

  auto hTriggerBC = outputManager.registerHist<TH2F>("hTriggersBC","Triggers Vs BC;BC", axisBC, axisTriggers);
  auto hTriggerBeamMask = outputManager.registerHist<TH2F>("hTriggersBeamMask","Triggers Vs Beam mask;BC;Beam mask", axisTriggers, axisBeam);

  auto hChIDperBC_OrA = outputManager.registerHist<TH2F>("hChIDperBC_OrA","ChannelID(kIsEventInTVDC PM bit) vs BC, OrA", axisBC, axisChannels);
  auto hChIDperBC_OrC = outputManager.registerHist<TH2F>("hChIDperBC_OrC","ChannelID(kIsEventInTVDC PM bit) vs BC, OrC", axisBC, axisChannels);
  auto hChIDperBC_Vertex = outputManager.registerHist<TH2F>("hChIDperBC_Vertex","ChannelID(kIsEventInTVDC PM bit) vs BC, Vertex", axisBC, axisChannels);
  outputManager.registerCurrentList("Events");
  auto hEventDistanceNoCuts = outputManager.registerHist<TH2F>("hEventDistanceNoCuts","Distribution of distance to previous event within orbit at FT0 vs current BC, no selection for event tagging", axisBCdistance, axisBC);

  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  std::set<unsigned int> setChIDsSpectraPerBC;
  std::bitset<sNchannelsAC> bitsetChIDsSpectraPerBC;
  for(int iCh=0;iCh<sNchannelsAC;iCh++) {
    setChIDsSpectraPerBC.insert(iCh);
    bitsetChIDsSpectraPerBC.set(iCh);
  }

  ////Map chID to BCs
  std::array<std::bitset<sNBC>,sNchannelsAC> arrMapChID2BC_FT0;
  std::array<int,sNchannelsAC> arrPrevBC2ChID_FT0;
  //std::array<int,sNchannelsAC> arrNprevBC2ChID_FT0;
  auto clearMapChID2BC_FT0 = [&arrMapChID2BC_FT0,&arrPrevBC2ChID_FT0]()->void {
    for(int iCh=0;iCh<sNchannelsAC;iCh++) {
      arrMapChID2BC_FT0[iCh].reset();
      arrPrevBC2ChID_FT0[iCh] = 0;
    }
  };
  auto checkPrevBC_FT0 = [&arrMapChID2BC_FT0] (uint8_t chID,uint16_t bc, uint32_t bcShift=1)->bool {
    return (bc-bcShift>-1) ? arrMapChID2BC_FT0[chID].test(bc-bcShift) : false;
  };
  uint32_t prevOrbit{};
  uint32_t orbitMin{0xffffffff},orbitMax{0};
  uint16_t prevBC{};
  double prevSumAmpA{};
  double prevSumAmpC{};
  bool prevEventCollBC{false};
  ////MCP arrays
  std::array<int, sNchannelsAC/4> arrChannelsPerMCP{};
  std::array<int, sNchannelsAC/4> arrAmpSumPerMCP{};

  ////
  std::vector<Digit> vecDigits;
  std::vector<Digit> *ptrVecDigits = &vecDigits;
  std::vector<ChannelData> vecChannelData;
  std::vector<ChannelData> *ptrVecChannelData = &vecChannelData;
  std::size_t mCntEvents{};
  std::size_t mNTFs{};
  for(const auto &filepathInput:vecFilepathInput) {
    std::cout<<"\nProcessing file: "<<filepathInput<<std::endl;
    TFile fileInput(filepathInput.c_str(),"READ");
    if(!fileInput.IsOpen()) {
      std::cout<<"\nWARNING! CANNOT OPEN FILE: "<<filepathInput<<std::endl;
      return;
    }
    TTree* treeInput = dynamic_cast<TTree*>(fileInput.Get(treeName.c_str()));
    if(treeInput==nullptr) {
      std::cout<<"\nWARNING! CANNOT FIND TREE: "<<treeName<<std::endl;
      return;
    }
    treeInput->SetBranchAddress(sDigitBranchName, &ptrVecDigits);
    treeInput->SetBranchAddress(sChannelDataBranchName, &ptrVecChannelData);
    std::size_t nTotalTFs = treeInput->GetEntries();
    std::size_t nPercents = 10;
    std::size_t stepTF = nPercents*nTotalTFs/100; //step for 10%
    if(stepTF==0) stepTF=nTotalTFs;
    std::cout<<"\nTotal number of TFs: "<<nTotalTFs<<std::endl;

    std::array<int,sNchannelsAC> arrAmp{};
    std::array<int,sNchannelsAC> arrTime{};
    std::array<uint8_t,sNchannelsAC> arrPMbits{};
    std::array<uint16_t, sNchannelsAC> arrChID_lastBC{};
    arrChID_lastBC.fill(0xffff);
    std::array<std::bitset<sNBC>, sNchannelsAC> arrChID_BC{};
    //Accumulating events into vector
    for (int iEvent = 0; iEvent < treeInput->GetEntries(); iEvent++) {
      //Iterating TFs in tree
      treeInput->GetEntry(iEvent);
      mCntEvents++;
      mNTFs++;
      if(mNTFs%stepTF==0) std::cout<<nPercents*mNTFs/stepTF<<"% processed"<<std::endl;
      for(const auto &digit : vecDigits) {
        //Iterating events(Digits)
        const auto& channels = digit.getBunchChannelData(vecChannelData);
        //VARIABLES TO USE
        const auto &ir = digit.getIntRecord();
        const auto &bc = ir.bc;
        const auto &orbit = ir.orbit;
        const auto &trg = digit.mTriggers;
        const auto &trgBits = digit.mTriggers.getTriggersignals();
        const auto &nChA = trg.getNChanA();
        const auto &nChC = trg.getNChanC();
        const auto &sumAmpA = trg.getAmplA();
        const auto &sumAmpC = trg.getAmplC();
        const auto &averageTimeA = trg.getTimeA();
        const auto &averageTimeC = trg.getTimeC();
        if(!trg.getDataIsValid() || trg.getOutputsAreBlocked()) continue;
/******************************
        **PUT HERE CODE FOR PROCESSING DIGITS
******************************/
        const uint64_t secSinceSOR = orbit2Sec(orbit);
        const bool isEventVertex = trg.getVertex();
        const bool isCollision = collBC.test(bc);
        const bool isMaskA = collBC_A.test(bc);
        const bool isMaskC = collBC_C.test(bc);
        const bool isMaskE = !(isCollision || isMaskA || isMaskC);
        const auto beamMask = arrBeamMask[bc];
        const bool isNewOrbit = (prevOrbit!=orbit);
        orbitMin=std::min(orbitMin,orbit);
        orbitMax=std::max(orbitMax,orbit);
        if(isNewOrbit) {
          clearMapChID2BC_FT0();
          arrChID_lastBC.fill(0xffff);
          prevBC=0xffff;
          prevEventCollBC=false;
          prevSumAmpA=0;
          prevSumAmpC=0;
        }
        else {
          hEventDistanceNoCuts->Fill(bc-prevBC,bc);
        }
        std::vector<uint8_t> vecTrgActivated{};
        for(int iTrgBit=0;iTrgBit<5;iTrgBit++)  {
          if(trgBits & (1<<iTrgBit)) {
            vecTrgActivated.emplace_back(iTrgBit);
            hTriggerBC->Fill(bc,static_cast<double>(iTrgBit));
            hBC_perBCmask->Fill(bc,getBeamMaskAndTrg(beamMask,iTrgBit));
            hBC_perBCmask->Fill(bc,getBeamMaskAndTrg(EBeamMask::kAny,iTrgBit));
          }
        }
        hBC_perBCmask->Fill(bc,getBeamMaskAndTrg(beamMask,mBitAllEvents));
        hBC_perBCmask->Fill(bc,getBeamMaskAndTrg(EBeamMask::kAny,mBitAllEvents));

        // MCP
        std::array<int, sNchannelsAC/4> arrChannelsPerMCP{};
        std::array<int, sNchannelsAC/4> arrAmpSumPerMCP{};
        std::map<unsigned int, std::vector<ChannelData> > mapMCP2data{};
        //Inner/Outer sum
        float sumAmpSideA_Inner{0};
        float sumAmpSideA_Outer{0};
        float sumAmpSideC{0};

        int nChSideA_Inner{0};
        int nChSideA_Outer{0};
        int nChSideC{0};
        //TimeA and TimeC
        EventTimeAmp eventTimeAmpCut{};
        EventTimeAmp eventTimeAmpNoCut{};
        EventTimeAmp eventTimeAmpNoTimeCut{};
        EventTimeAmp eventTimeInADCgate{};
        //
        for(const auto &channelData: channels) {
          //Iterating over ChannelData(PM data) per given Event(Digit)
          //VARIABLEES TO USE
          const auto &amp = Detector::amp(channelData);
          const auto &time = Detector::time(channelData);
          const auto &chID = Detector::channelID(channelData);
          const auto &pmBits = Detector::pmBits(channelData);
          if(chID>=sNchannelsAC) continue;
          const double timePs = time * 13.02;
          const unsigned int mcpID = chID/4;
          const bool isPMbitsGood = ((pmBits & pmBitsToCheck) == pmBitsGood);
          const bool isChannelUsedInTrg = pmBits & (1<<o2::ft0::ChannelData::kIsEventInTVDC);
          const bool isInnerSideA = chID<sNchannelsInnerA;
          const bool isOuterSideA = !isInnerSideA && (chID<sNchannelsA);
          const bool isSideC = (chID>=sNchannelsA);
          const bool isInGate = (std::abs(time) <= sOrGate);
          const bool isInADCgate = (std::abs(time) <= 192);
          const bool isAmpInWindow = (amp>=10);
          arrMapChID2BC_FT0[chID].set(bc);
          /*
          if(pmBits & (1<<ChannelDataFT0::kNumberADC)) {
          }
          else {
          }
          */
          /*
          **PUT HERE ANALYSIS CODE
          */
          hAmpPerChannelNoCuts->Fill(chID,amp);
          if(isInADCgate) {
            eventTimeInADCgate.fill(amp,time,chID);
          }
          if(isAmpInWindow) {
            if(isInGate) {
              eventTimeAmpCut.fill(amp,time,chID);
            }
            eventTimeAmpNoTimeCut.fill(amp,time,chID);
          }
          eventTimeAmpNoCut.fill(amp,time,chID);

          if(isEventVertex && isCollision && isPMbitsGood) {
            auto pairRes = mapMCP2data.insert({mcpID,{}});
            pairRes.first->second.emplace_back(channelData);
            //arrLowAmpVsTime[chID]->Fill(amp,time);
            hArrAmpVsTime[chID]->Fill(amp,time);
            arrChannelsPerMCP[mcpID]++;
            arrAmpSumPerMCP[mcpID]+=amp;
            for(const auto &enTrg: vecTrgActivated) {
              vecHistAmpPerChannelTrg[enTrg]->Fill(chID,amp);
            }
            if(isInnerSideA) {
              sumAmpSideA_Inner+=amp;
              nChSideA_Inner++;
            }
            else if(isOuterSideA) {
              sumAmpSideA_Outer+=amp;
              nChSideA_Outer++;
            }
            else if(isSideC) {
              sumAmpSideC+=amp;
              nChSideC++;
            }
          }
          if(isChannelUsedInTrg) {
            if(trg.getOrA()) {
              hChIDperBC_OrA->Fill(bc,static_cast<double>(chID));
            }
            if(trg.getOrC()) {
              hChIDperBC_OrC->Fill(bc,static_cast<double>(chID));
            }
            if(trg.getVertex()) {
              hChIDperBC_Vertex->Fill(bc,static_cast<double>(chID));
            }
          }
          arrChID_lastBC[chID]=bc;
          arrAmp[chID] = amp;
          arrTime[chID] = time;
          arrPMbits[chID] = pmBits;
        }
        eventTimeAmpNoCut.calculate();
        eventTimeAmpCut.calculate();
        eventTimeAmpNoTimeCut.calculate();
        eventTimeInADCgate.calculate();
        if(nChSideA_Inner>0) {
          if(nChSideA_Outer>0) {
            hSumAmpInnerVsOuter_sideA->Fill(sumAmpSideA_Inner, sumAmpSideA_Outer);
          }
          if(nChSideC>0) {
            hSumAmpInnerSideA_vs_SideC->Fill(sumAmpSideA_Inner, sumAmpSideC);
          }
        }
        if(nChSideC>0) {
          if(nChSideA_Outer>0) {
            hSumAmpOuterSideA_vs_SideC->Fill(sumAmpSideA_Outer, sumAmpSideC);
          }
          if(nChSideA_Outer+nChSideA_Inner>0) {
            hSumAmpSideA_vs_SideC->Fill(sumAmpSideA_Inner+sumAmpSideA_Outer, sumAmpSideC);
          }
        }

        hNchSideA_vs_SideC->Fill(nChSideA_Inner+nChSideA_Outer,nChSideC);
        hNchSideAinner_vs_SideC->Fill(nChSideA_Inner,nChSideC);
        hNchSideAouter_vs_SideC->Fill(nChSideA_Outer,nChSideC);
        hNchSideAinner_vs_SideAouter->Fill(nChSideA_Inner,nChSideA_Outer);

        hMeanTimeA_cuts_vs_noCuts->Fill(eventTimeAmpCut.mMeanTimeA,eventTimeAmpNoCut.mMeanTimeA);
        hMeanTimeA_cuts_vs_noTimeCut->Fill(eventTimeAmpCut.mMeanTimeA,eventTimeAmpNoTimeCut.mMeanTimeA);

        hMeanTimeC_cuts_vs_noCuts->Fill(eventTimeAmpCut.mMeanTimeC,eventTimeAmpNoCut.mMeanTimeC);
        hMeanTimeC_cuts_vs_noTimeCut->Fill(eventTimeAmpCut.mMeanTimeC,eventTimeAmpNoTimeCut.mMeanTimeC);

        hNchA_cuts_vs_noCuts->Fill(eventTimeAmpCut.mNchanA,eventTimeAmpNoCut.mNchanA);
        hNchA_cuts_vs_noTimeCut->Fill(eventTimeAmpCut.mNchanA,eventTimeAmpNoTimeCut.mNchanA);

        hNchC_cuts_vs_noCuts->Fill(eventTimeAmpCut.mNchanC,eventTimeAmpNoCut.mNchanC);
        hNchC_cuts_vs_noTimeCut->Fill(eventTimeAmpCut.mNchanC,eventTimeAmpNoTimeCut.mNchanC);

        if(eventTimeAmpNoCut.mNchanA>0)  hMeanTimeA_perBC->Fill(bc,eventTimeAmpNoCut.mMeanTimeA);
        if(eventTimeAmpNoCut.mNchanC>0)  hMeanTimeC_perBC->Fill(bc,eventTimeAmpNoCut.mMeanTimeC);

        if(eventTimeAmpCut.mNchanA>0) {
          if(eventTimeAmpNoCut.mNchanA>0) {
            hMeanTimeA_diff_noCuts->Fill(eventTimeAmpCut.mMeanTimeA - eventTimeAmpNoCut.mMeanTimeA);
            if(isEventVertex && isCollision) {
              hMeanTimeA_diff_noCuts_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeA - eventTimeAmpNoCut.mMeanTimeA);
            }
          }
          if(eventTimeAmpNoTimeCut.mNchanA>0) {
            hMeanTimeA_diff_noTimeCuts->Fill(eventTimeAmpCut.mMeanTimeA - eventTimeAmpNoTimeCut.mMeanTimeA);
            if(isEventVertex && isCollision) {
              hMeanTimeA_diff_noTimeCuts_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeA - eventTimeAmpNoTimeCut.mMeanTimeA);
            }
          }
        }

        if(eventTimeAmpCut.mNchanC>0) {
          if(eventTimeAmpNoCut.mNchanC>0) {
            hMeanTimeC_diff_noCuts->Fill(eventTimeAmpCut.mMeanTimeC - eventTimeAmpNoCut.mMeanTimeC);
            if(isEventVertex && isCollision) {
              hMeanTimeC_diff_noCuts_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeC - eventTimeAmpNoCut.mMeanTimeC);
            }
          }
          if(eventTimeAmpNoTimeCut.mNchanC>0) {
            hMeanTimeC_diff_noTimeCuts->Fill(eventTimeAmpCut.mMeanTimeC - eventTimeAmpNoTimeCut.mMeanTimeC);
            if(isEventVertex && isCollision) {
              hMeanTimeC_diff_noTimeCuts_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeC - eventTimeAmpNoTimeCut.mMeanTimeC);
            }
          }
        }
        if(eventTimeInADCgate.mNchanA>0) {
          hSumAmpA_perBC->Fill(bc,eventTimeInADCgate.mAmpSumA);
          for(const auto &enTrg: vecTrgActivated) {
            hLowSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(beamMask,enTrg));
            hSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(beamMask,enTrg));

            hLowSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(EBeamMask::kAny,enTrg));
            hSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(EBeamMask::kAny,enTrg));
          }
          hLowSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(beamMask,mBitAllEvents));
          hSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(beamMask,mBitAllEvents));

          hLowSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(EBeamMask::kAny,mBitAllEvents));
          hSumAmpA_perBCmask->Fill(eventTimeInADCgate.mAmpSumA,getBeamMaskAndTrg(EBeamMask::kAny,mBitAllEvents));
        }
        if(eventTimeInADCgate.mNchanC>0) {
          hSumAmpC_perBC->Fill(bc,eventTimeInADCgate.mAmpSumC);
          for(const auto &enTrg: vecTrgActivated) {
            hLowSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(beamMask,enTrg));
            hSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(beamMask,enTrg));

            hLowSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(EBeamMask::kAny,enTrg));
            hSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(EBeamMask::kAny,enTrg));
          }
          hLowSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(beamMask,mBitAllEvents));
          hSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(beamMask,mBitAllEvents));

          hLowSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(EBeamMask::kAny,mBitAllEvents));
          hSumAmpC_perBCmask->Fill(eventTimeInADCgate.mAmpSumC,getBeamMaskAndTrg(EBeamMask::kAny,mBitAllEvents));
        }
        /*
        if(prevEventCollBC) {
          hSumAmpA_distBC->Fill(bc-prevBC,eventTimeInADCgate.mAmpSumA);
        }
        */

        if(isEventVertex && isCollision) {
          hMeanTimeA_cuts_vs_noCuts_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeA,eventTimeAmpNoCut.mMeanTimeA);
          hMeanTimeA_cuts_vs_noTimeCut_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeA,eventTimeAmpNoTimeCut.mMeanTimeA);

          hMeanTimeC_cuts_vs_noTimeCut_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeC,eventTimeAmpNoTimeCut.mMeanTimeC);
          hMeanTimeC_cuts_vs_noCuts_vrtTrg->Fill(eventTimeAmpCut.mMeanTimeC,eventTimeAmpNoCut.mMeanTimeC);

          hNchA_cuts_vs_noCuts_vrtTrg->Fill(eventTimeAmpCut.mNchanA,eventTimeAmpNoCut.mNchanA);
          hNchA_cuts_vs_noTimeCut_vrtTrg->Fill(eventTimeAmpCut.mNchanA,eventTimeAmpNoTimeCut.mNchanA);

          hNchC_cuts_vs_noTimeCut_vrtTrg->Fill(eventTimeAmpCut.mNchanC,eventTimeAmpNoTimeCut.mNchanC);
          hNchC_cuts_vs_noCuts_vrtTrg->Fill(eventTimeAmpCut.mNchanC,eventTimeAmpNoCut.mNchanC);

        }
/*
        for(const auto &mcp: mapMCP2data) {
          if(mcp.second.size()>1) {
            for(const auto &channelData : mcp.second) {
              const auto &amp = channelData.QTCAmpl;
              const auto &time = channelData.CFDTime;
              const auto &chID = channelData.ChId;
              const auto &pmBits = channelData.ChainQTC;
              const unsigned int mcpID = chID/4;
              //arrAmpVsSumAmpMCP[chID]->Fill(amp,arrAmpSumPerMCP[mcpID]-amp);
              hArrLowAmpVsTimeSingleMCP[chID]->Fill(amp,time);
            }
          }
        }
*/
        prevOrbit = orbit;
        prevBC = bc;
        prevEventCollBC = isCollision;
        prevSumAmpA = eventTimeInADCgate.mAmpSumA;
        prevSumAmpC = eventTimeInADCgate.mAmpSumC;
      }
    }
    delete treeInput;
    fileInput.Close();
  }
  //Orbit stats
  std::cout<<"\nOrbits: ("<<orbitMin<<", "<<orbitMax<<")\n";
  //Writing data
  outputManager.storeOutput();
  std::cout<<std::endl;
}
void runAnalysisFull(const std::vector<std::string> &vecPathToSrc, const std::string &pathToDst,std::size_t nParallelJobs,std::size_t nChunksPerRun,const std::set<unsigned int> &setRunnum) {
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  /////////////////////////////////////////////////////
  const std::string filenamePrefix = "hist";
  /////////////////////////////////////////////////////
  utilities::InputFileManager inputFileManager;
  for(const auto & pathToSrc:vecPathToSrc) {
    inputFileManager.addPathSrc(pathToSrc);
  }
  const auto vecParams = inputFileManager.getFileChunks(nChunksPerRun,pathToDst,filenamePrefix);

  const auto &setRunnumsToProcces = setRunnum.size()>0 ? setRunnum : inputFileManager.mSetRunnums;
  const auto mapRun2EntryCCDB = utilities::ccdb::EntryCCDB::getMapRun2EntryCCDB(setRunnumsToProcces);
  ROOT::TProcessExecutor pool(nParallelJobs);
  const auto result = pool.Map([&mapRun2EntryCCDB,&setRunnumsToProcces](const utilities::InputFileManager::Parameters &entry) {
            if(setRunnumsToProcces.size()>0) {
              if(setRunnumsToProcces.find(entry.mRunnum)==setRunnumsToProcces.end()) {
                return 0;
              }
            }
            std::vector<std::string> vecFilepathInput{};
            for(const auto &entryFilepath: entry.mFilepathInputFIT) {
              if(Detector::sDetFIT_ID==detectorFIT::EDetectorFIT::kFT0) {
                vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFT0);
              }
              else if(Detector::sDetFIT_ID==detectorFIT::EDetectorFIT::kFV0) {
                vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFV0);
              }
              else if(Detector::sDetFIT_ID==detectorFIT::EDetectorFIT::kFDD) {
                vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFDD);
              }
            }
            const auto itEntryCCDB = mapRun2EntryCCDB.find(entry.mRunnum);
            if(itEntryCCDB != mapRun2EntryCCDB.end()) {
              processDigits(entry.mRunnum,vecFilepathInput,entry.mFilepathOutput,itEntryCCDB->second);
            }
            else {
              std::cout<<"ERROR! CANNOT FIND GRPLHCIFData for run "<<entry.mRunnum<<std::endl;
            }
            return 0;
          }
          , vecParams);
}
