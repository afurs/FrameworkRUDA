#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"
//#include "DataFormatsFDD/LookUpTable.h"
#include "FDDBase/Constants.h"

#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/LookUpTable.h"
#include "FT0Base/Geometry.h"

#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"
//#include "DataFormatsFV0/LookUpTable.h"
#include "FV0Base/Constants.h"

#include "DataFormatsParameters/GRPLHCIFData.h"

#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>
#include "ROOT/TProcessExecutor.hxx"

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"

#include "SyncDigits.h"

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

const float sNS2Cm = 29.97; // light NS to Centimetres
const int sNBC = 3564; // Number of BCs per Orbit
const int sOrbitPerTF=128;
const int sNBCperTF = sNBC*sOrbitPerTF;
const int sTFrate = 88;
const int sNBCperSec = sNBC*sOrbitPerTF*sTFrate;

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

struct ChPairs {
  int mChID1{-1};
  int mChID2{-1};
};

template<std::size_t NChannelsA,std::size_t NChannelsC>
struct EventChDataParams {
  constexpr static std::size_t sNchannelsA = NChannelsA;
  constexpr static std::size_t sNchannelsC = NChannelsC;
  constexpr static std::size_t sNchannelsAC = sNchannelsA + sNchannelsC;
  int mAmpSumA{};
  int mAmpSumC{};
  int mAmpSum{};
  float mMeanTimeA{};
  float mMeanTimeC{};
  int mNchanA{};
  int mNchanC{};
  int mNchan{};
  double mCollTime{};
  double mVrtPos{};
  bool mIsReadyAC{false};
  bool mIsReadyA{false};
  bool mIsReadyC{false};
  void print() const {
    std::cout<<std::endl
    <<"|"<<mMeanTimeA<<"|"<<mMeanTimeC
    <<"|"<<mAmpSumA<<"|"<<mAmpSumC
    <<"|"<<static_cast<int>(mNchanA)<<"|"<<static_cast<int>(mNchanC)
    <<std::endl;
  }
  template<typename AmpType, typename TimeType, typename ChIDtype>
  void fill(const AmpType &amp,const TimeType &time, const ChIDtype &chID) {
    if(chID<sNchannelsA) {
      mNchanA++;
      mMeanTimeA+=time;
      mAmpSumA+=amp;
    }
    else if(chID<sNchannelsAC){
      mNchanC++;
      mMeanTimeC+=time;
      mAmpSumC+=amp;
    }
  }
  void calculate() {
    if(mNchanA>0) {
      mMeanTimeA = mMeanTimeA/mNchanA;
      mIsReadyA=true;
      if(mNchanC>0) {
        mIsReadyAC=true;
      }
    }
    if(mNchanC>0) {
      mMeanTimeC = mMeanTimeC/mNchanC;
      mIsReadyC=true;
    }
    mCollTime = (mMeanTimeA + mMeanTimeC)/2 * sNSperTimeChannel;
    mVrtPos = (mMeanTimeC - mMeanTimeA)/2 * sNSperTimeChannel * sNS2Cm;
    mAmpSum = mAmpSumA + mAmpSumC;
    mNchan = mNchanA + mNchanC;
  }
};
using EventChDataParamsFDD = EventChDataParams<sNchannels_FDDA,sNchannels_FDDC>;
using EventChDataParamsFT0 = EventChDataParams<sNchannels_FT0A,sNchannels_FT0C>;
using EventChDataParamsFV0 = EventChDataParams<sNchannels_FV0A,0>;

void writeResult(TList *listOutput, const std::string &filepathOutput);
std::bitset<sNBC> getCollBC(unsigned int runnum);
const o2::parameters::GRPLHCIFData *getGRPLHCIFData(unsigned int runnum);
void processDigits(unsigned int runnum,
                   const std::vector<std::array<std::string,3> > &vecFilepathsFIT,
/*
                   const std::string &filepathInputFDD,
                   const std::string &filepathInputFT0,
                   const std::string &filepathInputFV0,
*/
                   const std::string &filepathOutput = "hists.root",
                   const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData=nullptr);

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
//  processDigits(runnum, inputPathFDD, inputPathFT0, inputPathFV0, outputPath);

