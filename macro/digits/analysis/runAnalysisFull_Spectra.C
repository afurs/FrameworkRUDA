#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/LookUpTable.h"
#include "DataFormatsParameters/GRPLHCIFData.h"
#include "FT0Base/Geometry.h"

#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "ROOT/TProcessExecutor.hxx"

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
const int sNchannelsFT0 = 212;
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

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using LUT = o2::fit::LookupTableBase<>;
using DigitFT0 = o2::ft0::Digit;
using ChannelDataFT0 = o2::ft0::ChannelData;

const o2::parameters::GRPLHCIFData *getGRPLHCIFData(unsigned int runnum);
void processDigits(unsigned int runnum, const std::vector<std::string> &vecFilepathInput,const std::string &filepathOutput,const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData);

void runAnalysisFull(const std::string &pathToSrcFDD = "/data/work/run3/digits/prod_test/run528231",
                     const std::string &pathToSrcFT0 = "/data/work/run3/digits/prod_test/run528231",
                     const std::string &pathToSrcFV0 = "/data/work/run3/digits/prod_test/run528231",
                     const std::string &pathToDst="hists_528231") {
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
          std::cout<<"\nFT0 files to process: ";
          std::cout<<"\n"<<syncFilepaths.getFT0();
          vecParams.push_back({runnum,{{syncFilepaths.getFDD(),syncFilepaths.getFT0(),syncFilepaths.getFV0()}},outputFilepath,ptrGRPLHCIFData});
          indexOutputChunk++;
        }
        else {
          auto &backEntry = vecParams.back();
          std::cout<<"\n"<<syncFilepaths.getFT0();
          backEntry.mFilepathInputFIT.push_back({syncFilepaths.getFDD(), syncFilepaths.getFT0(), syncFilepaths.getFV0()});
        }
        index++;
      }
      std::cout<<"\n-------------------------------------------";
    }
    std::cout<<"\n===========================================\n";
  };
  /////////////////////////////////////////////////////
  const std::size_t nParallelJobs=10;
  const std::size_t nChunksPerRun=10;
  /////////////////////////////////////////////////////
  ROOT::TProcessExecutor pool(nParallelJobs);
  prepareVecParams(nChunksPerRun);
  //
  const auto result = pool.Map([](const Parameters &entry) {
            std::vector<std::string> vecFilepathInput{};
            for(const auto &entryFilepath: entry.mFilepathInputFIT) {
              vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFT0);
            }
            processDigits(entry.mRunnum,vecFilepathInput,entry.mFilepathOutput,entry.mPtrGRPLHCIFData);
            return 0;
          }
          , vecParams);

}

void processDigits(unsigned int runnum, const std::vector<std::string> &vecFilepathInput, const std::string &filepathOutput,const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData)
{
  //Load libraries and define types
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //LUT
  const std::string lutFilepathJSON = (runnum < 520143) ? std::string{"lutFT0_old.json"} : std::string{"lutFT0_new.json"};
  LUT lut(lutFilepathJSON);
  std::map<std::string, uint8_t> mapFEE2hash;
  std::array<uint8_t, 230> mChID2PMhash; // map chID->hashed PM value
  uint8_t mTCMhash;
  auto lutSorted = lut.getVecMetadataFEE();
  std::sort(lutSorted.begin(), lutSorted.end(), [](const auto& first, const auto& second) { return first.mModuleName < second.mModuleName; });
  uint8_t binPos{ 0 };
  for (const auto& lutEntry : lutSorted) {
    const auto& moduleName = lutEntry.mModuleName;
    const auto& moduleType = lutEntry.mModuleType;
    const auto& strChID = lutEntry.mChannelID;
    const auto& pairIt = mapFEE2hash.insert({ moduleName, binPos });
    if (pairIt.second) {
      binPos++;
    }
    if (std::regex_match(strChID, std::regex("[[\\d]{1,3}"))) {
      int chID = std::stoi(strChID);
      if (chID < 230) {
        mChID2PMhash[chID] = mapFEE2hash[moduleName];
      } else {
        std::cout << "\nIncorrect LUT entry: chID " << strChID << " | " << moduleName;
      }
    } else if (moduleType != "TCM") {
      std::cout << "\nNon-TCM module w/o numerical chID: chID " << strChID << " | " << moduleName;
    } else if (moduleType == "TCM") {
      mTCMhash = mapFEE2hash[moduleName];
    }
  }

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
  //PM bits
  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);
  uint8_t pmBitsToCheck = pmBitsGood | pmBitsBad; //All except kNumberADC
  //Collision schema
  std::bitset<sNBC> collBC{}, collBC_A{}, collBC_C{}, collBC_E{};
