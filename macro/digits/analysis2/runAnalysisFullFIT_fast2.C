// Common O2
#include "CCDB/BasicCCDBManager.h"
#include "CommonUtils/NameConf.h"
#include "CommonConstants/LHCConstants.h"
#include "DataFormatsParameters/GRPLHCIFData.h"
//FDD
#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"
#include "FDDBase/Constants.h"
//FT0
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "FT0Base/Geometry.h"
//FV0
#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"
#include "FV0Base/Constants.h"

//ROOT
#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include "ROOT/TProcessExecutor.hxx"
//RUDA
#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
//Custom headers
#include "SyncDigits.h"
#include "EventChDataParams.h"
//STD
#include <vector>
#include <array>
#include <set>
#include <map>
#include <regex>
#include <utility>
#include <algorithm>

/*
HINTS:

Trigger getters
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/common/include/DataFormatsFIT/Triggers.h#L59-L71

PM bits
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/FT0/include/DataFormatsFT0/ChannelData.h#L37-L44

*/

//FDD
const float sNSperTimeChannelFDD=o2::fdd::timePerTDC; // nanoseconds per time channel, 0.01302
const int sNchannelsFDD = 19;
const int sNchannels_FDDA = 8;
const int sNchannels_FDDC = 8;
const int sNchannels_FDDAC = sNchannels_FDDA+sNchannels_FDDC;
const int sOrGateFDD = 153; // in TDC units

//FT0
const float sNSperTimeChannel=o2::ft0::Geometry::ChannelWidth*1e-3; // nanoseconds per time channel, 0.01302
const int sNchannelsFT0 = 212;
const int sNchannels_FT0A = 96;
const int sNchannels_FT0C = 112;
const int sNchannels_FT0AC = sNchannels_FT0A+sNchannels_FT0C;
const int sOrGateFT0 = 153; // in TDC units

//FV0
const float sNSperTimeChannelFV0=o2::ft0::Geometry::ChannelWidth*1e-3; // nanoseconds per time channel, 0.01302
const int sNchannelsFV0 = 49;
const int sNchannels_FV0A = 48;
const int sNchannels_FV0C = 0;
const int sNchannels_FV0AC = sNchannels_FV0A+sNchannels_FV0C;
const int sOrGateFV0 = 153; // in TDC units
// Global
const float sNS2Cm = 29.97; // light NS to Centimetres
const int sNBC = 3564; // Number of BCs per Orbit
const int sOrbitPerTF=128;
const int sNBCperTF = sNBC * sOrbitPerTF;
const double sTFrate = 1e6 / (sOrbitPerTF * o2::constants::lhc::LHCOrbitMUS);
const double sNBCperSec = sNBC*sOrbitPerTF*sTFrate;
const double sNsizeTS_seconds = 10000; // size of time series, in seconds
const double sNsizeTS_TF = sTFrate * sNsizeTS_seconds; // size of time series, in TF units
const std::size_t sNsizeTS_bins = 10000;

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;

using DigitFDD = o2::fdd::Digit;
using ChannelDataFDD = o2::fdd::ChannelData;
using TriggersFDD = o2::fdd::Triggers;

using DigitFT0 = o2::ft0::Digit;
using ChannelDataFT0 = o2::ft0::ChannelData;
using TriggersFT0 = o2::ft0::Triggers;

using DigitFV0 = o2::fv0::Digit;
using ChannelDataFV0 = o2::fv0::ChannelData;
using TriggersFV0 = o2::fv0::Triggers;

using EventChDataParamsFDD = digits::EventChDataParams<sNchannels_FDDA,sNchannels_FDDC>;
using EventChDataParamsFT0 = digits::EventChDataParams<sNchannels_FT0A,sNchannels_FT0C>;
using EventChDataParamsFV0 = digits::EventChDataParams<sNchannels_FV0A,0>;

void writeResult(TList *listOutput, const std::string &filepathOutput);
std::bitset<sNBC> getCollBC(unsigned int runnum);

void processDigits(unsigned int runnum,
                   const std::string &filepathInputFDD,
                   const std::string &filepathInputFT0,
                   const std::string &filepathInputFV0,
                   const std::string &filepathOutput = "hists.root",
                   const std::bitset<sNBC> &argCollBC = {},
                   bool uploadFromCCDB = true);

void runAnalysis(unsigned int runnum,
                 const std::string &pathToSrcFDD = "/data/work/run3/digits/production_all/d1",
                 const std::string &pathToSrcFT0 = "/data/work/run3/digits/production_all/d2",
                 const std::string &pathToSrcFV0 = "/data/work/run3/digits/production_all/d3",
                 const std::string &pathToDst="") {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  const auto mapRunToFilepathsFDD = Utils::makeMapRunsToFilepathsROOT(pathToSrcFDD);
  auto itFDD = mapRunToFilepathsFDD.find(runnum);

  const auto mapRunToFilepathsFT0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFT0);
  auto itFT0 = mapRunToFilepathsFT0.find(runnum);

  const auto mapRunToFilepathsFV0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFV0);
  auto itFV0 = mapRunToFilepathsFV0.find(runnum);

  const std::string outputPath = pathToDst+"hists"+std::to_string(runnum)+".root";

  std::string inputPathFDD{""};
  std::string inputPathFT0{""};
  std::string inputPathFV0{""};
  std::bitset<3> bitsetDet{};
  if(itFDD != mapRunToFilepathsFDD.end()) {
    inputPathFDD = itFDD->second[0];
    bitsetDet.set(0);
  }
  if(itFT0 != mapRunToFilepathsFT0.end()) {
    inputPathFT0 = itFT0->second[0];
    bitsetDet.set(1);
  }
  if(itFV0 != mapRunToFilepathsFV0.end()) {
    inputPathFV0 = itFV0->second[0];
    bitsetDet.set(2);
  }
  processDigits(runnum, inputPathFDD, inputPathFT0, inputPathFV0, outputPath);

/*
  if(bitsetDet.count()>1) {
    processDigits(runnum, inputPathFDD, inputPathFT0, inputPathFV0, outputPath);
  }
  else {
    std::cout<<"\nWARNING! CANNOT FIND FILE RELATED TO RUN "<<runnum<<" AT PATH "<<pathToSrc<<std::endl;
  }
*/
}