/*
  if(bitsetDet.count()>1) {
    processDigits(runnum, inputPathFDD, inputPathFT0, inputPathFV0, outputPath);
  }
  else {
    std::cout<<"\nWARNING! CANNOT FIND FILE RELATED TO RUN "<<runnum<<" AT PATH "<<pathToSrc<<std::endl;
  }
*/
}
void runAnalysisFull(const std::string &pathToSrcFDD = "/data/work/run3/digits/prod_test/run529418",
                     const std::string &pathToSrcFT0 = "/data/work/run3/digits/prod_test/run529418",
                     const std::string &pathToSrcFV0 = "/data/work/run3/digits/prod_test/run529418",
                     const std::string &pathToDst="histsFull_PbPb3") {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  // Get chunk index from filepath, in format "_chunkIndex.root", returns -1 in case of no index.
  std::function<int(const std::string&)> getChunkIndex = [] (const std::string& filepath) {
    auto regRunNum = std::regex{"_[0-9]{1,}.root"};
    std::smatch sm;
    bool searchResult = std::regex_search(filepath,sm,regRunNum);
    if(searchResult) {
      const std::string st = sm.str();
      const std::string stForm{"_%u.root"};
      int res{};
      std::sscanf(st.c_str(),stForm.c_str(),&res);
      return res;
    }
    else {
      return -1;
    }
  };
  // Get detector index from filepath, in format "_d[1-3]_" if there are no such format, index will belong to FT0 by default
  std::function<int(const std::string&)> getDetIndex = [] (const std::string& filepath) {
    auto regRunNum = std::regex{"_d[1-3]{1}_"};
    std::smatch sm;
    bool searchResult = std::regex_search(filepath,sm,regRunNum);
    if(searchResult) {
      const std::string st = sm.str();
      const std::string stForm{"_d%u_"};
      int res{};
      std::sscanf(st.c_str(),stForm.c_str(),&res);
      return res;
    }
    else {
      return -1;
    }
  };
  // Simple entry for FIT detector filepaths
  struct EntryFilepathFIT {
    std::string mFilepathInputFDD{};
    std::string mFilepathInputFT0{};
    std::string mFilepathInputFV0{};
    void print() const {
      std::cout<<std::endl;
      std::cout<<"\n FDD input: "<<mFilepathInputFDD;
      std::cout<<"\n FT0 input: "<<mFilepathInputFT0;
      std::cout<<"\n FV0 input: "<<mFilepathInputFV0;
      std::cout<<std::endl;
    }
  };
  struct SyncEntryFilenamesFIT {
    unsigned int mRunnum{};
    int mChunkIndex{-1};// starts from 0, if -1 then no chunk index
    std::array<std::string,3> mEntryFilepathsFIT{}; // 0 - FDD, 1 - FT0, 2 - FV0
    std::string getFDD() const { return mEntryFilepathsFIT[0];}
    std::string getFT0() const { return mEntryFilepathsFIT[1];}
    std::string getFV0() const { return mEntryFilepathsFIT[2];}
  };
  std::map<unsigned int, std::map<int, SyncEntryFilenamesFIT> > mapRunnum2Index2Filepath;

  std::function<void(unsigned int,const std::string &)> fillMap = [&mapRunnum2Index2Filepath,&getChunkIndex,&getDetIndex] (unsigned int runnum, const std::string& filepath) {
    auto itRun = mapRunnum2Index2Filepath.insert({runnum,{}});
    const int chunkIndex = getChunkIndex(filepath);
    int detIndex = getDetIndex(filepath);
    auto &mapIndex2Filepath = itRun.first->second;
    auto itChunk = mapIndex2Filepath.insert({chunkIndex,{}});
    auto &syncEntry = itChunk.first->second;
    switch(detIndex) {
      case 1:
        syncEntry.mEntryFilepathsFIT[0]=filepath;
        break;
      case 2:
        syncEntry.mEntryFilepathsFIT[1]=filepath;
        break;
      case 3:
        syncEntry.mEntryFilepathsFIT[2]=filepath;
        break;
      default:
        syncEntry.mEntryFilepathsFIT[1]=filepath;
        break;
    }
    syncEntry.mRunnum = runnum;
    syncEntry.mChunkIndex = chunkIndex;
  };
  std::map<unsigned int,const o2::parameters::GRPLHCIFData *> mapRunnum2CollBC{};

  std::function<void(const std::map<unsigned int,std::vector<std::string> > &)> prepareMaps =
  [&fillMap,&mapRunnum2CollBC](const std::map<unsigned int,std::vector<std::string> > &mapFilepaths) {
    for(const auto &entry: mapFilepaths) {
      const auto &runnum = entry.first;
      const auto &vecFilepaths = entry.second;
      for(const auto &filepath : vecFilepaths) {
        fillMap(runnum,filepath);
      }
      if(mapRunnum2CollBC.find(runnum)==mapRunnum2CollBC.end()) {
        auto ptrGRPLHCIFData = getGRPLHCIFData(runnum);
        mapRunnum2CollBC.insert({runnum,ptrGRPLHCIFData});
      }
    }
  };

  const auto mapRunToFilepathsFDD = Utils::makeMapRunsToFilepathsROOT(pathToSrcFDD);
  const auto mapRunToFilepathsFT0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFT0);
  const auto mapRunToFilepathsFV0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFV0);

  prepareMaps(mapRunToFilepathsFDD);
  prepareMaps(mapRunToFilepathsFT0);
  prepareMaps(mapRunToFilepathsFV0);

  struct Parameters {
    unsigned int mRunnum{};
    std::vector<EntryFilepathFIT> mFilepathInputFIT{};
    std::string mFilepathOutput{};
    const o2::parameters::GRPLHCIFData *mPtrGRPLHCIFData;
  };
  std::vector<Parameters> vecParams{};
  //Apply filtering here
  
  std::function<void(unsigned int)> prepareVecParams = [&vecParams,&mapRunnum2Index2Filepath,&mapRunnum2CollBC,&pathToDst] (unsigned int chunksPerRun) {
    std::cout<<"\nPreparing parameters..";
    std::cout<<"\nNumber of runs: "<<mapRunnum2Index2Filepath.size();
    std::cout<<"\nChunks per runs: "<<chunksPerRun;
    std::cout<<"\n===========================================";
    for(const auto &entryRun: mapRunnum2Index2Filepath) {
      const auto &runnum = entryRun.first;
      const auto &mapChunk2Entry = entryRun.second;
      const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData  = mapRunnum2CollBC.find(runnum)->second;
      int index{};
      int indexOutputChunk{};
      std::cout<<"\nRun "<<runnum<<" with number of files "<<mapChunk2Entry.size();
      for(const auto &entryChunk: mapChunk2Entry) {
        const auto &indexChunk = entryChunk.first;
        const auto &syncFilepaths = entryChunk.second;
        if(!(index%chunksPerRun)) {
          const std::string outputFilepath = pathToDst+"/"+"hist"+std::to_string(runnum)+Form("_%.3u",indexOutputChunk)+".root";
          std::cout<<"\n-------------------------------------------";
          std::cout<<"\nPreparing chunk: "<<indexOutputChunk;
          std::cout<<"\nOutput file path: "<<outputFilepath;
          std::cout<<"\nFiles to process: ";
          std::cout<<"\nFDD:"<<syncFilepaths.getFDD();
          std::cout<<"\nFT0:"<<syncFilepaths.getFT0();
          std::cout<<"\nFV0:"<<syncFilepaths.getFV0();
          vecParams.push_back({runnum,{{syncFilepaths.getFDD(),syncFilepaths.getFT0(),syncFilepaths.getFV0()}},outputFilepath,ptrGRPLHCIFData});
          indexOutputChunk++;
        }
        else {
          auto &backEntry = vecParams.back();
          std::cout<<"\n\nFDD:"<<syncFilepaths.getFDD();
          std::cout<<"\nFT0:"<<syncFilepaths.getFT0();
          std::cout<<"\nFV0:"<<syncFilepaths.getFV0();
          backEntry.mFilepathInputFIT.push_back({syncFilepaths.getFDD(), syncFilepaths.getFT0(), syncFilepaths.getFV0()});
        }
        index++;
      }
      std::cout<<"\n-------------------------------------------";
    }
    std::cout<<"\n===========================================\n";
  };
  /////////////////////////////////////////////////////
  const std::size_t nParallelJobs=12;
  const std::size_t nChunksPerRun=1;
  /////////////////////////////////////////////////////
  ROOT::TProcessExecutor pool(nParallelJobs);
  prepareVecParams(nChunksPerRun);
  //
  const auto result = pool.Map([](const Parameters &entry) {
            std::vector<std::array<std::string,3> > vecFilepathInput{};
            for(const auto &entryFilepath: entry.mFilepathInputFIT) {
              vecFilepathInput.push_back({entryFilepath.mFilepathInputFDD,entryFilepath.mFilepathInputFT0,entryFilepath.mFilepathInputFV0});
            }
            processDigits(entry.mRunnum,vecFilepathInput,entry.mFilepathOutput,entry.mPtrGRPLHCIFData);
            return 0;
          }
          , vecParams);

}