/*
  if(ptrGRPLHCIFData==nullptr && uploadFromCCDB==true) {
    const auto *ptrGRPLHCIFData_tmp = getGRPLHCIFData(runnum);
    const auto &bunchFilling = ptrGRPLHCIFData_tmp->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }
*/
  if(ptrGRPLHCIFData!=nullptr) {
    const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }
  // Map with output lists and their filepaths
  std::map<std::string,TList *> mapOutput2Store{};
  //Garbage list
  TList *listGarbage = new TList();
  listGarbage->SetOwner(true);
  listGarbage->SetName("output");
  //Output listsm, common
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  listGarbage->Add(listOutput);
  {
    const std::string filepathOutputBuf = filepathOutput;
//    mapOutput2Store.insert({filepathOutputBuf, listOutput});
  }

  //Amp per BC side A
  TList *listOutputAmpA_BC = new TList();
  listOutputAmpA_BC->SetOwner(true);
  listOutputAmpA_BC->SetName("output");
  listGarbage->Add(listOutputAmpA_BC);
  {
    const std::string filepathOutputBuf = filepathNoExt+std::string{"_AmpA.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputAmpA_BC});
  }

  //Amp per BC side C
  TList *listOutputAmpC_BC = new TList();
  listOutputAmpC_BC->SetOwner(true);
  listOutputAmpC_BC->SetName("output");
  listGarbage->Add(listOutputAmpC_BC);
  {
    const std::string filepathOutputBuf = filepathNoExt+std::string{"_AmpC.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputAmpC_BC});
  }

  //Time per BC side A
  TList *listOutputTimeA = new TList();
  listOutputTimeA->SetOwner(true);
  listOutputTimeA->SetName("output");
  listGarbage->Add(listOutputTimeA);
  {
    const std::string filepathOutputBuf = filepathNoExt+std::string{"_TimeA.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputTimeA});
  }

  TList *listOutputTimeA_BC = new TList();
  listOutputTimeA_BC->SetOwner(true);
  listOutputTimeA_BC->SetName("TimeA_BC");
  listOutputTimeA->Add(listOutputTimeA_BC);

  TList *listOutputTimeA_nPrevEvents = new TList();
  listOutputTimeA_nPrevEvents->SetOwner(true);
  listOutputTimeA_nPrevEvents->SetName("TimeA_nPrevEvents");
  listOutputTimeA->Add(listOutputTimeA_nPrevEvents);

  //Time per BC side C
  TList *listOutputTimeC = new TList();
  listOutputTimeC->SetOwner(true);
  listOutputTimeC->SetName("output");
  listGarbage->Add(listOutputTimeC);
  {
    const std::string filepathOutputBuf = filepathNoExt+std::string{"_TimeC.root"};
    mapOutput2Store.insert({filepathOutputBuf, listOutputTimeC});
  }

  TList *listOutputTimeC_BC = new TList();
  listOutputTimeC_BC->SetOwner(true);
  listOutputTimeC_BC->SetName("TimeC_BC");
  listOutputTimeC->Add(listOutputTimeC_BC);

  TList *listOutputTimeC_nPrevEvents = new TList();
  listOutputTimeC_nPrevEvents->SetOwner(true);
  listOutputTimeC_nPrevEvents->SetName("TimeC_nPrevEvents");
  listOutputTimeC->Add(listOutputTimeC_nPrevEvents);

  //Time
  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  
  std::set<unsigned int> setChIDsSpectraPerBC;
  std::bitset<sNchannelsAC> bitsetChIDsSpectraPerBC;
  for(int iCh=0;iCh<sNchannelsAC;iCh++) {
    setChIDsSpectraPerBC.insert(iCh);
    bitsetChIDsSpectraPerBC.set(iCh);
  }

  std::array<TH2F*, sNchannelsAC> arrHistAmpChID_PerBC{};
  std::array<TH2F*, sNchannelsAC> arrHistTimeChID_PerBC{};
  std::array<TH2F*, sNchannelsAC> arrHistTimeChID_nPrevEvents{};
  for(const auto &chID: setChIDsSpectraPerBC) {
    if(chID>=sNchannelsAC) {
      std::cout<<"\nWarning! Check channelID set! Cannot find chID "<<chID<<std::endl;
      continue;
    }
    //Time
    {
      std::string name =  "hTimePerBC_ch" + std::to_string(chID);
      std::string title = "Time ch" + std::to_string(chID) + " per BC, (collsion BC, vertex trigger, good PM bits);BC;Time [TDC]";
      TH2F *hist = new TH2F(name.c_str(),title.c_str(),sNBC,0,sNBC, 200,-100,100);
      arrHistTimeChID_PerBC[chID] = hist;
    }
    //Amp
    {
      std::string name =  "hAmpPerBC_ch" + std::to_string(chID);
      std::string title = "Amp ch" + std::to_string(chID) + " per BC, (collsion BC, vertex trigger, good PM bits);BC;Time [TDC]";
      TH2F *hist = new TH2F(name.c_str(),title.c_str(),sNBC,0,sNBC, 200,0,200);
      arrHistAmpChID_PerBC[chID] = hist;
    }
    //Time per n events before
    {
      std::string name =  "hTimeVsNprevEvents_ch" + std::to_string(chID);
      std::string title = "Time ch" + std::to_string(chID) + " vs number of previous events within orbit, (collsion BC, vertex trigger, good PM bits);N previous events;Time [TDC]";
      TH2F *hist = new TH2F(name.c_str(),title.c_str(),100,0,100, 200,-100,100);
      arrHistTimeChID_nPrevEvents[chID] = hist;
    }
    // Distinguishing sides
    if(chID<sNchannelsA) {
      listOutputTimeA_BC->Add(arrHistTimeChID_PerBC[chID]);
      listOutputAmpA_BC->Add(arrHistAmpChID_PerBC[chID]);
      listOutputTimeA_nPrevEvents->Add(arrHistTimeChID_nPrevEvents[chID]);
    }
    else {
      listOutputTimeC_BC->Add(arrHistTimeChID_PerBC[chID]);
      listOutputAmpC_BC->Add(arrHistAmpChID_PerBC[chID]);
      listOutputTimeC_nPrevEvents->Add(arrHistTimeChID_nPrevEvents[chID]);
    }
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
        orbitMin=std::min(orbitMin,orbit);
        orbitMax=std::max(orbitMax,orbit);
        prevOrbit = orbit;
        if(isNewOrbit) {
          clearMapChID2BC_FT0();
        }
        std::vector<uint8_t> vecTrgActivated{};
        for(int i=0;i<8;i++)  {
          if(trgBits & (1<<i)) {
            vecTrgActivated.push_back(i);
          }
        }

        for(const auto &channelData: channels) {
          //Iterating over ChannelData(PM data) per given Event(Digit)
          //VARIABLEES TO USE
          const auto &amp = channelData.QTCAmpl;
          const auto &time = channelData.CFDTime;
          const auto &chID = channelData.ChId;
          const auto &pmBits = channelData.ChainQTC;
          if(chID>=sNchannelsAC) continue;
          const double timePs = time * 13.02;
          const bool isPMbitsGood = ((pmBits & pmBitsToCheck) == pmBitsGood);
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
          if(std::abs(time)<sOrGate && isPMbitsGood) {
            if(isEventVertex&&isCollision && bitsetChIDsSpectraPerBC.test(chID)) {
              if(checkPrevBC_FT0(chID,bc,1)) {
                arrHistTimeChID_PerBC[chID]->Fill(bc,time);
                arrHistAmpChID_PerBC[chID]->Fill(bc,amp);
              }
              arrHistTimeChID_nPrevEvents[chID]->Fill(arrMapChID2BC_FT0[chID].count(),time);

              if(bc>500 && bc<2140) {
//                arrHistTimeChID_nPrevEvents[chID]->Fill(arrPrevBC2ChID_FT0[chID],time);
                arrPrevBC2ChID_FT0[chID]++;
              }

            }
          }

        }
      }
    }
    delete treeInput;
    fileInput.Close();
  }
  //Orbit stats
  std::cout<<"\nOrbits: ("<<orbitMin<<", "<<orbitMax<<")\n";
  //Writing data
  for(const auto &entry: mapOutput2Store) {
    const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(entry.second,entry.first);
  }
  delete listGarbage;
  std::cout<<std::endl;
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
