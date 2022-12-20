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
  uint8_t mNchanA{};
  uint8_t mNchanC{};
  double mCollTime{};
  double mVrtPos{};
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
    if(mNchanA>0) mMeanTimeA = mMeanTimeA/mNchanA;
    if(mNchanC>0) mMeanTimeC = mMeanTimeC/mNchanC;
    mCollTime = (mMeanTimeA + mMeanTimeC)/2 * sNSperTimeChannel;
    mVrtPos = (mMeanTimeC - mMeanTimeA)/2 * sNSperTimeChannel * sNS2Cm;
  }
};
using EventChDataParamsFDD = EventChDataParams<sNchannels_FDDA,sNchannels_FDDC>;
using EventChDataParamsFT0 = EventChDataParams<sNchannels_FT0A,sNchannels_FT0C>;
using EventChDataParamsFV0 = EventChDataParams<sNchannels_FV0A,0>;

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

void runAnalysisFull(const std::string &pathToSrcFDD = "/data/work/run3/digits/production_all/d1",
                     const std::string &pathToSrcFT0 = "/data/work/run3/digits/production_all/d2",
                     const std::string &pathToSrcFV0 = "/data/work/run3/digits/production_all/d3",
                     const std::string &pathToDst="") {

  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  const auto mapRunToFilepathsFDD = Utils::makeMapRunsToFilepathsROOT(pathToSrcFDD);
  const auto mapRunToFilepathsFT0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFT0);
  const auto mapRunToFilepathsFV0 = Utils::makeMapRunsToFilepathsROOT(pathToSrcFV0);

  const std::size_t nParallelJobs=40;
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
  for(const auto &entry: mapRunToFilepathsFDD) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    const std::string filepath = vecFilepaths[0];
    const std::string outputFile = pathToDst+"/"+"hist"+std::to_string(runnum)+".root";
    auto it = mapParameters.insert({runnum,{}});
    it.first->second.runnum = runnum;
    it.first->second.filepathInputFDD = filepath;
    it.first->second.filepathOutput = outputFile;
    it.first->second.init();
  }

  for(const auto &entry: mapRunToFilepathsFT0) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    const std::string filepath = vecFilepaths[0];
    const std::string outputFile = pathToDst+"/"+"hist"+std::to_string(runnum)+".root";
    auto it = mapParameters.insert({runnum,{}});
    it.first->second.runnum = runnum;
    it.first->second.filepathInputFT0 = filepath;
    it.first->second.filepathOutput = outputFile;
    it.first->second.init();
  }

  for(const auto &entry: mapRunToFilepathsFV0) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    const std::string filepath = vecFilepaths[0];
    const std::string outputFile = pathToDst+"/"+"hist"+std::to_string(runnum)+".root";
    auto it = mapParameters.insert({runnum,{}});
    it.first->second.runnum = runnum;
    it.first->second.filepathInputFV0 = filepath;
    it.first->second.filepathOutput = outputFile;
    it.first->second.init();
  }

  for(const auto &entry: mapParameters) {
    vecParams.push_back(entry.second);
  }

  const auto result = pool.Map([](const Parameters &entry) {
          processDigits(entry.runnum,entry.filepathInputFDD,entry.filepathInputFT0,entry.filepathInputFV0,entry.filepathOutput,entry.collBC);
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
/*
  TList *listOutputAmpBC = new TList();
  listOutputAmpBC->SetOwner(true);
  listOutputAmpBC->SetName("output");
  {
    const std::string filepathOutputBuf = filepathNoExt + std::string{"_AmpBC.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputAmpBC};
  }
  listGarbage->Add(listOutputAmpBC);

  TList *listOutputTimeBC = new TList();
  listOutputTimeBC->SetOwner(true);
  listOutputTimeBC->SetName("output");
  {
    const std::string filepathOutputBuf = filepathNoExt + std::string{"_TimeBC.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputTimeBC};
  }
  listGarbage->Add(listOutputTimeBC);
*/
  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  TH2F *hTriggerFDD_FT0 = new TH2F("hTriggerFDD_FT0","Trigger correlation FDD vs FT0;TriggerFDD;TriggerFT0",
                                   mMapTrgNamesFDD.size(),0,mMapTrgNamesFDD.size(),
                                   mMapTrgNamesFT0.size(),0,mMapTrgNamesFT0.size());
  HistHelper::makeHistBinNamed(hTriggerFDD_FT0,mMapTrgNamesFDD,0);
  HistHelper::makeHistBinNamed(hTriggerFDD_FT0,mMapTrgNamesFT0,1);
  listOutput->Add(hTriggerFDD_FT0);

  TH2F *hTriggerFDD_FV0 = new TH2F("hTriggerFDD_FV0","Trigger correlation FDD vs FV0;TriggerFDD;TriggerFV0",
                                   mMapTrgNamesFDD.size(),0,mMapTrgNamesFDD.size(),
                                   mMapTrgNamesFV0.size(),0,mMapTrgNamesFV0.size());
  HistHelper::makeHistBinNamed(hTriggerFDD_FV0,mMapTrgNamesFDD,0);
  HistHelper::makeHistBinNamed(hTriggerFDD_FV0,mMapTrgNamesFV0,1);
  listOutput->Add(hTriggerFDD_FV0);

  TH2F *hTriggerFT0_FV0 = new TH2F("hTriggerFT0_FV0","Trigger correlation FT0 vs FV0;TriggerFT0;TriggerFV0",
                                   mMapTrgNamesFT0.size(),0,mMapTrgNamesFT0.size(),
                                   mMapTrgNamesFV0.size(),0,mMapTrgNamesFV0.size());
  HistHelper::makeHistBinNamed(hTriggerFT0_FV0,mMapTrgNamesFT0,0);
  HistHelper::makeHistBinNamed(hTriggerFT0_FV0,mMapTrgNamesFV0,1);
  listOutput->Add(hTriggerFT0_FV0);

  std::array<TH2F*, 8> hArrTriggerFT0_FDD_FV0{};
  for(int iTrgFT0=0;iTrgFT0<mMapTrgNamesFT0.size();iTrgFT0++) {
    std::string histName = "hTriggerFT0_"+ mMapTrgNamesFT0.find(iTrgFT0+1)->second+"_FV0_FDD";
    std::string histTitle = "Trigger correlation FDD vs FV0, with FT0 trigger("+ mMapTrgNamesFT0.find(iTrgFT0+1)->second+");TriggerFDD;TriggerFV0";
    TH2F *hTmp = new TH2F(histName.c_str(),histTitle.c_str(),
                          mMapTrgNamesFDD.size(),0,mMapTrgNamesFDD.size(),
                          mMapTrgNamesFV0.size(),0,mMapTrgNamesFV0.size());
    HistHelper::makeHistBinNamed(hTmp,mMapTrgNamesFDD,0);
    HistHelper::makeHistBinNamed(hTmp,mMapTrgNamesFV0,1);
    hArrTriggerFT0_FDD_FV0[iTrgFT0] = hTmp;
    listOutput->Add(hTmp);
  }
/*
  TH2F *hTriggersVsBC = new TH2F("hTriggersVsBC","Triggers vs BC;BC;Triggers",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersVsBC,mMapTrgNames,1);
  listTriggers->Add(hTriggersVsBC);
*/
  TH2F *hCollisionTimeVsVertexFDD_noCut_spotFT0_1 = new TH2F("hCollisionTimeVsVertexFDD_noCut_spotFT0_1","Collision time vs Vertex position, FDD(w/o cuts, FT0 spot #1);Vertex [cm];Collision time [ns]",6000,-300,300,1000,-20,80);
  listOutput->Add(hCollisionTimeVsVertexFDD_noCut_spotFT0_1);

  TH2F *hMeanTimeVsSumAmpFV0_noCut_spotFT0_1 = new TH2F("hMeanTimeVsSumAmpFV0_noCut_spotFT0_1","MeanTime vs SumAmp, FV0, (w/o cuts, FT0 spot #1);MeanTime [ns];SumAmp",1000,-20,80,10000,0,100000);
  listOutput->Add(hMeanTimeVsSumAmpFV0_noCut_spotFT0_1);

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
        const auto &ir = entry.first;;
        const auto &bc = ir.bc;
        const auto &orbit = ir.orbit;
        const double secSinceSOR = 1.*(orbit*sNBC+bc)/sNBCperSec;

        const auto &digitFDD = entry.second.isFDD() ? vecDigitsFDD[entry.second.getIndexFDD()] : dummyDigitFDD;
        const auto &digitFT0 = entry.second.isFT0() ? vecDigitsFT0[entry.second.getIndexFT0()] : dummyDigitFT0;
        const auto &digitFV0 = entry.second.isFV0() ? vecDigitsFV0[entry.second.getIndexFV0()] : dummyDigitFV0;

        const auto &trgFDD = digitFDD.mTriggers;
        const auto &trgFT0 = digitFT0.mTriggers;
        const auto &trgFV0 = digitFV0.mTriggers;

        const auto &trgBitsFDD = trgFDD.getTriggersignals();
        const auto &trgBitsFT0 = trgFT0.getTriggersignals();
        const auto &trgBitsFV0 = trgFV0.getTriggersignals();
        const auto trgBitsFDD_FT0 = trgBitsFDD & trgBitsFT0;
        const auto trgBitsFDD_FV0 = trgBitsFDD & trgBitsFV0;
        const auto trgBitsFT0_FV0 = trgBitsFT0 & trgBitsFV0;
        const auto trgBitsFIT = trgBitsFDD_FT0 & trgBitsFDD_FV0;

        const auto& channelsFDD = digitFDD.getBunchChannelData(vecChannelDataFDD);
        const auto& channelsFT0 = digitFT0.getBunchChannelData(vecChannelDataFT0);
        const auto& channelsFV0 = digitFV0.getBunchChannelData(vecChannelDataFV0);

        const bool isCollision = collBC.test(bc);
        // ChannelData processing
        //FDD
        EventChDataParamsFDD chDataParamsFDD_noCut{};
        EventChDataParamsFDD chDataParamsFDD{};

        for(const auto &channelDataFDD: channelsFDD) {
          const auto &amp = channelDataFDD.mChargeADC;
          const auto &time = channelDataFDD.mTime;
          const auto &chID = channelDataFDD.mPMNumber;
          const auto &pmBits = channelDataFDD.mFEEBits;

          chDataParamsFDD_noCut.fill(amp,time,chID);
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
            chDataParamsFDD.fill(amp,time,chID);
          }
        }
        chDataParamsFDD_noCut.calculate();
        chDataParamsFDD.calculate();

        //FT0
        EventChDataParamsFT0 chDataParamsFT0_noCut{};
        EventChDataParamsFT0 chDataParamsFT0{};

        for(const auto &channelDataFT0: channelsFT0) {
          const auto &amp = channelDataFT0.QTCAmpl;
          const auto &time = channelDataFT0.CFDTime;
          const auto &chID = channelDataFT0.ChId;
          const auto &pmBits = channelDataFT0.ChainQTC;

          chDataParamsFT0_noCut.fill(amp,time,chID);
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
            chDataParamsFT0.fill(amp,time,chID);
          }
        }
        chDataParamsFT0_noCut.calculate();
        chDataParamsFT0.calculate();

        //FV0
        EventChDataParamsFV0 chDataParamsFV0_noCut{};
        EventChDataParamsFV0 chDataParamsFV0{};

        for(const auto &channelDataFV0: channelsFV0) {
          const auto &amp = channelDataFV0.QTCAmpl;
          const auto &time = channelDataFV0.CFDTime;
          const auto &chID = channelDataFV0.ChId;
          const auto &pmBits = channelDataFV0.ChainQTC;

          chDataParamsFV0_noCut.fill(amp,time,chID);
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
            chDataParamsFV0.fill(amp,time,chID);
          }
        }
        chDataParamsFV0_noCut.calculate();
        chDataParamsFV0.calculate();

        //Trigger
        //FDD
        std::vector<uint8_t> vecTrgBitsFDD{};
        for(int iTrg=0; iTrg < 8; iTrg++ ) {
          if(trgBitsFDD & (1<<iTrg)) {
            vecTrgBitsFDD.push_back(iTrg);
          }
        }
        //FT0
        std::vector<uint8_t> vecTrgBitsFT0{};
        for(int iTrg = 0; iTrg < 8; iTrg++ ) {
          if(trgBitsFT0 & (1<<iTrg)) {
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

        const bool isSpotFT0_1 = (chDataParamsFT0_noCut.mCollTime>-3.5)
                              && (chDataParamsFT0_noCut.mCollTime<-1.5)
                              && (chDataParamsFT0_noCut.mVrtPos>-90.)
                              && (chDataParamsFT0_noCut.mVrtPos<-70.);
        if(isSpotFT0_1) {
          hCollisionTimeVsVertexFDD_noCut_spotFT0_1->Fill(chDataParamsFDD_noCut.mCollTime,chDataParamsFDD_noCut.mVrtPos);
          hMeanTimeVsSumAmpFV0_noCut_spotFT0_1->Fill(chDataParamsFT0_noCut.mMeanTimeA,chDataParamsFT0_noCut.mAmpSumA);
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
