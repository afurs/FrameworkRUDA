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
#include "O2_RUDA/SyncDigits.h"

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
using DetectorFDD = detectorFIT::DetectorFDD;
using DetectorFT0 = detectorFIT::DetectorFT0;
using DetectorFV0 = detectorFIT::DetectorFV0;

//Detector parameters
using DigitFDD = DetectorFDD::Digit_t;
using ChannelDataFDD = DetectorFDD::ChannelData_t;
const int sNchannelsA_FDD = DetectorFDD::sNchannelsA;
const int sNchannelsC_FDD = DetectorFDD::sNchannelsC;
const int sNchannelsAC_FDD = DetectorFDD::sNchannelsAC;
using EventTimeAmpFDD = digits::EventChDataParamsFIT<DetectorFDD>;

using DigitFT0 = DetectorFT0::Digit_t;
using ChannelDataFT0 = DetectorFT0::ChannelData_t;
const int sNchannelsA_FT0 = DetectorFT0::sNchannelsA;
const int sNchannelsC_FT0 = DetectorFT0::sNchannelsC;
const int sNchannelsAC_FT0 = DetectorFT0::sNchannelsAC;
using EventTimeAmpFT0 = digits::EventChDataParamsFIT<DetectorFT0>;

using DigitFV0 = DetectorFV0::Digit_t;
using ChannelDataFV0 = DetectorFV0::ChannelData_t;
const int sNchannelsA_FV0 = DetectorFV0::sNchannelsA;
const int sNchannelsC_FV0 = DetectorFV0::sNchannelsC;
const int sNchannelsAC_FV0 = DetectorFV0::sNchannelsAC;
using EventTimeAmpFV0 = digits::EventChDataParamsFIT<DetectorFV0>;

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using LUT = o2::fit::LookupTableBase<>;
using Axis = helpers::hists::Axis;