void runAnalysisFull(const std::string &pathToSrcFDD = "/data/work/run3/digits/prod_test/run529011",
                     const std::string &pathToSrcFT0 = "/data/work/run3/digits/prod_test/run529011",
                     const std::string &pathToSrcFV0 = "/data/work/run3/digits/prod_test/run529011",
                     const std::string &pathToDst="histsBC1") {



  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  const auto mapRunToFilepathsFDD = Utils::makeMapRunsToFilepathsROOT(pathToSrcFDD);
  const auto mapRunToFilepathsFT0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFT0);
  const auto mapRunToFilepathsFV0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFV0);

  const std::size_t nParallelJobs=10;
  ROOT::TProcessExecutor pool(nParallelJobs);
  struct Parameters {
    unsigned int runnum{};
    std::string filepathInputFDD{};
    std::string filepathInputFT0{};
    std::string filepathInputFV0{};
    std::string filepathOutput{};
    std::bitset<sNBC> collBC{};
    bool isReady{};
    void init() {
      if(!isReady){
        isReady=true;
        collBC=getCollBC(runnum);
      }
    }
  };

  std::vector<Parameters> vecParams{};
  std::map<unsigned int, Parameters> mapParameters{};
  std::map<unsigned int,std::bitset<sNBC>> mapRunnum2CollBC{};
  for(const auto &entry: mapRunToFilepathsFDD) {
    const auto &runnum = entry.first;
    auto vecFilepaths = entry.second;
    std::sort(vecFilepaths.begin(),vecFilepaths.end());
    for(const auto en: vecFilepaths) {
      std::cout<<std::endl<<en;
    }
    std::cout<<std::endl;
    unsigned int index{0};
    for(const auto &filepath : vecFilepaths) {
      if(filepath.find("_d1_") != string::npos) {

        const std::string outputFile = pathToDst+"/"+"hist"+std::to_string(runnum)+Form("_%.3u",index)+".root";
        unsigned int id = index;
        auto it = mapParameters.insert({id,{}});
        it.first->second.runnum = runnum;
        it.first->second.filepathInputFDD = filepath;
        it.first->second.filepathOutput = outputFile;
        if(mapRunnum2CollBC.find(runnum)==mapRunnum2CollBC.end()) {
          const auto collBC = getCollBC(runnum);
          mapRunnum2CollBC.insert({runnum,collBC});
          it.first->second.collBC = collBC;
        }
        else {
          it.first->second.collBC = mapRunnum2CollBC.find(runnum)->second;
        }
        index++;
      }
    }
  }

  for(const auto &entry: mapRunToFilepathsFT0) {
    const auto &runnum = entry.first;
    auto vecFilepaths = entry.second;
    std::sort(vecFilepaths.begin(),vecFilepaths.end());
    unsigned int index{0};
    for(const auto &filepath : vecFilepaths) {
      if(filepath.find("_d2_") != string::npos) {
        //const std::string outputFile = pathToDst+"/"+"hist"+std::to_string(runnum)+".root";
        unsigned int id = index;
        auto it = mapParameters.insert({id,{}});
        it.first->second.runnum = runnum;
        it.first->second.filepathInputFT0 = filepath;
        //it.first->second.filepathOutput = outputFile;
        //it.first->second.init();
        if(mapRunnum2CollBC.find(runnum)==mapRunnum2CollBC.end()) {
          const auto collBC = getCollBC(runnum);
          mapRunnum2CollBC.insert({runnum,collBC});
          it.first->second.collBC = collBC;
        }
        else {
          it.first->second.collBC = mapRunnum2CollBC.find(runnum)->second;
        }

        index++;
      }
    }
  }

  for(const auto &entry: mapRunToFilepathsFV0) {
    const auto &runnum = entry.first;
    auto vecFilepaths = entry.second;
    std::sort(vecFilepaths.begin(),vecFilepaths.end());
    unsigned int index{0};
    for(const auto &filepath : vecFilepaths) {
      if(filepath.find("_d3_") != string::npos) {
        //const std::string outputFile = pathToDst+"/"+"hist"+std::to_string(runnum)+".root";
        unsigned int id = index;
        auto it = mapParameters.insert({id,{}});
        it.first->second.runnum = runnum;
        it.first->second.filepathInputFV0 = filepath;
        //it.first->second.filepathOutput = outputFile;
        //it.first->second.init();
        if(mapRunnum2CollBC.find(runnum)==mapRunnum2CollBC.end()) {
          const auto collBC = getCollBC(runnum);
          mapRunnum2CollBC.insert({runnum,collBC});
          it.first->second.collBC = collBC;
        }
        else {
          it.first->second.collBC = mapRunnum2CollBC.find(runnum)->second;
        }

        index++;
      }
    }
  }

  for(const auto &entry: mapParameters) {
    vecParams.push_back(entry.second);
  }

  const auto result = pool.Map([](const Parameters &entry) {
          processDigits(entry.runnum,entry.filepathInputFDD,entry.filepathInputFT0,entry.filepathInputFV0,entry.filepathOutput,entry.collBC,false);
          return 0;}
          , vecParams);
}

