#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/LookUpTable.h"
#include "DataFormatsParameters/GRPLHCIFData.h"
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

#include <bitset>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <regex>
#include <utility>
#include <algorithm>
#include <numeric>
/*
HINTS:

Trigger getters
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/common/include/DataFormatsFIT/Triggers.h#L59-L71

PM bits
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/FT0/include/DataFormatsFT0/ChannelData.h#L37-L44

*/
using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using LUT = o2::fit::LookupTableBase<>;
using DigitFT0 = o2::ft0::Digit;
using ChannelDataFT0 = o2::ft0::ChannelData;
using Axis = helpers::hists::Axis;

const float sNSperTimeChannel=o2::ft0::Geometry::ChannelWidth*1e-3; // nanoseconds per time channel, 0.01302
const float sNS2Cm = 29.97; // light NS to Centimetres
const int sNBC = 3564; // Number of BCs per Orbit
const int sOrbitPerTF=128;
const int sNBCperTF = sNBC*sOrbitPerTF;
const int sTFrate = 88;
const double sTFLengthSec = 1./sTFrate;
const int sNBCperSec = sNBC*sOrbitPerTF*sTFrate;

const int sNchannelsA = 96;
const int sNchannelsC = 112;
const int sNchannelsAC = sNchannelsA+sNchannelsC;

const int sOrGate = 153; // in TDC units

void processDigits(unsigned int runnum, const std::vector<std::string> &vecFilepathInput,const std::string &filepathOutput,const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData);
void runAnalysisFull(const std::string &pathToSrc, const std::string &pathToDst="hists") {
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  /////////////////////////////////////////////////////
//  const std::size_t nParallelJobs=8;
//  const std::size_t nChunksPerRun=15;
  const std::size_t nParallelJobs=8;
  const std::size_t nChunksPerRun=1;

  const std::string filenamePrefix = "hist";
  /////////////////////////////////////////////////////
  utilities::InputFileManager inputFileManager;

  inputFileManager.addPathSrc(pathToSrc);
  const auto vecParams = inputFileManager.getFileChunks(nChunksPerRun,pathToDst,filenamePrefix);
  const auto mapRun2GRPLHCIFData = utilities::ccdb::getMapRun2GRPLHCIFData(inputFileManager.mSetRunnums);
  ROOT::TProcessExecutor pool(nParallelJobs);
  const auto result = pool.Map([&mapRun2GRPLHCIFData](const utilities::InputFileManager::Parameters &entry) {
            std::vector<std::string> vecFilepathInput{};
            for(const auto &entryFilepath: entry.mFilepathInputFIT) {
              vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFT0);
            }
            const auto itGRPLHCIFData = mapRun2GRPLHCIFData.find(entry.mRunnum);
            if(itGRPLHCIFData!=mapRun2GRPLHCIFData.end()) {
              processDigits(entry.mRunnum,vecFilepathInput,entry.mFilepathOutput,itGRPLHCIFData->second);
            }
            else {
              std::cout<<"ERROR! CANNOT FIND GRPLHCIFData for run "<<entry.mRunnum<<std::endl;
            }
            return 0;
          }
          , vecParams);
}