/*
  const auto result = pool.Map([](const Parameters &entry) {
          processDigits(entry.runnum,entry.filepathInputFDD,entry.filepathInputFT0,entry.filepathInputFV0,entry.filepathOutput,entry.collBC,false);
          return 0;}
          , vecParams);
*/

template <typename ChArrType>
bool isChOk (const ChArrType &satArr, int chID, int amp, int adc) {
  if(amp<satArr[adc][chID]) {
    return true;
  }
  else {
    return false;
  }
}

void processDigits(unsigned int runnum,
                   const std::vector<std::array<std::string,3> > &vecFilepathsFIT,
/*
                   const std::string &filepathInputFDD,
                   const std::string &filepathInputFT0,
                   const std::string &filepathInputFV0,
*/
                   const std::string &filepathOutput,
                   const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData)
{
  // saturation coefs
  int FDD_adc0=3950;
  int FDD_adc1=3966;
  int FV0_adc0=3945;
  int FV0_adc1=3945;
  int FT0_adc0[32]={3120,1996,1980,4047,4095,4054,4095,4095,4082,2633,2817,3951,2942,2926,3986,2670,3350,3113,3349,3680,2836,4050,2385,2970,4093,3004,2839,3684,3055,4013,2330,2911};
  int FT0_adc1[32]={3110,2022,1984,4025,3200,4005,4095,3310,3984,2586,2850,3891,2918,4057,2647,3386,3996,3075,3368,3844,2897,3981,2390,2939,4025,3002,2813,3723,3040,4025,2304,2878};

  std::array<int,sNchannels_FDDAC> satPeak_FDD_adc[2]{};
  std::array<int,sNchannels_FT0AC> satPeak_FT0_adc[2]{};
  std::array<int,sNchannels_FV0AC> satPeak_FV0_adc[2]{};
  satPeak_FDD_adc[0].fill(FDD_adc0);
  satPeak_FDD_adc[1].fill(FDD_adc1);
  satPeak_FT0_adc[0].fill(4095);
  satPeak_FT0_adc[1].fill(4095);
  
  satPeak_FV0_adc[0].fill(FV0_adc0);
  satPeak_FV0_adc[1].fill(FV0_adc1);
  for(int i=0;i<32;i++) {
    satPeak_FT0_adc[0][i]=FT0_adc0[i];
    satPeak_FT0_adc[1][i]=FT0_adc1[i];
  }
  
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
  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);
  */

  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);

  uint8_t pmBitsToCheck = pmBitsGood | pmBitsBad; //All except kNumberADC
  uint8_t numberADC = 1<<ChannelDataFT0::kNumberADC;
  std::cout<<"\nGOOD PM BITS: "<<static_cast<int>(pmBitsGood);
  std::cout<<"\nBAD PM BITS: "<<static_cast<int>(pmBitsBad);
  std::cout<<"\nCHECK PM BITS: "<<static_cast<int>(pmBitsToCheck);

  uint8_t validBits = 0b10011111;
  uint8_t bitsIsDataValid = 0b10000000;
  std::bitset<sNBC> collBC{}, collBC_A{}, collBC_C{};
  if(ptrGRPLHCIFData!=nullptr) {
    const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }


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
  
  TList *listFDD = new TList();
  listFDD->SetOwner(true);
  listFDD->SetName("output");
  {
    const std::string filepathOutputBuf = filepathNoExt+"_FDD.root";
    mapOutput2Store.insert({filepathOutputBuf, listFDD});
  }
  listGarbage->Add(listFDD);

  TList *listFT0 = new TList();
  listFT0->SetOwner(true);
  listFT0->SetName("output");
  {
    const std::string filepathOutputBuf = filepathNoExt+"_FT0.root";
    mapOutput2Store.insert({filepathOutputBuf, listFT0});
  }
  listGarbage->Add(listFT0);

  TList *listFV0 = new TList();
  listFV0->SetOwner(true);
  listFV0->SetName("output");
  {
    const std::string filepathOutputBuf = filepathNoExt+"_FV0.root";
    mapOutput2Store.insert({filepathOutputBuf, listFV0});
  }
  listGarbage->Add(listFV0);

  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  const uint32_t startOrbit = 28756480;
  const double runDuration = 10*3600;// run duration in seconds
  const double orbitRate = 128.*88.;
  auto getTimeSinceSOR = [&startOrbit,&orbitRate] (uint32_t orbit) ->double {
    return static_cast<double>(orbit - startOrbit)/orbitRate;
  };

  std::array<TH2F *,8> arrSumAmpFDD_Ts{};
  std::array<TH2F *,8> arrSumAmpFT0_Ts{};
  std::array<TH2F *,8> arrSumAmpFV0_Ts{};

  std::array<TH2F *,8> arrSumAmpFDD_Ts_noPMbit{};
  std::array<TH2F *,8> arrSumAmpFT0_Ts_noPMbit{};
  std::array<TH2F *,8> arrSumAmpFV0_Ts_noPMbit{};


  for(int i=0;i<8;i++) {
    if(i==5||i==6) continue;
    std::string trgNameFDD = mMapTrgNamesFDD.find(i+1)->second;
    std::string trgNameFT0 = mMapTrgNamesFT0.find(i+1)->second;
    std::string trgNameFV0 = mMapTrgNamesFV0.find(i+1)->second;
    {
      std::string histName = "hSumAmpFDD_Ts_trg"+trgNameFDD;
      std::string histTitle = "SumAmp FDD timeseries, trigger "+trgNameFDD+";Time since SOR [seconds];SumAmp [ADC]";
      TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),3600,0,runDuration,4000,0,40000);
      arrSumAmpFDD_Ts[i] = hist;
      listFDD->Add(hist);
    }
    {
      std::string histName = "hSumAmpFDD_Ts_noPMbits_trg"+trgNameFDD;
      std::string histTitle = "SumAmp FDD timeseries, trigger "+trgNameFDD+" (no PMbit cut);Time since SOR [seconds];SumAmp [ADC]";
      TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),3600,0,runDuration,4000,0,40000);
      arrSumAmpFDD_Ts_noPMbit[i] = hist;
      listFDD->Add(hist);
    }

    {
      std::string histName = "hSumAmpFT0_Ts_trg"+trgNameFT0;
      std::string histTitle = "SumAmp FT0 timeseries, trigger "+trgNameFT0+";Time since SOR [seconds];SumAmp [ADC]";
      TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),3600,0,runDuration,4000,0,40000);
      arrSumAmpFT0_Ts[i] = hist;
      listFT0->Add(hist);
    }
    {
      std::string histName = "hSumAmpFT0_Ts_noPMbits_trg"+trgNameFT0;
      std::string histTitle = "SumAmp FT0 timeseries, trigger "+trgNameFT0+" (no PMbit cut);Time since SOR [seconds];SumAmp [ADC]";
      TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),3600,0,runDuration,4000,0,40000);
      arrSumAmpFT0_Ts_noPMbit[i] = hist;
      listFT0->Add(hist);
    }


    {
      std::string histName = "hSumAmpFV0_Ts_trg"+trgNameFV0;
      std::string histTitle = "SumAmp FV0 timeseries, trigger "+trgNameFV0+";Time since SOR [seconds];SumAmp [ADC]";
      TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),3600,0,runDuration,4000,0,40000);
      arrSumAmpFV0_Ts[i] = hist;
      listFV0->Add(hist);
    }
    {
      std::string histName = "hSumAmpFV0_Ts_noPMbits_trg"+trgNameFV0;
      std::string histTitle = "SumAmp FV0 timeseries, trigger "+trgNameFV0+" (no PMbit cut);Time since SOR [seconds];SumAmp [ADC]";
      TH2F *hist = new TH2F(histName.c_str(),histTitle.c_str(),3600,0,runDuration,4000,0,40000);
      arrSumAmpFV0_Ts_noPMbit[i] = hist;
      listFV0->Add(hist);
    }


  }



  TH2F *hAmpPerChFDD_adc0_pure = new TH2F("hAmpPerChFDD_adc0_pure","FDD Amplitude per Channel ID (ADC0, good PM bits);Channel [ID];Amplitude [ADC]",sNchannels_FDDAC,0,sNchannels_FDDAC,4200,-100,4100);
  listOutput->Add(hAmpPerChFDD_adc0_pure);

  TH2F *hAmpPerChFDD_adc1_pure = new TH2F("hAmpPerChFDD_adc1_pure","FDD Amplitude per Channel ID (ADC1, good PM bits);Channel [ID];Amplitude [ADC]",sNchannels_FDDAC,0,sNchannels_FDDAC,4200,-100,4100);
  listOutput->Add(hAmpPerChFDD_adc1_pure);

  TH2F *hAmpPerChFT0_adc0_pure = new TH2F("hAmpPerChFT0_adc0_pure","FT0 Amplitude per Channel ID (ADC0, good PM bits);Channel [ID];Amplitude [ADC]",sNchannels_FT0AC,0,sNchannels_FT0AC,4200,-100,4100);
  listOutput->Add(hAmpPerChFT0_adc0_pure);

  TH2F *hAmpPerChFT0_adc1_pure = new TH2F("hAmpPerChFT0_adc1_pure","FT0 Amplitude per Channel ID (ADC1, good PM bits);Channel [ID];Amplitude [ADC]",sNchannels_FT0AC,0,sNchannels_FT0AC,4200,-100,4100);
  listOutput->Add(hAmpPerChFT0_adc1_pure);

  TH2F *hAmpPerChFV0_adc0_pure = new TH2F("hAmpPerChFV0_adc0_pure","FV0 Amplitude per Channel ID (ADC0, good PM bits);Channel [ID];Amplitude [ADC]",sNchannels_FV0AC,0,sNchannels_FV0AC,4200,-100,4100);
  listOutput->Add(hAmpPerChFV0_adc0_pure);

  TH2F *hAmpPerChFV0_adc1_pure = new TH2F("hAmpPerChFV0_adc1_pure","FV0 Amplitude per Channel ID (ADC1, good PM bits);Channel [ID];Amplitude [ADC]",sNchannels_FV0AC,0,sNchannels_FV0AC,4200,-100,4100);
  listOutput->Add(hAmpPerChFV0_adc1_pure);

  
  TH2F *hCollisionTimeVsVertexFT0_noCut = new TH2F("hCollisionTimeVsVertexFT0_noCut","Collision time vs Vertex position, FT0(w/o cuts);Vertex [cm];Collision time [ns]",2000,-100,100,1000,-50,50);
  listOutput->Add(hCollisionTimeVsVertexFT0_noCut);

  TH2F *hCollisionTimeVsVertexFT0_vrtTrg = new TH2F("hCollisionTimeVsVertexFT0_vrtTrg","Collision time vs Vertex position, FT0(vertex trigger);Vertex [cm];Collision time [ns]",2000,-100,100,1000,-50,50);
  listOutput->Add(hCollisionTimeVsVertexFT0_vrtTrg);


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
  for(const auto &entryFilepath: vecFilepathsFIT) {
    //TFile *fileInputFDD = TFile::Open(filepathInputFDD.c_str());
    //TFile *fileInputFT0 = TFile::Open(filepathInputFT0.c_str());
    //TFile *fileInputFV0 = TFile::Open(filepathInputFV0.c_str());
    const std::string filepathInputFDD = entryFilepath[0];
    const std::string filepathInputFT0 = entryFilepath[1];
    const std::string filepathInputFV0 = entryFilepath[2];

    std::cout<<"\nProcessing file: "<<filepathInputFDD<<"\nProcessing file: "<<filepathInputFT0<<"\nProcessing file: "<<filepathInputFV0<<std::endl;

    

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

        const auto trgBitsFDD_FT0 = trgBitsFDD & trgBitsFT0;
        const auto trgBitsFDD_FV0 = trgBitsFDD & trgBitsFV0;
        const auto trgBitsFT0_FV0 = trgBitsFT0 & trgBitsFV0;
        const auto trgBitsFIT = trgBitsFDD_FT0 & trgBitsFDD_FV0;

        const auto& channelsFDD = digitFDD.getBunchChannelData(vecChannelDataFDD);
        const auto& channelsFT0 = digitFT0.getBunchChannelData(vecChannelDataFT0);
        const auto& channelsFV0 = digitFV0.getBunchChannelData(vecChannelDataFV0);
        const bool isVrt_FDD = trgFDD.getVertex();
        const bool isVrt_FT0 = trgFT0.getVertex();
        const bool isOrA_FV0 = trgFV0.getOrA();
        const bool isCollision = collBC.test(bc);
        const auto timeSinceSOR = getTimeSinceSOR(orbit);
        
        if(!isCollision) continue;
        // ChannelData processing
        //FDD
        EventChDataParamsFDD chDataParamsFDD_noCut{};
        EventChDataParamsFDD chDataParamsFDD{};
        EventChDataParamsFDD chDataParamsFDD_sat{};

        for(const auto &channelDataFDD: channelsFDD) {
          const auto &amp = channelDataFDD.mChargeADC;
          const auto &time = channelDataFDD.mTime;
          const auto &chID = channelDataFDD.mPMNumber;
          const auto &pmBits = channelDataFDD.mFEEBits;
          const int adc = int((pmBits & numberADC)!=0);
          const bool isNotSat = isChOk(satPeak_FDD_adc,chID,amp,adc);

          chDataParamsFDD_noCut.fill(amp,time,chID);
          if(((pmBits & pmBitsToCheck) == pmBitsGood)/*&&isCollision&&isVrt_FDD*/) {
            chDataParamsFDD.fill(amp,time,chID);
            if(adc) {
              hAmpPerChFDD_adc1_pure->Fill(chID,amp);
            }
            else {
              hAmpPerChFDD_adc0_pure->Fill(chID,amp);
            }
          }
        }

        chDataParamsFDD_noCut.calculate();
        chDataParamsFDD.calculate();
        chDataParamsFDD_sat.calculate();
        //FT0
        EventChDataParamsFT0 chDataParamsFT0_noCut{};
        EventChDataParamsFT0 chDataParamsFT0{};
        EventChDataParamsFT0 chDataParamsFT0_sat{};
 
        for(const auto &channelDataFT0: channelsFT0) {
          const auto &amp = channelDataFT0.QTCAmpl;
          const auto &time = channelDataFT0.CFDTime;
          const auto &chID = channelDataFT0.ChId;
          const auto &pmBits = channelDataFT0.ChainQTC;
          const int adc = int((pmBits & numberADC)!=0);
          const bool isNotSat = isChOk(satPeak_FT0_adc,chID,amp,adc);
          chDataParamsFT0_noCut.fill(amp,time,chID);
          if(((pmBits & pmBitsToCheck) == pmBitsGood)) {
            chDataParamsFT0.fill(amp,time,chID);
            if(adc) {
              hAmpPerChFT0_adc1_pure->Fill(chID,amp);
            }
            else {
              hAmpPerChFT0_adc0_pure->Fill(chID,amp);
            }
          }
        }
        chDataParamsFT0_noCut.calculate();
        chDataParamsFT0.calculate();
        chDataParamsFT0_sat.calculate();

        //FV0
        EventChDataParamsFV0 chDataParamsFV0_noCut{};
        EventChDataParamsFV0 chDataParamsFV0{};
        EventChDataParamsFV0 chDataParamsFV0_sat{};

        for(const auto &channelDataFV0: channelsFV0) {
          const auto &amp = channelDataFV0.QTCAmpl;
          const auto &time = channelDataFV0.CFDTime;
          const auto &chID = channelDataFV0.ChId;
          const auto &pmBits = channelDataFV0.ChainQTC;
          const int adc = int((pmBits & numberADC)!=0);
          const bool isNotSat = isChOk(satPeak_FDD_adc,chID,amp,adc);

          chDataParamsFV0_noCut.fill(amp,time,chID);
          if(((pmBits & pmBitsToCheck) == pmBitsGood) ) {
            chDataParamsFV0.fill(amp,time,chID);
            if(adc) {
              hAmpPerChFV0_adc1_pure->Fill(chID,amp);
            }
            else {
              hAmpPerChFV0_adc0_pure->Fill(chID,amp);
            }
          }
        }
        chDataParamsFV0_noCut.calculate();
        chDataParamsFV0.calculate();
        chDataParamsFV0_sat.calculate();
        //Trigger
        //FDD
        std::vector<uint8_t> vecTrgBitsFDD{};
        for(int iTrg=0; iTrg < 8; iTrg++ ) {
          if(trgBitsFDD & (1<<iTrg)) {
            if(chDataParamsFDD.mNchan)  arrSumAmpFDD_Ts[iTrg]->Fill(timeSinceSOR,chDataParamsFDD.mAmpSum);
            if(chDataParamsFDD_noCut.mNchan)  arrSumAmpFDD_Ts_noPMbit[iTrg]->Fill(timeSinceSOR,chDataParamsFDD_noCut.mAmpSum);

/*
            hTriggerFDD_BC->Fill(bc,iTrg);
            vecTrgBitsFDD.push_back(iTrg);
*/
          }
        }
        //FT0
        std::vector<uint8_t> vecTrgBitsFT0{};
        for(int iTrg = 0; iTrg < 8; iTrg++ ) {
          if(trgBitsFT0 & (1<<iTrg)) {
            if(chDataParamsFT0.mNchan)  arrSumAmpFT0_Ts[iTrg]->Fill(timeSinceSOR,chDataParamsFT0.mAmpSum);
            if(chDataParamsFT0_noCut.mNchan)  arrSumAmpFT0_Ts_noPMbit[iTrg]->Fill(timeSinceSOR,chDataParamsFT0_noCut.mAmpSum);

/*
            hTriggerFT0_BC->Fill(bc,iTrg);
            vecTrgBitsFT0.push_back(iTrg);
            const auto trgBitFT0{iTrg};
            for(const auto &trgBitFDD: vecTrgBitsFDD) {
              hTriggerFDD_FT0->Fill(trgBitFDD,trgBitFT0);
            }
*/
          }
        }
        //FV0
        std::vector<uint8_t> vecTrgBitsFV0{};
        for(int iTrg = 0; iTrg < 8; iTrg++ ) {
          if(trgBitsFV0 & (1<<iTrg)) {
            if(chDataParamsFV0.mNchan)  arrSumAmpFV0_Ts[iTrg]->Fill(timeSinceSOR,chDataParamsFV0.mAmpSum);
            if(chDataParamsFV0_noCut.mNchan)  arrSumAmpFV0_Ts_noPMbit[iTrg]->Fill(timeSinceSOR,chDataParamsFV0_noCut.mAmpSum);
/*
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
*/
          }
        }
        /////////////////
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
const o2::parameters::GRPLHCIFData *getGRPLHCIFData(unsigned int runnum) {
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
    return nullptr;
  }
  std::map<std::string, std::string> mapMetadata;
  std::map<std::string, std::string> mapHeader;
  const auto *ptrGRPLHCIFData = ccdb_api.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF",mapMetadata,ts,&mapHeader);
  return ptrGRPLHCIFData;
}   