void processDigits(unsigned int runnum,
                   const std::string &filepathInputFDD,
                   const std::string &filepathInputFT0,
                   const std::string &filepathInputFV0,
                   const std::string &filepathOutput,
                   const std::bitset<sNBC> &argCollBC,
                   bool uploadFromCCDB)
{
  uint64_t orbitShift{0};
  if(runnum==529011) orbitShift = 459000000;
  if(runnum==529014) orbitShift = 459000000+68259840;
  
  std::cout<<"\nProcessing..."
  <<"\nRunnum: "<<runnum
  <<"\nFDD filepath: "<<filepathInputFDD
  <<"\nFT0 filepath: "<<filepathInputFT0
  <<"\nFV0 filepath: "<<filepathInputFV0
  <<"\nOutput file: "<<filepathOutput
  <<std::endl;
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Filenames
  const std::string extFileoutput = ".root";
  std::string filename = filepathOutput;
  std::string filepathNoExt{};
  {
    const auto pos=filepathOutput.find_last_of('/');
    if(pos != std::string::npos) {
      filename=filepathOutput.substr(pos+1);
    }

    const auto posExt=filepathOutput.find_last_of(extFileoutput)-extFileoutput.size() + 1;
    if(posExt != std::string::npos) {
      filepathNoExt = filepathOutput.substr(0,posExt);
    }
    else {
      std::cout<<"Warning! Check output filepath! There are no extension: "<<extFileoutput<<std::endl;
      return;
    }
  }
  //  Typedefs
  typedef digits::SyncParam<uint32_t> SyncParam_t;
  //Constants
  const std::string treeName="o2sim";

  const std::map<unsigned int, std::string> mMapTrgNamesFDD = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Cen"},
    {o2::fit::Triggers::bitSCen+1, "SCen"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"},
    {o2::fit::Triggers::bitLaser+1, "Laser"},
    {o2::fit::Triggers::bitOutputsAreBlocked+1,"OutputAreBlocked"},
    {o2::fit::Triggers::bitDataIsValid+1, "DataIsValid"}};

  const std::map<unsigned int, std::string> mMapTrgNamesFT0 = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Cen"},
    {o2::fit::Triggers::bitSCen+1, "SCen"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"},
    {o2::fit::Triggers::bitLaser+1, "Laser"},
    {o2::fit::Triggers::bitOutputsAreBlocked+1,"OutputAreBlocked"},
    {o2::fit::Triggers::bitDataIsValid+1, "DataIsValid"}};

  const std::map<unsigned int, std::string> mMapTrgNamesFV0 = {
    {o2::fit::Triggers::bitA+1, "OrA" },
    {o2::fit::Triggers::bitAOut+1, "OrAOut" },
    {o2::fit::Triggers::bitTrgNchan+1, "TrgNChan" },
    {o2::fit::Triggers::bitTrgCharge+1, "TrgCharge" },
    {o2::fit::Triggers::bitAIn+1, "OrAIn" },
    {o2::fit::Triggers::bitLaser+1, "Laser"},
    {o2::fit::Triggers::bitOutputsAreBlocked+1,"OutputAreBlocked"},
    {o2::fit::Triggers::bitDataIsValid+1, "DataIsValid"}};

  /*
  std:string allowedTriggerBits = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Cen"},
    {o2::fit::Triggers::bitSCen+1, "SCen"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"},
    {o2::fit::Triggers::bitAOut+1, "OrAOut" },
    {o2::fit::Triggers::bitTrgNchan+1, "TrgNChan" },
    {o2::fit::Triggers::bitTrgCharge+1, "TrgCharge" },
    {o2::fit::Triggers::bitAIn+1, "OrAIn" }};

  auto hasherTrgFIT = [](uint8_t trgBitFDD, uint8_t trgBitFT0, uint8_t trgBitFV0) -> uint32_t {
    return (1 << trgBitFDD) | (1<<(trgBitFT0+9)) | (1<<(trgBitFV0+18));
  }
  
  {
    std::size_t index{0};
    for(const auto &enFDD: mMapTrgNamesFDD) {
      if(allowedTriggerBits.find(enFDD.second) != allowedTriggerBits.end()) {
        index++;
        std::string trgName=
      }
    }
    for(const auto &enFT0: mMapTrgNamesFT0) {
      if(allowedTriggerBits.find(enFDD.second) != allowedTriggerBits.end()) {
        index++;
      }
    }
    for(const auto &enFV0: mMapTrgNamesFV0) {
      if(allowedTriggerBits.find(enFDD.second) != allowedTriggerBits.end()) {
        index++;
      }
    }
    for(const auto &enFDD: mMapTrgNamesFDD) {
      for(const auto &enFT0: mMapTrgNamesFT0) {
        for(const auto &enFV0: mMapTrgNamesFV0) {
          index++;
        }
      }
    }
  }
  */

  const std::map<unsigned int, std::string> mMapPMbits = {
    {o2::ft0::ChannelData::kNumberADC+1, "NumberADC" },
    {o2::ft0::ChannelData::kIsDoubleEvent+1, "IsDoubleEvent" },
    {o2::ft0::ChannelData::kIsTimeInfoNOTvalid+1, "IsTimeInfoNOTvalid" },
    {o2::ft0::ChannelData::kIsCFDinADCgate+1, "IsCFDinADCgate" },
    {o2::ft0::ChannelData::kIsTimeInfoLate+1, "IsTimeInfoLate" },
    {o2::ft0::ChannelData::kIsAmpHigh+1, "IsAmpHigh" },
    {o2::ft0::ChannelData::kIsEventInTVDC+1, "IsEventInTVDC" },
    {o2::ft0::ChannelData::kIsTimeInfoLost+1, "IsTimeInfoLost" }};

/*
  auto invertTrgMap = [] (const auto &mapTrg) {
    std::map<typename decltype(mapTrg)::mapped_type, typename decltype(mapTrg)::key_type> resultMao{};
    
  }
  auto trgFIT_bitHasher = [](uint8_t bitFDD, uint8_t bitFT0, uint8_t bitFV0) {
    constexpr int 
  };

*/
/*
  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);
  */

  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);

  uint8_t pmBitsToCheck = pmBitsGood | pmBitsBad; //All except kNumberADC
  std::cout<<"\nGOOD PM BITS: "<<static_cast<int>(pmBitsGood);
  std::cout<<"\nBAD PM BITS: "<<static_cast<int>(pmBitsBad);
  std::cout<<"\nCHECK PM BITS: "<<static_cast<int>(pmBitsToCheck);


  std::bitset<sNBC> collBC_tmp=argCollBC;
  if(collBC_tmp.count()==0 && uploadFromCCDB) {
    collBC_tmp = getCollBC(runnum);
  }
  const auto collBC = collBC_tmp;

  std::map<std::string,TList *> mapOutput2Store{};
  //Garbage list
  TList *listGarbage = new TList();
  listGarbage->SetOwner(true);
  listGarbage->SetName("output");
  //Output lists
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  {
    const std::string filepathOutputBuf = filepathOutput;
    mapOutput2Store.insert({filepathOutputBuf, listOutput});
  }
  listGarbage->Add(listOutput);

  //Output lists FDD
  TList *listOutputFDD = new TList();
  listOutputFDD->SetOwner(true);
  listOutputFDD->SetName("output");
  {
    const std::string filepathOutputBuf =filepathNoExt+std::string{"_FDD.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputFDD});
  }
  listGarbage->Add(listOutputFDD);

  //Output lists FT0
  TList *listOutputFT0 = new TList();
  listOutputFT0->SetOwner(true);
  listOutputFT0->SetName("output");
  {
    const std::string filepathOutputBuf =filepathNoExt+std::string{"_FT0.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputFT0});
  }
  listGarbage->Add(listOutputFT0);

  //Output lists FV0
  TList *listOutputFV0 = new TList();
  listOutputFV0->SetOwner(true);
  listOutputFV0->SetName("output");
  {
    const std::string filepathOutputBuf =filepathNoExt+std::string{"_FV0.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputFV0});
  }
  listGarbage->Add(listOutputFV0);
  const double N_TS_MAX = sNsizeTS_TF;
  /*
  TH2F *hTrgFDD_TS = new TH2F("hTrgFDD_TS","Timeseries vs FDD triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFDD.size(),0,mMapTrgNamesFDD.size());
  HistHelper::makeHistBinNamed(hTrgFDD_TS,mMapTrgNamesFDD,1);
  listOutput->Add(hTrgFDD_TS);

  TH2F *hTrgFT0_TS = new TH2F("hTrgFT0_TS","Timeseries vs FT0 triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFT0.size(),0,mMapTrgNamesFT0.size());
  HistHelper::makeHistBinNamed(hTrgFT0_TS,mMapTrgNamesFT0,1);
  listOutput->Add(hTrgFT0_TS);

  TH2F *hTrgFV0_TS = new TH2F("hTrgFV0_TS","Timeseries vs FV0 triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFV0.size(),0,mMapTrgNamesFV0.size());
  HistHelper::makeHistBinNamed(hTrgFV0_TS,mMapTrgNamesFV0,1);
  listOutput->Add(hTrgFV0_TS);
  std::map<unsigned int, std::string> mMapTrgNamesFDD_FT0{};
  std::map<unsigned int, std::string> mMapTrgNamesFT0_FV0{};
  std::map<unsigned int, std::string> mMapTrgNamesFDD_FV0{};
  std::array<std::array<unsigned int, 8>, 8> arr2D2hashValue{};
  std::map<unsigned int, std::string> mMapTrgNamesFIT{};
  std::array<std::array<std::array<unsigned int, 8>, 8>, 8> arr3D2hashValue{};
  for(int iTrg=0;iTrg<8;iTrg++) {
    for(int iTrg2=0;iTrg2<8;iTrg2++) {
      const std::string binNameFDD_FT0 = mMapTrgNamesFDD.find(iTrg+1)->second + "FDD && "+ mMapTrgNamesFT0.find(iTrg2+1)->second + "FT0";
      const std::string binNameFT0_FV0 = mMapTrgNamesFT0.find(iTrg+1)->second + "FT0 && "+ mMapTrgNamesFV0.find(iTrg2+1)->second + "FV0";
      const std::string binNameFDD_FV0 = mMapTrgNamesFDD.find(iTrg+1)->second + "FDD && " + mMapTrgNamesFV0.find(iTrg2+1)->second + "FV0";
      const unsigned int binPos = 1 + iTrg + iTrg2 * 8;
      arr2D2hashValue[iTrg][iTrg2] = binPos - 1;
      mMapTrgNamesFDD_FT0.insert({binPos, binNameFDD_FT0});
      mMapTrgNamesFT0_FV0.insert({binPos, binNameFT0_FV0});
      mMapTrgNamesFDD_FV0.insert({binPos, binNameFDD_FV0});
      for(int iTrg3=0;iTrg3<8;iTrg3++) {
        const std::string binNameFIT = mMapTrgNamesFDD.find(iTrg+1)->second + "FDD && " + mMapTrgNamesFT0.find(iTrg2+1)->second + "FT0 && "+ mMapTrgNamesFV0.find(iTrg3+1)->second + "FV0";
        const unsigned int binPos = 1 + iTrg + iTrg2*8 + iTrg3 * 8 * 8;
        arr3D2hashValue[iTrg][iTrg2][iTrg3] = binPos - 1;
        mMapTrgNamesFIT.insert({binPos, binNameFIT});
      }
    }
  }
  TH2F *hTrgFDD_FT0_TS = new TH2F("hTrgFDD_FT0_TS","Timeseries vs FDD & FT0 triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFDD_FT0.size(),0,mMapTrgNamesFDD_FT0.size());
  HistHelper::makeHistBinNamed(hTrgFDD_FT0_TS,mMapTrgNamesFDD_FT0,1);
  listOutput->Add(hTrgFDD_FT0_TS);

  TH2F *hTrgFT0_FV0_TS = new TH2F("hTrgFT0_FV0_TS","Timeseries vs FT0 & FV0 triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFT0_FV0.size(),0,mMapTrgNamesFT0_FV0.size());
  HistHelper::makeHistBinNamed(hTrgFT0_FV0_TS,mMapTrgNamesFT0_FV0,1);
  listOutput->Add(hTrgFT0_FV0_TS);

  TH2F *hTrgFDD_FV0_TS = new TH2F("hTrgFDD_FV0_TS","Timeseries vs FDD & FV0 triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFDD_FV0.size(),0,mMapTrgNamesFDD_FV0.size());
  HistHelper::makeHistBinNamed(hTrgFDD_FV0_TS,mMapTrgNamesFDD_FV0,1);
  listOutput->Add(hTrgFDD_FV0_TS);

  TH2F *hTrgFIT_TS = new TH2F("hTrgFIT_TS","Timeseries vs FDD & FT0 & FV0 triggers;[TFID];Trigger",sNsizeTS_bins,0,N_TS_MAX,mMapTrgNamesFIT.size(),0,mMapTrgNamesFIT.size());
  HistHelper::makeHistBinNamed(hTrgFIT_TS,mMapTrgNamesFIT,1);
  listOutput->Add(hTrgFIT_TS);
  */

  /*
  //FT0
  TH2F *hAmpFT0A_TS = new TH2F("hAmpFT0A_TS","Timeseries vs amplitude FT0A;[TFID];Amp [ADC]",sNsizeTS_bins,0,N_TS_MAX,4200,-100,4100);
  listOutputFT0->Add(hAmpFT0A_TS);
  TH2F *hAmpFT0C_TS = new TH2F("hAmpFT0C_TS","Timeseries vs amplitude FT0C;[TFID];Amp [ADC]",sNsizeTS_bins,0,N_TS_MAX,4200,-100,4100);
  listOutputFT0->Add(hAmpFT0C_TS);

  TH2F *hTimeFT0A_TS = new TH2F("hTimeFT0A_TS","Timeseries vs time FT0A;[TFID];Time [TDC]",sNsizeTS_bins,0,N_TS_MAX,4000,-2000,2000);
  listOutputFT0->Add(hTimeFT0A_TS);
  TH2F *hTimeFT0C_TS = new TH2F("hTimeFT0C_TS","Timeseries vs time FT0C;[TFID];Time [TDC]",sNsizeTS_bins,0,N_TS_MAX,4000,-2000,2000);
  listOutputFT0->Add(hTimeFT0C_TS);

  TH2F *hChIDsFT0_inGate_TS = new TH2F("hChIDsFT0_inGate_TS","Timeseries vs channel IDs FT0 (in gate +- 153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FT0AC,0,sNchannels_FT0AC);
  listOutputFT0->Add(hChIDsFT0_inGate_TS);

  TH2F *hChIDsFT0_belowGate_TS = new TH2F("hChIDsFT0_belowGate_TS","Timeseries vs channel IDs FT0 (below gate < -153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FT0AC,0,sNchannels_FT0AC);
  listOutputFT0->Add(hChIDsFT0_belowGate_TS);

  TH2F *hChIDsFT0_underGate_TS = new TH2F("hChIDsFT0_underGate_TS","Timeseries vs channel IDs FT0 (under gate > 153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FT0AC,0,sNchannels_FT0AC);
  listOutputFT0->Add(hChIDsFT0_underGate_TS);
  //FV0
  TH2F *hAmpFV0A_TS = new TH2F("hAmpFV0A_TS","Timeseries vs amplitude FV0A;[TFID];Amp [ADC]",sNsizeTS_bins,0,N_TS_MAX,4200,-100,4100);
  listOutputFV0->Add(hAmpFV0A_TS);

  TH2F *hTimeFV0A_TS = new TH2F("hTimeFV0A_TS","Timeseries vs time FV0A;[TFID];Time [TDC]",sNsizeTS_bins,0,N_TS_MAX,4000,-2000,2000);
  listOutputFV0->Add(hTimeFV0A_TS);

  TH2F *hChIDsFV0_inGate_TS = new TH2F("hChIDsFV0_inGate_TS","Timeseries vs channel IDs FV0 (in gate +- 153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FV0AC,0,sNchannels_FV0AC);
  listOutputFV0->Add(hChIDsFV0_inGate_TS);

  TH2F *hChIDsFV0_belowGate_TS = new TH2F("hChIDsFV0_belowGate_TS","Timeseries vs channel IDs FV0 (below gate < -153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FV0AC,0,sNchannels_FV0AC);
  listOutputFV0->Add(hChIDsFV0_belowGate_TS);

  TH2F *hChIDsFV0_underGate_TS = new TH2F("hChIDsFV0_underGate_TS","Timeseries vs channel IDs FV0 (under gate > 153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FV0AC,0,sNchannels_FV0AC);
  listOutputFV0->Add(hChIDsFV0_underGate_TS);
  //FDD
  TH2F *hAmpFDDA_TS = new TH2F("hAmpFDDA_TS","Timeseries vs amplitude FDDA;[TFID];Amp [ADC]",sNsizeTS_bins,0,N_TS_MAX,4200,-100,4100);
  listOutputFDD->Add(hAmpFDDA_TS);
  TH2F *hAmpFDDC_TS = new TH2F("hAmpFDDC_TS","Timeseries vs amplitude FDDC;[TFID];Amp [ADC]",sNsizeTS_bins,0,N_TS_MAX,4200,-100,4100);
  listOutputFDD->Add(hAmpFDDC_TS);

  TH2F *hTimeFDDA_TS = new TH2F("hTimeFDDA_TS","Timeseries vs time FDDA;[TFID];Time [TDC]",sNsizeTS_bins,0,N_TS_MAX,4000,-2000,2000);
  listOutputFDD->Add(hTimeFDDA_TS);
  TH2F *hTimeFDDC_TS = new TH2F("hTimeFDDC_TS","Timeseries vs time FDDC;[TFID];Time [TDC]",sNsizeTS_bins,0,N_TS_MAX,4000,-2000,2000);
  listOutputFDD->Add(hTimeFDDC_TS);

  TH2F *hChIDsFDD_inGate_TS = new TH2F("hChIDsFDD_inGate_TS","Timeseries vs channel IDs FDD (in gate +- 153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FDDAC,0,sNchannels_FDDAC);
  listOutputFDD->Add(hChIDsFDD_inGate_TS);

  TH2F *hChIDsFDD_belowGate_TS = new TH2F("hChIDsFDD_belowGate_TS","Timeseries vs channel IDs FDD (below gate < -153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FDDAC,0,sNchannels_FDDAC);
  listOutputFDD->Add(hChIDsFDD_belowGate_TS);

  TH2F *hChIDsFDD_underGate_TS = new TH2F("hChIDsFDD_underGate_TS","Timeseries vs channel IDs FDD (under gate > 153 TDC);[TFID];ChannelID",sNsizeTS_bins,0,N_TS_MAX,sNchannels_FDDAC,0,sNchannels_FDDAC);
  listOutputFDD->Add(hChIDsFDD_underGate_TS);
  */
  std::set<std::string> allowedTriggerBitNames = { "OrA"
    , "OrC"
    , "Cen"
    , "SCen"
    , "Vertex"
    , "OrAOut"
    , "TrgNChan"
    , "TrgCharge"
    , "OrAIn"
    , "DataIsValid" };
  const uint8_t allowedTrgBits = 0b10011111;
  using ArrHistTrg2BC_t = std::array<TH2F *, 8>;
  ArrHistTrg2BC_t arrHistTrgFDD_BC_TS{};
  ArrHistTrg2BC_t arrHistTrgFT0_BC_TS{};
  ArrHistTrg2BC_t arrHistTrgFV0_BC_TS{};
  auto initHistBC_TS = [&] (const std::map<unsigned int, std::string> &mapTrg, ArrHistTrg2BC_t &arrHists,TList *listHists,const std::string &detName) {
    for(const auto &en: mapTrg) {
      const auto &binTrg = en.first;
      const auto &trgBitName = en.second;
      if(allowedTriggerBitNames.find(trgBitName)!=allowedTriggerBitNames.end()) {
        const std::string histName = "hBC_TS_"+detName+"_"+trgBitName;
        const std::string histTitle = "Timeseries BC, "+detName+" trigger "+trgBitName+";[TFID];BC";
        TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),sNsizeTS_bins,0,N_TS_MAX,sNBC,0,sNBC);
        listHists->Add(hist);
        arrHists[binTrg-1] = hist;
      }
    }
  };
  initHistBC_TS(mMapTrgNamesFDD, arrHistTrgFDD_BC_TS, listOutputFDD, "FDD");
  initHistBC_TS(mMapTrgNamesFT0, arrHistTrgFT0_BC_TS, listOutputFT0, "FT0");
  initHistBC_TS(mMapTrgNamesFV0, arrHistTrgFV0_BC_TS, listOutputFV0, "FV0");

  // FDD
  TH2F *hCollisionTimeVsVertexFDD_noCut = new TH2F("hCollisionTimeVsVertexFDD_noCut","Collision time vs Vertex position, FDD(w/o cuts);Vertex [cm];Collision time [ns]",4000,-200,200,1000,-50,50);
  listOutputFDD->Add(hCollisionTimeVsVertexFDD_noCut);
  TH2F *hCollisionTimeVsVertexFDD = new TH2F("hCollisionTimeVsVertexFDD","Collision time vs Vertex position, FDD(all channels in CFD gate);Vertex [cm];Collision time [ns]",2000,-100,100,1000,-50,50);
  listOutputFDD->Add(hCollisionTimeVsVertexFDD);
  
  
  // FT0
  TH2F *hCollisionTimeVsVertexFT0_noCut = new TH2F("hCollisionTimeVsVertexFT0_noCut","Collision time vs Vertex position, FT0(w/o cuts);Vertex [cm];Collision time [ns]",2000,-100,100,1000,-50,50);
  listOutputFT0->Add(hCollisionTimeVsVertexFT0_noCut);
  TH2F *hCollisionTimeVsVertexFT0 = new TH2F("hCollisionTimeVsVertexFT0","Collision time vs Vertex position, FT0(all channels in CFD gate);Vertex [cm];Collision time [ns]",2000,-100,100,1000,-50,50);
  listOutputFT0->Add(hCollisionTimeVsVertexFT0);
  // FV0
  TH2F *hMeanTimeVsSumAmpFV0_noCut = new TH2F("hMeanTimeVsSumAmpFV0_noCut","MeanTime vs SumAmp, FV0, (w/o cuts);MeanTime [ns];SumAmp",1000,-20,80,4000,0,100000);
  listOutputFV0->Add(hMeanTimeVsSumAmpFV0_noCut);
  TH2F *hMeanTimeVsSumAmpFV0 = new TH2F("hMeanTimeVsSumAmpFV0","MeanTime vs SumAmp, FV0, (w/o cuts);MeanTime [ns];SumAmp",2000,-100,100,4000,0,100000);
  listOutputFV0->Add(hMeanTimeVsSumAmpFV0);

  //FDD
  using DigitFDD = o2::fdd::Digit;
  using ChannelDataFDD = o2::fdd::ChannelData;
  std::vector<DigitFDD> vecDigitsFDD;
  std::vector<DigitFDD> *ptrVecDigitsFDD = &vecDigitsFDD;
  std::vector<ChannelDataFDD> vecChannelDataFDD;
  std::vector<ChannelDataFDD> *ptrVecChannelDataFDD = &vecChannelDataFDD;

  //FT0
  using DigitFT0 = o2::ft0::Digit;
  using ChannelDataFT0 = o2::ft0::ChannelData;
  std::vector<DigitFT0> vecDigitsFT0;
  std::vector<DigitFT0> *ptrVecDigitsFT0 = &vecDigitsFT0;
  std::vector<ChannelDataFT0> vecChannelDataFT0;
  std::vector<ChannelDataFT0> *ptrVecChannelDataFT0 = &vecChannelDataFT0;

  //FV0
  using DigitFV0 = o2::fv0::Digit;
  using ChannelDataFV0 = o2::fv0::ChannelData;
  std::vector<DigitFV0> vecDigitsFV0;
  std::vector<DigitFV0> *ptrVecDigitsFV0 = &vecDigitsFV0;
  std::vector<ChannelDataFV0> vecChannelDataFV0;
  std::vector<ChannelDataFV0> *ptrVecChannelDataFV0 = &vecChannelDataFV0;

  std::size_t mCntEvents{};
  std::size_t mNTFs{};
  std::vector<std::string> vecFilenames{};

  {
    std::cout<<"\nProcessing file: "<<filepathInputFT0<<std::endl;
    //TFile *fileInputFDD = TFile::Open(filepathInputFDD.c_str());
    //TFile *fileInputFT0 = TFile::Open(filepathInputFT0.c_str());
    //TFile *fileInputFV0 = TFile::Open(filepathInputFV0.c_str());

    TFile fileInputFDD(filepathInputFDD.c_str(),"READ");
    TFile fileInputFT0(filepathInputFT0.c_str(),"READ");
    TFile fileInputFV0(filepathInputFV0.c_str(),"READ");

    TTree* treeInputFDD = nullptr;
    TTree* treeInputFT0 = nullptr;
    TTree* treeInputFV0 = nullptr;

    if(!fileInputFDD.IsOpen()) {
      std::cout<<"\nThere are no digit file for FDD!\n"<<std::endl;
    }
    else {
      treeInputFDD = dynamic_cast<TTree*>(fileInputFDD.Get(treeName.c_str()));
    }

    if(!fileInputFT0.IsOpen()) {
      std::cout<<"\nThere are no digit file for FT0!\n"<<std::endl;
    }
    else {
      treeInputFT0 = dynamic_cast<TTree*>(fileInputFT0.Get(treeName.c_str()));
    }

    if(!fileInputFV0.IsOpen()) {
      std::cout<<"\nThere are no digit file for FV0!\n"<<std::endl;
    }
    else {
      treeInputFV0 = dynamic_cast<TTree*>(fileInputFV0.Get(treeName.c_str()));
    }

    if(treeInputFDD!=nullptr) {
      treeInputFDD->SetBranchAddress("FDDDigit", &ptrVecDigitsFDD);
      treeInputFDD->SetBranchAddress("FDDDigitCh", &ptrVecChannelDataFDD);
    }

    if(treeInputFT0!=nullptr) {
      treeInputFT0->SetBranchAddress("FT0DIGITSBC", &ptrVecDigitsFT0);
      treeInputFT0->SetBranchAddress("FT0DIGITSCH", &ptrVecChannelDataFT0);
    }

    if(treeInputFV0!=nullptr) {
      treeInputFV0->SetBranchAddress("FV0DigitBC", &ptrVecDigitsFV0);
      treeInputFV0->SetBranchAddress("FV0DigitCh", &ptrVecChannelDataFV0);
    }

    std::size_t nTotalTFs = treeInputFT0->GetEntries();
    std::size_t nPercents = 10;
    std::size_t stepTF = nPercents*nTotalTFs/100; //step for 10%
    if(stepTF==0) stepTF=nTotalTFs;
    std::cout<<"\nTotal number of TFs: "<<nTotalTFs<<std::endl;

    DigitFDD dummyDigitFDD{};
    DigitFT0 dummyDigitFT0{};
    DigitFV0 dummyDigitFV0{};

    //Accumulating events into vector
    for (int iEntry = 0; iEntry < nTotalTFs; iEntry++) {
      //Iterating TFs in tree
      if(treeInputFDD!=nullptr) {
        treeInputFDD->GetEntry(iEntry);
      }
      if(treeInputFT0!=nullptr) {
        treeInputFT0->GetEntry(iEntry);
      }
      if(treeInputFV0!=nullptr) {
        treeInputFV0->GetEntry(iEntry);
      }
      mCntEvents++;
      mNTFs++;
      if(mNTFs%stepTF==0) std::cout<<nPercents*mNTFs/stepTF<<"% processed"<<std::endl;
/*
      uint32_t orbitOld{};
      std::array<uint16_t, sNchannelsAC> arrChID_lastBC{};
      arrChID_lastBC.fill(0xffff);
      std::array<std::bitset<sNBC>, sNchannelsAC> arrChID_BC{};
*/
      const auto syncMap = SyncParam_t::makeSyncMap(vecDigitsFDD,vecDigitsFT0,vecDigitsFV0);

      for(const auto &entry: syncMap) {
        const auto &ir = entry.first;
        const auto &bc = ir.bc;
        const auto &orbit = ir.orbit;
        const double secSinceSOR = (orbit * o2::constants::lhc::LHCOrbitMUS + bc * o2::constants::lhc::LHCBunchSpacingMUS) * 1e-6;
        const std::size_t TFID = (orbit-orbitShift/*-459000000*//*-68259840*/) / sOrbitPerTF;
        //std::cout<<std::endl<<TFID<<"|"<<orbit;
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

        const uint8_t trgBitsFDD = allowedTrgBits & trgFDD.getTriggersignals();
        const uint8_t trgBitsFT0 = allowedTrgBits & trgFT0.getTriggersignals();
        const uint8_t trgBitsFV0 = allowedTrgBits & trgFV0.getTriggersignals();


        const auto trgBitsFDD_FT0 = trgBitsFDD & trgBitsFT0;
        const auto trgBitsFDD_FV0 = trgBitsFDD & trgBitsFV0;
        const auto trgBitsFT0_FV0 = trgBitsFT0 & trgBitsFV0;
        const auto trgBitsFIT = trgBitsFDD_FT0 & trgBitsFDD_FV0;

        const auto& channelsFDD = digitFDD.getBunchChannelData(vecChannelDataFDD);
        const auto& channelsFT0 = digitFT0.getBunchChannelData(vecChannelDataFT0);
        const auto& channelsFV0 = digitFV0.getBunchChannelData(vecChannelDataFV0);

        const bool isCollision = collBC.test(bc);

        EventChDataParamsFDD chDataParamsFDD_noCut{};
        EventChDataParamsFDD chDataParamsFDD{};
        EventChDataParamsFT0 chDataParamsFT0_noCut{};
        EventChDataParamsFT0 chDataParamsFT0{};
        EventChDataParamsFV0 chDataParamsFV0_noCut{};
        EventChDataParamsFV0 chDataParamsFV0{};

        // ChannelData processing
        //FDD

        for(const auto &channelDataFDD: channelsFDD) {
          const auto &amp = channelDataFDD.mChargeADC;
          const auto &time = channelDataFDD.mTime;
          const auto &chID = channelDataFDD.mPMNumber;
          const auto &pmBits = channelDataFDD.mFEEBits;


/*
          if(std::abs(time)<=sOrGateFDD) {
            hChIDsFDD_inGate_TS->Fill(TFID,chID);
          }
          else if(time>sOrGateFDD) {
            hChIDsFDD_underGate_TS->Fill(TFID,chID);
          }
          else {
            hChIDsFDD_belowGate_TS->Fill(TFID,chID);
          }
          if(chID < sNchannels_FDDA) {
            hAmpFDDA_TS->Fill(TFID,amp);
            hTimeFDDA_TS->Fill(TFID,time);
          }
          else if(chID < sNchannels_FDDAC) {
            hAmpFDDC_TS->Fill(TFID,amp);
            hTimeFDDC_TS->Fill(TFID,time);
          }
*/

          chDataParamsFDD_noCut.fill(amp,time,chID);
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
            chDataParamsFDD.fill(amp,time,chID);
          }

        }
        
        //FT0

        for(const auto &channelDataFT0: channelsFT0) {
          const auto &amp = channelDataFT0.QTCAmpl;
          const auto &time = channelDataFT0.CFDTime;
          const auto &chID = channelDataFT0.ChId;
          const auto &pmBits = channelDataFT0.ChainQTC;

/*
          if(std::abs(time)<=sOrGateFT0) {
            hChIDsFT0_inGate_TS->Fill(TFID,chID);
          }
          else if(time>sOrGateFT0) {
            hChIDsFT0_underGate_TS->Fill(TFID,chID);
          }
          else {
            hChIDsFT0_belowGate_TS->Fill(TFID,chID);
          }
          if(chID < sNchannels_FT0A) {
            hAmpFT0A_TS->Fill(TFID,amp);
            hTimeFT0A_TS->Fill(TFID,time);
          }
          else if(chID < sNchannels_FT0AC) {
            hAmpFT0C_TS->Fill(TFID,amp);
            hTimeFT0C_TS->Fill(TFID,time);
          }
*/

          chDataParamsFT0_noCut.fill(amp,time,chID);
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
            chDataParamsFT0.fill(amp,time,chID);
          }
        }

        //FV0

        for(const auto &channelDataFV0: channelsFV0) {
          const auto &amp = channelDataFV0.QTCAmpl;
          const auto &time = channelDataFV0.CFDTime;
          const auto &chID = channelDataFV0.ChId;
          const auto &pmBits = channelDataFV0.ChainQTC;

/*
          if(std::abs(time)<=sOrGateFV0) {
            hChIDsFV0_inGate_TS->Fill(TFID,chID);
          }
          else if(time>sOrGateFV0) {
            hChIDsFV0_underGate_TS->Fill(TFID,chID);
          }
          else {
            hChIDsFV0_belowGate_TS->Fill(TFID,chID);
          }
          if(chID < sNchannels_FV0A) {
            hAmpFV0A_TS->Fill(TFID,amp);
            hTimeFV0A_TS->Fill(TFID,time);
          }
*/

          chDataParamsFV0_noCut.fill(amp,time,chID);
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
            chDataParamsFV0.fill(amp,time,chID);
          }
        }

        chDataParamsFDD_noCut.calculate();
        chDataParamsFDD.calculate();
        chDataParamsFT0_noCut.calculate();
        chDataParamsFT0.calculate();
        chDataParamsFV0_noCut.calculate();
        chDataParamsFV0.calculate();

        //Trigger
        //FDD
        std::vector<uint8_t> vecTrgBitsFDD{};
        std::vector<uint8_t> vecTrgBitsFT0{};
        std::vector<uint8_t> vecTrgBitsFV0{};

        for(int iTrg=0; iTrg < 8; iTrg++ ) {
          const uint8_t trgVal = (1<<iTrg);
          const bool isTrgFDD = bool(trgBitsFDD & trgVal);
          const bool isTrgFT0 = bool(trgBitsFT0 & trgVal);
          const bool isTrgFV0 = bool(trgBitsFV0 & trgVal);
          if(isTrgFDD) {
            vecTrgBitsFDD.push_back(iTrg);
            //hTrgFDD_TS->Fill(TFID,iTrg);
            arrHistTrgFDD_BC_TS[iTrg]->Fill(TFID,bc);
          }
          if(isTrgFT0) {
            vecTrgBitsFT0.push_back(iTrg);
            //hTrgFT0_TS->Fill(TFID,iTrg);
            arrHistTrgFT0_BC_TS[iTrg]->Fill(TFID,bc);
          }
          if(isTrgFV0) {
            vecTrgBitsFV0.push_back(iTrg);
            //hTrgFV0_TS->Fill(TFID,iTrg);
            arrHistTrgFV0_BC_TS[iTrg]->Fill(TFID,bc);
          }
        }

/*
        for(const auto & bitFT0: vecTrgBitsFT0) {
          for(const auto & bitFV0: vecTrgBitsFV0) {
            const unsigned int pos = arr2D2hashValue[bitFT0][bitFV0];
            hTrgFT0_FV0_TS->Fill(TFID,pos);
          }
        }


        for(const auto & bitFDD: vecTrgBitsFDD) {
          for(const auto & bitFT0: vecTrgBitsFT0) {
            const unsigned int pos = arr2D2hashValue[bitFDD][bitFT0];
            hTrgFDD_FT0_TS->Fill(TFID,pos);
            for(const auto & bitFV0: vecTrgBitsFV0) {
              const unsigned int pos2 = arr3D2hashValue[bitFDD][bitFT0][bitFV0];
              hTrgFIT_TS->Fill(TFID,pos2);
            }
          }
          for(const auto & bitFV0: vecTrgBitsFV0) {
            const unsigned int pos = arr2D2hashValue[bitFDD][bitFV0];
            hTrgFDD_FV0_TS->Fill(TFID,pos);
          }
        }

*/
/*
        //Trigger
        //FDD
        std::vector<uint8_t> vecTrgBitsFDD{};
        for(int iTrg=0; iTrg < 8; iTrg++ ) {
          if(trgBitsFDD & (1<<iTrg)) {
            hTriggerFDD_BC->Fill(bc,iTrg);
            vecTrgBitsFDD.push_back(iTrg);
          }
        }
        //FT0
        std::vector<uint8_t> vecTrgBitsFT0{};
        for(int iTrg = 0; iTrg < 8; iTrg++ ) {
          if(trgBitsFT0 & (1<<iTrg)) {
            hTriggerFT0_BC->Fill(bc,iTrg);
            vecTrgBitsFT0.push_back(iTrg);
            const auto trgBitFT0{iTrg};
            for(const auto &trgBitFDD: vecTrgBitsFDD) {
              hTriggerFDD_FT0->Fill(trgBitFDD,trgBitFT0);
            }
          }
        }
        //FV0
        std::vector<uint8_t> vecTrgBitsFV0{};
        for(int iTrg = 0; iTrg < 8; iTrg++ ) {
          if(trgBitsFV0 & (1<<iTrg)) {
            hTriggerFV0_BC->Fill(bc,iTrg);
            vecTrgBitsFV0.push_back(iTrg);
            const auto trgBitFV0{iTrg};
            for(const auto &trgBitFDD: vecTrgBitsFDD) {
              hTriggerFDD_FV0->Fill(trgBitFDD,trgBitFV0);
            }
            for(const auto &trgBitFT0: vecTrgBitsFT0) {
              hTriggerFT0_FV0->Fill(trgBitFT0,trgBitFV0);
              for(const auto &trgBitFDD: vecTrgBitsFDD) {
                hArrTriggerFT0_FDD_FV0[trgBitFT0]->Fill(trgBitFDD,trgBitFV0);
              }
            }
          }
        }
*/
/*
        const bool isSpotFT0_1 = (chDataParamsFT0_noCut.mIsReadyAC)
                              && (chDataParamsFT0_noCut.mCollTime>-3.5)
                              && (chDataParamsFT0_noCut.mCollTime<-1.5)
                              && (chDataParamsFT0_noCut.mVrtPos>-90.)
                              && (chDataParamsFT0_noCut.mVrtPos<-70.);
*/
        if(chDataParamsFDD_noCut.mIsReadyAC) {
          hCollisionTimeVsVertexFDD_noCut->Fill(chDataParamsFDD_noCut.mVrtPos,chDataParamsFDD_noCut.mCollTime);
          if(chDataParamsFDD.mIsReadyAC) {
            hCollisionTimeVsVertexFDD->Fill(chDataParamsFDD.mVrtPos,chDataParamsFDD.mCollTime);
          }
        }
        if(chDataParamsFT0_noCut.mIsReadyAC)  {
          hCollisionTimeVsVertexFT0_noCut->Fill(chDataParamsFT0_noCut.mVrtPos,chDataParamsFT0_noCut.mCollTime);
          if(chDataParamsFT0.mIsReadyAC)  {
            hCollisionTimeVsVertexFT0->Fill(chDataParamsFT0.mVrtPos,chDataParamsFT0.mCollTime);
          }
        }
        
        
      }// entry SyncMap
    }// entry TTree
    if(fileInputFDD.IsOpen()) {
      if(treeInputFDD!=nullptr) {
        delete treeInputFDD;
      }
      fileInputFDD.Close();
    }
    if(fileInputFT0.IsOpen()) {
      if(treeInputFT0!=nullptr) {
        delete treeInputFT0;
      }
      fileInputFT0.Close();
    }
    if(fileInputFV0.IsOpen()) {
      if(treeInputFV0!=nullptr) {
        delete treeInputFV0;
      }
      fileInputFV0.Close();
    }
  }//
  // Postproc
  /*
  TH1F *hTrgEfficiency = dynamic_cast<TH1F*>(hTriggers->Clone("hTrgEfficiency"));
  hTrgEfficiency->SetTitle("Trigger efficiency, HardwareTrg/EmulatedTrg");
  hTrgEfficiency->Divide(hTriggersEmu);
  HistHelper::makeHistBinNamed(hTrgEfficiency,mMapTrgNames,0);
  listTriggers->Add(hTrgEfficiency);

  TH1F *hTrgRates = (TH1F *) hTriggers->Clone("hTrgRates");
  hTrgRates->SetTitle("Trigger rates");
  {
    TH1F histBuf("histBuf","histBuf",mMapTrgNames.size(),0,mMapTrgNames.size());
    HistHelper::makeHistBinNamed(&histBuf,mMapTrgNames,0);

    double timeLength = double(mNTFs) * sTFLengthSec;
    for(int iBin=1;iBin<mMapTrgNames.size()+1;iBin++) {
      histBuf.SetBinContent(iBin,timeLength);
    }
    hTrgRates->Divide(&histBuf);
  }
  HistHelper::makeHistBinNamed(hTrgRates,mMapTrgNames,0);
  listTriggers->Add(hTrgRates);
  */
  //Writing data
  for(const auto &entry: mapOutput2Store) {
    const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(entry.second,entry.first);
  }
  delete listGarbage;
  std::cout<<std::endl;
}

std::bitset<sNBC> getCollBC(unsigned int runnum) {
  std::bitset<sNBC> collBC{};
  const std::string tsKey = "SOR";
  o2::ccdb::CcdbApi ccdb_api;
  ccdb_api.init(o2::base::NameConf::getCCDBServer());
  const auto headers = ccdb_api.retrieveHeaders("RCT/Info/RunInformation", std::map<std::string, std::string>(),runnum);
  uint64_t ts{};
  const auto &itTimestamp = headers.find(tsKey);
  if(itTimestamp!=headers.end()) {
    ts = std::stoll(itTimestamp->second);
  }
  else {
    return collBC;
  }
  std::map<std::string, std::string> mapMetadata;
  std::map<std::string, std::string> mapHeader;
  const auto *ptrGRPLHCIFData = ccdb_api.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF",mapMetadata,ts,&mapHeader);
  const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
  //ptrGRPLHCIFData->print();
  collBC=bunchFilling.getBCPattern();
  return collBC;
}