void processDigits(unsigned int runnum, const std::vector<std::string> &vecFilepathInput, const std::string &filepathOutput,const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData)
{
  //Load libraries and define types
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Constants
  const std::string treeName="o2sim";
  const std::map<unsigned int, std::string> mMapTrgNames = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Cen"},
    {o2::fit::Triggers::bitSCen+1, "SCen"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"},
    {o2::fit::Triggers::bitLaser+1, "Laser"},
    {o2::fit::Triggers::bitOutputsAreBlocked+1,"OutputAreBlocked"},
    {o2::fit::Triggers::bitDataIsValid+1, "DataIsValid"}};
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
  const uint64_t firstOrbit = 2677122;
  const uint64_t lastOrbit = 10316543;
  auto orbit2Sec = [](uint64_t orbit) {
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
  if(ptrGRPLHCIFData!=nullptr) {
    const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }
  std::array<int,sNBC> prevCollisionBCperBC{};
  {
    int prevBC=-1;
    for(int iBC=0;iBC<sNBC;iBC++) {
      if(collBC.test(iBC) ){
        prevBC=iBC;
        prevCollisionBCperBC[iBC] = -1;
      }
      else {
        prevCollisionBCperBC[iBC] = prevBC;
      }
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
  const Axis axisChannels(sNchannelsAC,0,sNchannelsAC,"ChannelID");
  const Axis axisBC(sNBC,0,sNBC,"BC");
  const Axis axisTriggers(mMapTrgNames.size(),0,mMapTrgNames.size(),"Triggers",mMapTrgNames);
  const Axis axisVertex(400,-200.,200.);
  const Axis axisTimeseriesSec(lastSec-firstSec,firstSec,lastSec,"Time since SOR [seconds]");


  const Axis axisTimeFull(4000,-2000.,2000.);
  const Axis axisAmplitudeFull(4100,0.,4100.);

  const Axis axisTimeLowBins(200,-2000.,2000.);
  const Axis axisAmplitudeLowBins(205,0.,4100.);

  const Axis axisTimeLowBins4Corr(1000,-2000.,2000.);
  const Axis axisLowAmplitudes(100,0.,100.);

  const Axis axisTimeRange500(1000,-500.,500.);
  const Axis axisTimeRange200(400,-200.,200.);

  const Axis axisAmplitudeBins2(2050,0.,4100.);

  const Axis axisAmplitudeBins4(1025,0.,4100.);
  const Axis axisSumAmplBins10(1200,0.,12000.);


  //Amplitude vs Time
  auto arrLowAmpVsTime = make2DHistsPerSide("LowAmpVsTime_side%s","hLowAmpVsTime%i","Low amplitude vs time, channelID %i (collision BC, vertex, good PM bits);Amp [ADC];Time [TDC]",axisLowAmplitudes,axisTimeRange500);
//  auto arrAmpVsTime = make2DHistsPerSide("AmpVsTime_side%s","hAmpVsTime%i","Amplitude vs time, channelID %i;Amp [ADC];Time [TDC]",axisAmplitudeLowBins,axisTimeLowBins);
  auto arrAmpVsTime = make2DHistsPerSide("AmpVsTime_side%s","hAmpVsTime%i","Amplitude vs time, channelID %i (collisionBC + vertex trigger + good PM bits);Amp [ADC];Time [TDC]",axisAmplitudeBins2,axisTimeRange500);
//*  auto arrAmpVsSumAmpMCP = make2DHistsPerSide("AmpVsSumAmpMCP_side%s","hAmpVsSumAmpMCP%i","Amplitude vs sum amp within MCP(except given chID), channelID %i (collisionBC + vertex trigger + good PM bits);Amp [ADC];SumAmp [ADC]",axisAmplitudeBins4,axisSumAmplBins10);
  auto arrTimeTs = make2DHistsPerSide("TimeTs_side%s","hTimeTs%i","Time spectra timeseries, channelID %i (collisionBC + vertex trigger + good PM bits);Time [TDC]",axisTimeseriesSec,axisTimeRange200);
//  auto arrTimeVsVertex = make2DHistsPerSide("TimeVsVertex_side%s","hTimeVsVertex%i","Vertex vs time, channelID %i;Vertex[cm];Time [TDC]",axisVertex,axisTimeRange500);

  //Spectra list
//  outputManager.registerCurrentList("Spectra");
//  auto hAmpSingleMCP = outputManager.registerHist<TH2F>("hAmpSingleMCP","Amplitude(only single channel within MCP + collision + vertex trigger + good PM bits + abs(time)<153 TDC);ChannelID;Amplitude [ADC]",axisChannels,axisAmplitudeFull);
//  auto hTimeSingleMCP = outputManager.registerHist<TH2F>("hTimeSingleMCP","Time(only single channel within MCP);ChannelID;Time [TDC]",axisChannels,axisTimeFull);
  //Trigger list
  outputManager.registerCurrentList("Triggers");
//  auto hTriggerReflection = outputManager.registerHist<TH1F>("hTriggerReflection","Triggers at reflectiobn BC;Trigger",axisTriggers);
  auto hTriggerBC = outputManager.registerHist<TH2F>("hTriggersBC","Triggers Vs BC;BC", axisBC, axisTriggers);
  auto hTriggerPerAfterCollBC = outputManager.registerHist<TH2F>("hTriggerPerAfterCollBC","Triggers Vs BC, after collision BC(previous event - collision, isCFDinGate PM bit);BC", axisBC, axisTriggers);
  auto hChIDperBC_OrA = outputManager.registerHist<TH2F>("hChIDperBC_OrA","ChannelID(kIsEventInTVDC PM bit) vs BC, OrA", axisBC, axisChannels);
  auto hChIDperBC_OrC = outputManager.registerHist<TH2F>("hChIDperBC_OrC","ChannelID(kIsEventInTVDC PM bit) vs BC, OrC", axisBC, axisChannels);
  auto hChIDperBC_Vertex = outputManager.registerHist<TH2F>("hChIDperBC_Vertex","ChannelID(kIsEventInTVDC PM bit) vs BC, Vertex", axisBC, axisChannels);

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
  ////MCP arrays
  std::array<int, sNchannelsAC/4> arrChannelsPerMCP{};
  std::array<int, sNchannelsAC/4> arrAmpSumPerMCP{};

  ////
  std::vector<DigitFT0> vecDigits;
  std::vector<DigitFT0> *ptrVecDigits = &vecDigits;
  std::vector<ChannelDataFT0> vecChannelData;
  std::vector<ChannelDataFT0> *ptrVecChannelData = &vecChannelData;
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
    treeInput->SetBranchAddress("FT0DIGITSBC", &ptrVecDigits);
    treeInput->SetBranchAddress("FT0DIGITSCH", &ptrVecChannelData);
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
        const auto &ir = digit.mIntRecord;
        const auto &bc = ir.bc;
        const auto &orbit = ir.orbit;
        const auto &trg = digit.mTriggers;
        const auto &trgBits = digit.mTriggers.getTriggersignals();
        const auto &nChA = digit.mTriggers.getNChanA();
        const auto &nChC = digit.mTriggers.getNChanC();
        const auto &sumAmpA = digit.mTriggers.getAmplA();
        const auto &sumAmpC = digit.mTriggers.getAmplC();
        const auto &averageTimeA = digit.mTriggers.getTimeA();
        const auto &averageTimeC = digit.mTriggers.getTimeC();
/******************************
        **PUT HERE CODE FOR PROCESSING DIGITS
******************************/
        const double secSinceSOR = 1.*(orbit*sNBC+bc)/sNBCperSec;
        const bool isEventVertex = (trg.getVertex() && trg.getDataIsValid() && !trg.getOutputsAreBlocked());
        const bool isCollision = collBC.test(bc);
        const bool isMaskA = collBC_A.test(bc);
        const bool isMaskC = collBC_C.test(bc);
        const bool isMaskE = !(isCollision || isMaskA || isMaskC);
        const bool isNewOrbit = (prevOrbit!=orbit);
        const uint64_t sec = orbit2Sec(orbit);

        orbitMin=std::min(orbitMin,orbit);
        orbitMax=std::max(orbitMax,orbit);
        prevOrbit = orbit;
        if(isNewOrbit) {
          clearMapChID2BC_FT0();
          arrChID_lastBC.fill(0xffff);
          prevBC=0xffff;
        }
        std::vector<uint8_t> vecTrgActivated{};
        for(int i=0;i<8;i++)  {
          if(trgBits & (1<<i)) {
            vecTrgActivated.emplace_back(i);
            hTriggerBC->Fill(bc,static_cast<double>(i));
          }
        }
        // MCP
        std::array<int, sNchannelsAC/4> arrChannelsPerMCP{};
        std::array<int, sNchannelsAC/4> arrAmpSumPerMCP{};
        std::map<unsigned int, std::vector<ChannelDataFT0> > mapMCP2data{};
        //
        for(const auto &channelData: channels) {
          //Iterating over ChannelData(PM data) per given Event(Digit)
          //VARIABLEES TO USE
          const auto &amp = channelData.QTCAmpl;
          const auto &time = channelData.CFDTime;
          const auto &chID = channelData.ChId;
          const auto &pmBits = channelData.ChainQTC;
          if(chID>=sNchannelsAC) continue;
          const double timePs = time * 13.02;
          const unsigned int mcpID = chID/4;
          const bool isPMbitsGood = ((pmBits & pmBitsToCheck) == pmBitsGood);
          const bool isChannelUsedInTrg = pmBits & (1<<ChannelDataFT0::kIsEventInTVDC);
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
          if(isEventVertex && isCollision && isPMbitsGood) {
            auto pairRes = mapMCP2data.insert({mcpID,{}});
            pairRes.first->second.emplace_back(channelData);
            arrLowAmpVsTime[chID]->Fill(amp,time);
            arrAmpVsTime[chID]->Fill(amp,time);
            arrTimeTs[chID]->Fill(sec,time);
            arrChannelsPerMCP[mcpID]++;
            arrAmpSumPerMCP[mcpID]+=amp;
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
        /*
        for(const auto &mcp: mapMCP2data) {
          if(mcp.second.size()>1) {
            for(const auto &channelData : mcp.second) {
              const auto &amp = channelData.QTCAmpl;
              const auto &time = channelData.CFDTime;
              const auto &chID = channelData.ChId;
              const auto &pmBits = channelData.ChainQTC;
              const unsigned int mcpID = chID/4;
              arrAmpVsSumAmpMCP[chID]->Fill(amp,arrAmpSumPerMCP[mcpID]-amp);
            }
          }
        }
        */
        prevBC = bc;
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