using EntryCCDB = utilities::ccdb::EntryCCDB;
using InputDataSample = std::vector<std::tuple<std::string,std::string,std::string> >;

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
void processDigits(unsigned int runnum, const InputDataSample &vecTupleFilepathInput, const std::string &filepathOutput,const EntryCCDB &entryCCDB)
{
  //Load libraries and define types
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Constants
  typedef digits::SyncParam<uint32_t> SyncParam_t;
  const std::string treeName="o2sim";
  //Trigger constants
  uint8_t validBits = 0b10011111;
  uint8_t bitsIsDataValid = 0b10000000;

  const std::map<unsigned int, std::string> mMapTrgBitNamesFDD = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Central"},
    {o2::fit::Triggers::bitSCen+1, "Semicentral"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"}};

  const std::map<unsigned int, std::string> mMapTrgBitNamesFT0 = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Central"},
    {o2::fit::Triggers::bitSCen+1, "Semicentral"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"}};

  const std::map<unsigned int, std::string> mMapTrgBitNamesFV0 = {
    {o2::fit::Triggers::bitA+1, "OrA" },
    {o2::fit::Triggers::bitAOut+1, "OrAOut" },
    {o2::fit::Triggers::bitTrgNchan+1, "TrgNChan" },
    {o2::fit::Triggers::bitTrgCharge+1, "TrgCharge" },
    {o2::fit::Triggers::bitAIn+1, "OrAIn" }};

  auto makeMapTrgValues2Names = [] (const std::map<unsigned int, std::string> &mapTrgNames) {
    std::map<unsigned int, std::string> mapResult{};
    const int nBits = mapTrgNames.size();
    for(int trgValue = 0;trgValue<std::pow(2,nBits);trgValue++) {
      std::string binName{""};
      for(const auto& entry : mapTrgNames) {
        const int bitPos = entry.first - 1;
        if(bitPos>0) {
          binName += std::string{" + "};
        }
        if((trgValue & (1<<bitPos))>0) {
          binName += entry.second;
        }
        else {
          binName += std::string{"!"} + entry.second;
        }
      }
      mapResult.insert({trgValue+1,binName});
    }
    return mapResult;
  };
  const std::map<std::string, std::map<unsigned int, std::string> > mMapDet2MapTrgNames = {
  {"FDD", mMapTrgBitNamesFDD},
  {"FT0", mMapTrgBitNamesFT0},
  {"FV0", mMapTrgBitNamesFV0}};
  const unsigned int mAnyTrgBitFDD = mMapTrgBitNamesFDD.size();
  const unsigned int mAnyTrgBitFT0 = mMapTrgBitNamesFT0.size();
  const unsigned int mAnyTrgBitFV0 = mMapTrgBitNamesFV0.size();

  std::map<unsigned int, std::string> mMapAllTrgBitNamesFDD = mMapTrgBitNamesFDD;
  mMapAllTrgBitNamesFDD.insert({mAnyTrgBitFDD+1,"AnyTrg"});
  std::map<unsigned int, std::string> mMapAllTrgBitNamesFT0 = mMapTrgBitNamesFT0;
  mMapAllTrgBitNamesFT0.insert({mAnyTrgBitFT0+1,"AnyTrg"});
  std::map<unsigned int, std::string> mMapAllTrgBitNamesFV0 = mMapTrgBitNamesFV0;
  mMapAllTrgBitNamesFV0.insert({mAnyTrgBitFV0+1,"AnyTrg"});

//  enum ETriggers {kOrA,kOrC,kCent,kSemiCent,kVertex,kAllEvents};


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
  const std::map<unsigned int,std::string> mMapBeamMask{{EBeamMask::kEmpty+1,"Empty"},
                                                          {EBeamMask::kBeam+1,"BeamBeam"},
                                                          {EBeamMask::kBeamA+1,"BeamA"},
                                                          {EBeamMask::kBeamC+1,"BeamC"},
                                                          {EBeamMask::kAny+1,"AnyBeam"}};

  const std::map<unsigned int,std::string> mMapBeamMaskBasic{{EBeamMask::kEmpty+1,"Empty"},
                                                          {EBeamMask::kBeam+1,"BeamBeam"},
                                                          {EBeamMask::kBeamA+1,"BeamA"},
                                                          {EBeamMask::kBeamC+1,"BeamC"}};

  o2::parameters::GRPLHCIFData objGRPLHCIFData = entryCCDB.mGRPLHCIFData;
//  if(ptrGRPLHCIFData!=nullptr) {
    const auto &bunchFilling = objGRPLHCIFData.getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  //}
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
  //Functor hashed beamMask-TrgBit
  
  const int nTrgBitsFDD = mMapAllTrgBitNamesFDD.size(); // 5 trg bits + all event(no trg cut) bit
  auto getBeamMaskAndTrgFDD = [&nTrgBitsFDD] (int beamMask, int trgBitPos) {
    return nTrgBitsFDD * beamMask + trgBitPos;
  };
  const int nTrgBitsFT0 = mMapAllTrgBitNamesFT0.size(); // 5 trg bits + all event(no trg cut) bit
  auto getBeamMaskAndTrgFT0 = [&nTrgBitsFT0] (int beamMask, int trgBitPos) {
    return nTrgBitsFT0 * beamMask + trgBitPos;
  };
  const int nTrgBitsFV0 = mMapAllTrgBitNamesFV0.size(); // 5 trg bits + all event(no trg cut) bit
  auto getBeamMaskAndTrgFV0 = [&nTrgBitsFV0] (int beamMask, int trgBitPos) {
    return nTrgBitsFV0 * beamMask + trgBitPos;
  };
/*
  for(int iMask=0;iMask<5;iMask++) {
  for(int iTrg=0;iTrg<6;iTrg++) {
     std::cout<<std::endl<<iMask<<"|"<<iTrg<<"|"<<getBeamMaskAndTrgFT0(iMask,iTrg)<<std::endl;
    }
  }
  return;
*/
  //Functor for preparing bean +trg combinations
  auto makePairMapCombinations = [] (const std::map<unsigned int,std::string> &mapFirst/*beam*/, const std::map<unsigned int,std::string> &mapSecond/*trg*/,const std::string &delimiter = "_",const std::string &prefixFirst="",const std::string &prefixSecond="") {
    std::map<unsigned int,std::string> mapResult{};
    const int nElementsSecondMap = mapSecond.size();
    for(auto const &entryFirst : mapFirst) {
      for(const auto &entrySecond: mapSecond) {
        const auto id = (entryFirst.first - 1 ) * nElementsSecondMap + (entrySecond.first - 1);
        const auto iBin = id + 1;
        const std::string binName = prefixFirst + entryFirst.second + delimiter + prefixSecond + entrySecond.second;
        mapResult.insert({iBin,binName});
      }
    }
    return mapResult;
  };
  //Map all beam + trg
  const std::map<unsigned int,std::string> mMapTrgAndBeamMaskFDD = makePairMapCombinations(mMapBeamMask,mMapAllTrgBitNamesFDD);
  const std::map<unsigned int,std::string> mMapTrgAndBeamMaskFT0 = makePairMapCombinations(mMapBeamMask,mMapAllTrgBitNamesFT0);
  const std::map<unsigned int,std::string> mMapTrgAndBeamMaskFV0 = makePairMapCombinations(mMapBeamMask,mMapAllTrgBitNamesFV0);
  const std::map<unsigned int,std::string> mMapTrgAndBeamMask_FDD_FT0 = makePairMapCombinations(mMapTrgAndBeamMaskFDD,mMapTrgAndBeamMaskFT0,"_","FDD_","FT0_");
  const std::map<unsigned int,std::string> mMapTrgAndBeamMask_FDD_FV0 = makePairMapCombinations(mMapTrgAndBeamMaskFDD,mMapTrgAndBeamMaskFV0,"_","FDD_","FV0_");
  const std::map<unsigned int,std::string> mMapTrgAndBeamMask_FT0_FV0 = makePairMapCombinations(mMapTrgAndBeamMaskFT0,mMapTrgAndBeamMaskFV0,"_","FT0_","FV0_");
  const int nTrgBitBeamMask = mMapTrgAndBeamMaskFT0.size(); // 5 trg bits + all event(no trg cut) bit
  auto getHashedTrgBitBeamMask = [&nTrgBitBeamMask] (int first, int second) {
    return first * nTrgBitBeamMask + second;
  };

  //Functors for hist per side creation
  auto makeNameTitle = [](const std::vector<int> &vecIDs,const std::string &name,const std::string &title) {
    std::vector< std::pair <std::string, std::string> > vecNameTitle{};
    for(const auto& num: vecIDs) {
      vecNameTitle.emplace_back(std::string{Form(name.c_str(),num)},std::string{Form(title.c_str(),num)});
    }
    return vecNameTitle;
  };

  //Output manager
  utilities::OutputHistManager outputManager(utilities::OutputManager::getFilepathPrefix(filepathOutput)+std::string{"_"});
  //Axes
  const Axis axisBC(sNBC,0,sNBC,"BC");
  const Axis axisBCdistance(sNBC,0,sNBC,"BC distance");
  const Axis axisBeamMaskBasic("Beam mask",mMapBeamMaskBasic);
//  const Axis axisTimeseriesSec(lastSec-firstSec,firstSec,lastSec,"Time since SOR [seconds]");

  const Axis axisBeam("Beam mask",mMapBeamMask);

  const Axis axisTriggersFDD("Triggers FDD",mMapTrgBitNamesFDD);
  const Axis axisBeamAndTriggersFDD("Beam mask and triggers, FDD",mMapTrgAndBeamMaskFDD);
  const Axis axisBeamAndTriggers_FDD_FT0("Beam mask and triggers, FDD+FT0",mMapTrgAndBeamMask_FDD_FT0);

  const Axis axisTriggersFT0("Triggers FT0",mMapTrgBitNamesFT0);
  const Axis axisBeamAndTriggersFT0("Beam mask and triggers, FT0",mMapTrgAndBeamMaskFT0);
  const Axis axisBeamAndTriggers_FDD_FV0("Beam mask and triggers, FDD+FV0",mMapTrgAndBeamMask_FDD_FV0);

  const Axis axisTriggersFV0("Triggers FV0",mMapTrgBitNamesFV0);
  const Axis axisBeamAndTriggersFV0("Beam mask and triggers, FV0",mMapTrgAndBeamMaskFV0);
  const Axis axisBeamAndTriggers_FT0_FV0("Beam mask and triggers, FT0+FV0",mMapTrgAndBeamMask_FT0_FV0);

  //Trigger list
  outputManager.registerCurrentList("Triggers");
  auto hTF = outputManager.registerHist<TH1F>("hTF","Number of processed TFs;TF", 1, 0,1);
  auto hSchemaBC = outputManager.registerHist<TH2F>("hSchemaBC","BC schema;BC; Beam mask", axisBC, axisBeamMaskBasic);

  auto hTrgAndBeamMaskFDD_perBC = outputManager.registerHist<TH2F>("hTrgAndBeamMaskFDD_perBC","Beam mask and trigger bits per BC, FDD",axisBC,axisBeamAndTriggersFDD);
  auto hTrgAndBeamMaskFT0_perBC = outputManager.registerHist<TH2F>("hTrgAndBeamMaskFT0_perBC","Beam mask and trigger bits per BC, FT0",axisBC,axisBeamAndTriggersFT0);
  auto hTrgAndBeamMaskFV0_perBC = outputManager.registerHist<TH2F>("hTrgAndBeamMaskFV0_perBC","Beam mask and trigger bits per BC, FV0",axisBC,axisBeamAndTriggersFV0);

  auto hTrgAndBeamMask_FDD_FT0 = outputManager.registerHist<TH2F>("hTrgAndBeamMask_FDD_FT0","Beam mask and trigger bits , FDD vs FT0",axisBeamAndTriggersFDD,axisBeamAndTriggersFT0);
  auto hTrgAndBeamMask_FDD_FV0 = outputManager.registerHist<TH2F>("hTrgAndBeamMask_FDD_FV0","Beam mask and trigger bits , FDD vs FV0",axisBeamAndTriggersFDD,axisBeamAndTriggersFV0);
  auto hTrgAndBeamMask_FT0_FV0 = outputManager.registerHist<TH2F>("hTrgAndBeamMask_FT0_FV0","Beam mask and trigger bits , FT0 vs FV0",axisBeamAndTriggersFT0,axisBeamAndTriggersFV0);

  auto hTrgAndBeamMaskPerBC_FDD_FT0 = outputManager.registerHist<TH2F>("hTrgAndBeamMaskPerBC_FDD_FT0","Beam mask and trigger bits per BC, FDD and FT0",axisBC,axisBeamAndTriggers_FDD_FT0);
  auto hTrgAndBeamMaskPerBC_FDD_FV0 = outputManager.registerHist<TH2F>("hTrgAndBeamMaskPerBC_FDD_FV0","Beam mask and trigger bits per BC, FDD and FV0",axisBC,axisBeamAndTriggers_FDD_FV0);
  auto hTrgAndBeamMaskPerBC_FT0_FV0 = outputManager.registerHist<TH2F>("hTrgAndBeamMaskPerBC_FT0_FV0","Beam mask and trigger bits per BC, FT0 and FV0",axisBC,axisBeamAndTriggers_FT0_FV0);

  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  //Filling BC mask schema
  for(int iBC =0 ;iBC<arrBeamMask.size();iBC++) {
    hSchemaBC->Fill(iBC,arrBeamMask[iBC]);
  }

  uint32_t prevOrbit{};
  uint32_t orbitMin{0xffffffff},orbitMax{0};
  uint16_t prevBC{};
  bool prevEventCollBC{false};
  ////
  std::vector<DigitFDD> vecDigitsFDD;
  std::vector<ChannelDataFDD> vecChannelDataFDD;

  std::vector<DigitFT0> vecDigitsFT0;
  std::vector<ChannelDataFT0> vecChannelDataFT0;

  std::vector<DigitFV0> vecDigitsFV0;
  std::vector<ChannelDataFV0> vecChannelDataFV0;

  DigitFDD dummyDigitFDD{};
  DigitFT0 dummyDigitFT0{};
  DigitFV0 dummyDigitFV0{};

  std::size_t mCntEvents{};
  std::size_t mNTFs{};
  for(const auto &tupleInputFilepath : vecTupleFilepathInput) {
    const std::string &inputFilepathFDD = std::get<0>(tupleInputFilepath);
    const std::string &inputFilepathFT0 = std::get<1>(tupleInputFilepath);
    const std::string &inputFilepathFV0 = std::get<2>(tupleInputFilepath);

    digits::ManagerSyncFIT managerSyncFIT(inputFilepathFDD,inputFilepathFT0,inputFilepathFV0);
    managerSyncFIT.setVars(vecDigitsFDD,vecChannelDataFDD,vecDigitsFT0,vecChannelDataFT0,vecDigitsFV0,vecChannelDataFV0);
    std::size_t nTotalTFs = managerSyncFIT.getEntries();

    std::size_t nPercents = 10;
    std::size_t stepTF = nPercents*nTotalTFs/100; //step for 10%
    if(stepTF==0) stepTF=nTotalTFs;
    const std::string messageMetics = "\nProcessing files: \n"+inputFilepathFDD+std::string{"\n"}+inputFilepathFT0+std::string{"\n"}+inputFilepathFV0
    +std::string{"\nTotal number of TFs: "}+std::to_string(nTotalTFs)+std::string{"\n"};
    //std::cout<<"\nProcessing files: \n"<<inputFilepathFDD<<std::endl<<inputFilepathFT0<<std::endl<<inputFilepathFV0<<std::endl;
    std::cout<<messageMetics;
    //std::cout<<"\nTotal number of TFs: "<<nTotalTFs<<std::endl;

    //Accumulating events into vector
    for (int iEvent = 0; iEvent < nTotalTFs; iEvent++) {
      //Iterating TFs in tree
      hTF->Fill(0);
      managerSyncFIT.getEntry(iEvent);
      mCntEvents++;
      mNTFs++;
      if(mNTFs%stepTF==0) std::cout<<nPercents*mNTFs/stepTF<<"% processed"<<std::endl;

      const auto syncMap = SyncParam_t::makeSyncMap(vecDigitsFDD,vecDigitsFT0,vecDigitsFV0);
      for(const auto &entry: syncMap) {
        const auto &ir = entry.first;
        const auto &bc = ir.bc;
        const auto &orbit = ir.orbit;
        const double secSinceSOR = 1.*(orbit*sNBC+bc)/sNBCperSec;

        const auto isFDD = entry.second.isFDD();
        const auto isFT0 = entry.second.isFT0();
        const auto isFV0 = entry.second.isFV0();

        const auto &digitFDD = isFDD ? vecDigitsFDD[entry.second.getIndexFDD()] : dummyDigitFDD;
        const auto &digitFT0 = isFT0 ? vecDigitsFT0[entry.second.getIndexFT0()] : dummyDigitFT0;
        const auto &digitFV0 = isFV0 ? vecDigitsFV0[entry.second.getIndexFV0()] : dummyDigitFV0;

        const auto &trgFDD = digitFDD.mTriggers;
        const auto &trgFT0 = digitFT0.mTriggers;
        const auto &trgFV0 = digitFV0.mTriggers;
        /*
        const auto &trgBitsFDD = trgFDD.getTriggersignals();
        const auto &trgBitsFT0 = trgFT0.getTriggersignals();
        const auto &trgBitsFV0 = trgFV0.getTriggersignals();
        */
        const uint8_t trgBitsFDD = trgFDD.getTriggersignals() & validBits;
        const uint8_t trgBitsFT0 = trgFT0.getTriggersignals() & validBits;
        const uint8_t trgBitsFV0 = trgFV0.getTriggersignals() & validBits;

/*
        const bool isDataValid_FDD = (bitsIsDataValid & trgBitsFDD)>0;
        const bool isDataValid_FT0 = (bitsIsDataValid & trgBitsFT0)>0;
        const bool isDataValid_FV0 = (bitsIsDataValid & trgBitsFV0)>0;
*/
        const auto trgBitsFDD_FT0 = trgBitsFDD & trgBitsFT0;
        const auto trgBitsFDD_FV0 = trgBitsFDD & trgBitsFV0;
        const auto trgBitsFT0_FV0 = trgBitsFT0 & trgBitsFV0;
        const auto trgBitsFIT = trgBitsFDD_FT0 & trgBitsFDD_FV0;

        const auto& channelsFDD = digitFDD.getBunchChannelData(vecChannelDataFDD);
        const auto& channelsFT0 = digitFT0.getBunchChannelData(vecChannelDataFT0);
        const auto& channelsFV0 = digitFV0.getBunchChannelData(vecChannelDataFV0);

        const bool isCollision = collBC.test(bc);
        const bool isMaskA = collBC_A.test(bc);
        const bool isMaskC = collBC_C.test(bc);
        const bool isMaskE = !(isCollision || isMaskA || isMaskC);
        const auto beamMask = arrBeamMask[bc];
        const bool isNewOrbit = (prevOrbit!=orbit);
        orbitMin=std::min(orbitMin,orbit);
        orbitMax=std::max(orbitMax,orbit);
        if(isNewOrbit) {
          prevBC=0xffff;
        }
        else {

        }
/******************************
        **PUT HERE CODE FOR PROCESSING DIGITS
******************************/
/*
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
*/
        std::vector<uint8_t> vecTrgBitsFDD{},vecTrgBitsFT0{},vecTrgBitsFV0{};

        std::vector<uint8_t> vecTrgBitsBeamMaskFDD{},vecTrgBitsBeamMaskFT0{},vecTrgBitsBeamMaskFV0{};

        const auto trgBitBeamMaskAnyTrg = getBeamMaskAndTrgFT0(beamMask,mAnyTrgBitFT0);
        const auto trgBitBeamMaskAnyBeamAnyTrg = getBeamMaskAndTrgFT0(EBeamMask::kAny,mAnyTrgBitFT0);


        for(int iTrg=0; iTrg < 5; iTrg++ ) {
          const auto trgBitBeamMask = getBeamMaskAndTrgFT0(beamMask,iTrg);
          const auto trgBitBeamMaskAnyBeam = getBeamMaskAndTrgFT0(EBeamMask::kAny,iTrg);
          if(trgBitsFDD & (1<<iTrg)) {
            vecTrgBitsFDD.push_back(iTrg);
            vecTrgBitsBeamMaskFDD.push_back(trgBitBeamMask);
            vecTrgBitsBeamMaskFDD.push_back(trgBitBeamMaskAnyBeam);
          }
          if(trgBitsFT0 & (1<<iTrg)) {
            vecTrgBitsFT0.push_back(iTrg);
            vecTrgBitsBeamMaskFT0.push_back(trgBitBeamMask);
            vecTrgBitsBeamMaskFT0.push_back(trgBitBeamMaskAnyBeam);
          }
          if(trgBitsFV0 & (1<<iTrg)) {
            vecTrgBitsFV0.push_back(iTrg);
            vecTrgBitsBeamMaskFV0.push_back(trgBitBeamMask);
            vecTrgBitsBeamMaskFV0.push_back(trgBitBeamMaskAnyBeam);
          }
        }
        if(isFDD) {
          vecTrgBitsBeamMaskFDD.push_back(trgBitBeamMaskAnyTrg);
          vecTrgBitsBeamMaskFDD.push_back(trgBitBeamMaskAnyBeamAnyTrg);
        }
        if(isFT0) {
          vecTrgBitsBeamMaskFT0.push_back(trgBitBeamMaskAnyTrg);
          vecTrgBitsBeamMaskFT0.push_back(trgBitBeamMaskAnyBeamAnyTrg);
        }
        if(isFV0) {
          vecTrgBitsBeamMaskFV0.push_back(trgBitBeamMaskAnyTrg);
          vecTrgBitsBeamMaskFV0.push_back(trgBitBeamMaskAnyBeamAnyTrg);
        }

        for(const auto &trgBeamMaskFDD : vecTrgBitsBeamMaskFDD) {
          hTrgAndBeamMaskFDD_perBC->Fill(bc,trgBeamMaskFDD);
          for(const auto &trgBeamMaskFT0 : vecTrgBitsBeamMaskFT0) {
            hTrgAndBeamMask_FDD_FT0->Fill(trgBeamMaskFDD,trgBeamMaskFT0);
            const auto hashValue = getHashedTrgBitBeamMask(trgBeamMaskFDD,trgBeamMaskFT0);
            hTrgAndBeamMaskPerBC_FDD_FT0->Fill(bc,hashValue);
          }
          for(const auto &trgBeamMaskFV0 : vecTrgBitsBeamMaskFV0) {
            hTrgAndBeamMask_FDD_FV0->Fill(trgBeamMaskFDD,trgBeamMaskFV0);
            const auto hashValue = getHashedTrgBitBeamMask(trgBeamMaskFDD,trgBeamMaskFV0);
            hTrgAndBeamMaskPerBC_FDD_FV0->Fill(bc,hashValue);
          }
        }

        for(const auto &trgBeamMaskFT0 : vecTrgBitsBeamMaskFT0) {
          hTrgAndBeamMaskFT0_perBC->Fill(bc,trgBeamMaskFT0);
          for(const auto &trgBeamMaskFV0 : vecTrgBitsBeamMaskFV0) {
            hTrgAndBeamMask_FT0_FV0->Fill(trgBeamMaskFT0,trgBeamMaskFV0);
            const auto hashValue = getHashedTrgBitBeamMask(trgBeamMaskFT0,trgBeamMaskFV0);
            hTrgAndBeamMaskPerBC_FT0_FV0->Fill(bc,hashValue);
          }
        }
        for(const auto &trgBeamMaskFV0 : vecTrgBitsBeamMaskFV0) {
          hTrgAndBeamMaskFV0_perBC->Fill(bc,trgBeamMaskFV0);
        }

        prevOrbit = orbit;
        prevBC = bc;
      }// Synchronised events
    }// TF
  }// tuple of input files
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
  const auto mapRun2EntryCCDB = utilities::ccdb::EntryCCDB::getMapRun2EntryCCDB(setRunnumsToProcces,true,false,true);
  ROOT::TProcessExecutor pool(nParallelJobs);
  const auto result = pool.Map([&mapRun2EntryCCDB,&setRunnumsToProcces](const utilities::InputFileManager::Parameters &entry) {
            if(setRunnumsToProcces.size()>0) {
              if(setRunnumsToProcces.find(entry.mRunnum)==setRunnumsToProcces.end()) {
                return 0;
              }
            }
            InputDataSample vecTupleFilepathInput{};
            for(const auto &entryFilepath: entry.mFilepathInputFIT) {
              vecTupleFilepathInput.emplace_back(std::make_tuple(entryFilepath.mFilepathInputFDD,entryFilepath.mFilepathInputFT0,entryFilepath.mFilepathInputFV0));
            }
            const auto itEntryCCDB = mapRun2EntryCCDB.find(entry.mRunnum);
            if(itEntryCCDB != mapRun2EntryCCDB.end()) {
              processDigits(entry.mRunnum,vecTupleFilepathInput,entry.mFilepathOutput,itEntryCCDB->second);
            }
            else {
              std::cout<<"ERROR! CANNOT FIND GRPLHCIFData for run "<<entry.mRunnum<<std::endl;
            }
            return 0;
          }
          , vecParams);
}
