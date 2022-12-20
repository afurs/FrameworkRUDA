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


#include "readCSV.h"
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
void writeResult(TList *listOutput, const std::string &filepathOutput);

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
  //const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
  //ptrGRPLHCIFData->print();
  //collBC=bunchFilling.getBCPattern();
  //return collBC;
}

void processDigits(unsigned int runnum, const std::string &filepathSrc,const std::string &filepathOutput,const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData, bool uploadFromCCDB, const std::array<double,208> &calib);

/*
void runAnalysis(unsigned int runnum, const std::string &pathToSrc = "/data/work/run3/digits/production",const std::string &pathToDst="") {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  const auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(pathToSrc);
  auto it = mapRunToFilepaths.find(runnum);
  if(it != mapRunToFilepaths.end()) {
    const std::string outputPath = pathToDst+"hists"+std::to_string(runnum)+".root";
    const auto &vecPaths = it->second;
    const std::string &inputPath = vecPaths[0];
    processDigits(runnum,inputPath,outputPath);
  }
  else {
    std::cout<<"\nWARNING! CANNOT FIND FILE RELATED TO RUN "<<runnum<<" AT PATH "<<pathToSrc<<std::endl;
  }
}
*/
void runAnalysisFull(const std::string &pathToSrc = "/data/work/run3/digits/production",const std::string &pathToOutput = "hists", bool applyTimeCalib=false) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //
//  std::map<unsigned int, Entry> mapCalibOffset= getMapCSV("tableCalibOffsets.csv");
    std::map<unsigned int, Entry> mapCalibOffset= getMapCSV_3("tableCalibOffsets_run523144.csv");
  const auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(pathToSrc);
  const std::size_t nParallelJobs=10;
  ROOT::TProcessExecutor pool(nParallelJobs);
  struct Parameters {
    unsigned int runnum{};
    std::string filepathInput{};
    std::string filepathOutput{};
    const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData;
    std::array<double,208> calibOffset{};
  };
  std::vector<Parameters> vecParams{};
  for(const auto &entry: mapRunToFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    if(mapCalibOffset.size()!=0 && mapCalibOffset.find(runnum)==mapCalibOffset.end()) {
      std::cout<<"\n No calib offsets for run "<<runnum<<", skipping...\n";
      continue;
    }
//    if(runnum>=521553) continue;
    const std::string outputFile = pathToOutput+"/"+"hist"+std::to_string(runnum)+".root";
    std::cout<<"\nPreparing run "<<runnum<<std::endl;
    std::bitset<sNBC> collBC{};
    const auto ptrGRPLHCIFData = getGRPLHCIFData(runnum);
    for(const auto &filepath: vecFilepaths) {

      const std::string filenameFormat = "o2_ft0digits_%llu_%llu.root";
      const auto filename = Utils::getFilename(filepath);
      uint64_t buf{0};
      uint64_t chunk{0};
      std::sscanf(filename.c_str(),filenameFormat.c_str(),&buf,&chunk);

      const std::string outputFile = pathToOutput+"/"+"hist"+std::to_string(runnum)+"_"+Form("%02i",chunk)+".root";
      std::cout<<"\nPreparing file "<<filepath<<" into output file "<<outputFile<<" chunk "<<chunk<<std::endl;
      std::array<double,208> arrZeros{0};
      if(applyTimeCalib) {
        vecParams.push_back({runnum,filepath,outputFile,ptrGRPLHCIFData,mapCalibOffset.find(runnum)->second.mMapChunk2Offset.find(chunk)->second});
      }
      else {
        vecParams.push_back({runnum,filepath,outputFile,ptrGRPLHCIFData,arrZeros});
      }
    }
//    vecParams.push_back({runnum,filepath,outputFile,nullptr});

//
//      vecParams.push_back({runnum,filepath,outputFile,collBC});
//    processDigits(runnum,filepath,outputFile);
  }

  const auto result = pool.Map([](const Parameters &entry) {
          processDigits(entry.runnum,entry.filepathInput,entry.filepathOutput,entry.ptrGRPLHCIFData,false,entry.calibOffset);
          return 0;}
          , vecParams);
}

void processDigits(unsigned int runnum, const std::string &filepathSrc, const std::string &filepathOutput,const o2::parameters::GRPLHCIFData *ptrGRPLHCIFData, bool uploadFromCCDB, const std::array<double,208> &calib)
{
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
/*
  for(const auto &en: mapFEE2hash) {
    std::cout<<std::endl<<en.first<<"|"<<static_cast<int>(en.second);
  }
  std::cout<<std::endl;
  for(const auto &en: mChID2PMhash) {
    std::cout<<std::endl<<static_cast<int>(en);
  }
*/
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
  //Constants
  const std::string treeName="o2sim";
  std::map<int,float> mapChID2Amp;
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
  /*
  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);
  */
  std::bitset<sNchannelsAC> chIDA_toTurnOff{};
  //chIDA_toTurnOff.set(72);
  //chIDA_toTurnOff.set(73);
  //chIDA_toTurnOff.set(74);
  //chIDA_toTurnOff.set(75);
  
  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);

  uint8_t pmBitsToCheck = pmBitsGood | pmBitsBad; //All except kNumberADC
  std::cout<<"\nGOOD PM BITS: "<<static_cast<int>(pmBitsGood);
  std::cout<<"\nBAD PM BITS: "<<static_cast<int>(pmBitsBad);
  std::cout<<"\nCHECK PM BITS: "<<static_cast<int>(pmBitsToCheck);

  //
  std::array<int,sNchannelsFT0> arrAmp{};
  std::array<int,sNchannelsFT0> arrTime{};
  //const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
  //ptrGRPLHCIFData->print();
  //collBC=bunchFilling.getBCPattern();

  std::bitset<sNBC> collBC{}, collBC_A{}, collBC_C{}, collBC_E{};
  if(ptrGRPLHCIFData==nullptr && uploadFromCCDB==true) {
    const auto *ptrGRPLHCIFData_tmp = getGRPLHCIFData(runnum);
    const auto &bunchFilling = ptrGRPLHCIFData_tmp->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }
  else if(ptrGRPLHCIFData!=nullptr) {
    const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
    collBC = bunchFilling.getBCPattern();
    collBC_A = bunchFilling.getBeamPattern(0);
    collBC_C = bunchFilling.getBeamPattern(1);
  }
  //Garbage list
  TList *listGarbage = new TList();
  listGarbage->SetOwner(true);
  listGarbage->SetName("output");
  //Output lists
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  listGarbage->Add(listOutput);

  TList *listOutputAmpBC = new TList();
  listOutputAmpBC->SetOwner(true);
  listOutputAmpBC->SetName("output");
  listGarbage->Add(listOutputAmpBC);

  TList *listOutputTimeBC = new TList();
  listOutputTimeBC->SetOwner(true);
  listOutputTimeBC->SetName("output");
  listGarbage->Add(listOutputTimeBC);
  //Amp
  const std::string filepathOutputAmpBC=filepathNoExt+std::string{"_AmpBC.root"};

  TList *listSpectraAmp = new TList();
  listSpectraAmp->SetName("listSpectraAmp");
  listSpectraAmp->SetOwner(true);
  listOutputAmpBC->Add(listSpectraAmp);

  TList *listAmpPerChIDPerBC = new TList();
  listAmpPerChIDPerBC->SetName("listAmpPerChIDPerBC");
  listAmpPerChIDPerBC->SetOwner(true);
  listOutputAmpBC->Add(listAmpPerChIDPerBC);

  TList *listAmpReflection = new TList();
  listAmpReflection->SetName("listAmpReflection");
  listAmpReflection->SetOwner(true);
  listOutputAmpBC->Add(listAmpReflection);
  //Time
  const std::string filepathOutputTimeBC=filepathNoExt+std::string{"_TimeBC.root"};

  TList *listSpectraTime = new TList();
  listSpectraTime->SetName("listSpectraTime");
  listSpectraTime->SetOwner(true);
  listOutputTimeBC->Add(listSpectraTime);

  TList *listTimePerChIDPerBC = new TList();
  listTimePerChIDPerBC->SetName("listTimePerChIDPerBC");
  listTimePerChIDPerBC->SetOwner(true);
  listOutputTimeBC->Add(listTimePerChIDPerBC);

  TList *listTimeReflection = new TList();
  listTimeReflection->SetName("listTimeReflection");
  listTimeReflection->SetOwner(true);
  listOutputTimeBC->Add(listTimeReflection);
  //Triggers
  TList *listTriggers = new TList();
  listTriggers->SetName("listTriggers");
  listTriggers->SetOwner(true);
  listOutput->Add(listTriggers);

  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  // FEE
  TH2F *hBCvsFEE = new TH2F("hBCvsFEE","BC vs FT0 FEE modules;BC;FEE modules",sNBC,0,sNBC,mapFEE2hash.size(), 0, mapFEE2hash.size());
  listOutput->Add(hBCvsFEE);

  TH2F *hBCvsFEE_inColl = new TH2F("hBCvsFEE_inColl","BC(in collision) vs FT0 FEE modules;BC;FEE modules",sNBC,0,sNBC,mapFEE2hash.size(), 0, mapFEE2hash.size());
  listOutput->Add(hBCvsFEE_inColl);

  TH2F *hBCvsFEE_outColl = new TH2F("hBCvsFEE_outColl","BC(out of collision) vs FT0 FEE modules;BC;FEE modules",sNBC,0,sNBC,mapFEE2hash.size(), 0, mapFEE2hash.size());
  listOutput->Add(hBCvsFEE_outColl);

  for (const auto& entry : mapFEE2hash) {
    hBCvsFEE->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    hBCvsFEE_inColl->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    hBCvsFEE_outColl->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
  }

  TH2F *hAmpPerChID = new TH2F("hAmpPerChannel","Amplitude per chID(only good PM bits);chID;Amp",sNchannelsFT0,0,sNchannelsFT0,4100,-100,4000);
  listOutput->Add(hAmpPerChID);

  TH2F *hAmpPerChID_cfdInADC = new TH2F("hAmpPerChannel_cfdInADC","Amplitude per chID(only CFDinADCgate PM cut);chID;Amp",sNchannelsFT0,0,sNchannelsFT0,4100,-100,4000);
  listOutput->Add(hAmpPerChID_cfdInADC);

  TH2F *hAmpPerChID_noCuts = new TH2F("hAmpPerChannel_noCuts","Amplitude per chID(no cuts);chID;Amp",sNchannelsFT0,0,sNchannelsFT0,4100,-100,4000);
  listOutput->Add(hAmpPerChID_noCuts);

/*
  TH2F *hAmpPerBC = new TH2F("hAmpPerBC","Amplitude per BC;BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listSpectraAmp->Add(hAmpPerBC);

  TH2F *hAmpPerBC_sideA = new TH2F("hAmpPerBC_sideA","Amp per BC(side A);BC;Amp",sNBC,0,sNBC,4000,-2000,2000);
  listSpectraAmp->Add(hAmpPerBC_sideA);

  TH2F *hAmpPerBC_sideC = new TH2F("hAmpPerBC_sideC","Amp per BC(side C);BC;Amp",sNBC,0,sNBC,4000,-2000,2000);
  listSpectraAmp->Add(hAmpPerBC_sideC);
*/
  TH2F *hAmpADC0_BC_sideA = new TH2F("hAmpADC0_BC_sideA","Amp(ADC 0) per BC(side A);BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listSpectraAmp->Add(hAmpADC0_BC_sideA);

  TH2F *hAmpADC1_BC_sideA = new TH2F("hAmpADC1_BC_sideA","Amp(ADC 1) per BC(side A);BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listSpectraAmp->Add(hAmpADC1_BC_sideA);

  TH2F *hAmpADC0_BC_sideC = new TH2F("hAmpADC0_BC_sideC","Amp(ADC 0) per BC(side C);BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listSpectraAmp->Add(hAmpADC0_BC_sideC);

  TH2F *hAmpADC1_BC_sideC = new TH2F("hAmpADC1_BC_sideC","Amp(ADC 1) per BC(side C);BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listSpectraAmp->Add(hAmpADC1_BC_sideC);

  TH2F *hAmpSumA_BC = new TH2F("hAmpSumA_BC","Amp sum per BC(side A);BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumA_BC);

  TH2F *hAmpSumC_BC = new TH2F("hAmpSumC_BC","Amp sum per BC(side C);BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumC_BC);

  TH2F *hAmpSum_BC = new TH2F("hAmpSum_BC","Amp sum per BC;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSum_BC);

  TH2F *hAmpSumA_BC_maskA = new TH2F("hAmpSumA_BC_maskA","Amp sum per BC(side A), maskA;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumA_BC_maskA);

  TH2F *hAmpSumC_BC_maskA = new TH2F("hAmpSumC_BC_maskA","Amp sum per BC(side C), maskA;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumC_BC_maskA);

  TH2F *hAmpSum_BC_maskA = new TH2F("hAmpSum_BC_maskA","Amp sum per BC, maskA;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSum_BC_maskA);

  TH2F *hAmpSumA_BC_maskC = new TH2F("hAmpSumA_BC_maskC","Amp sum per BC(side A), maskC;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumA_BC_maskC);

  TH2F *hAmpSumC_BC_maskC = new TH2F("hAmpSumC_BC_maskC","Amp sum per BC(side C), maskC;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumC_BC_maskC);

  TH2F *hAmpSum_BC_maskC = new TH2F("hAmpSum_BC_maskC","Amp sum per BC, maskC;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSum_BC_maskC);

  TH2F *hAmpSumA_BC_maskE = new TH2F("hAmpSumA_BC_maskE","Amp sum per BC(side A), maskE;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumA_BC_maskE);

  TH2F *hAmpSumC_BC_maskE = new TH2F("hAmpSumC_BC_maskE","Amp sum per BC(side C), maskE;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSumC_BC_maskE);

  TH2F *hAmpSum_BC_maskE = new TH2F("hAmpSum_BC_maskE","Amp sum per BC, maskE;BC;Amp",sNBC,0,sNBC,1000,0,100000);
  listSpectraAmp->Add(hAmpSum_BC_maskE);




  TH2F *hAmpSumA_vs_Trg = new TH2F("hAmpSumA_vs_Trg","Amp sum side A vs Triggers;SumAmp;Triggers",10000,0,100000,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hAmpSumA_vs_Trg,mMapTrgNames,1);
  listSpectraAmp->Add(hAmpSumA_vs_Trg);

  TH2F *hAmpSumC_vs_Trg = new TH2F("hAmpSumC_vs_Trg","Amp sum side C vs Triggers;SumAmp;Triggers",10000,0,100000,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hAmpSumC_vs_Trg,mMapTrgNames,1);
  listSpectraAmp->Add(hAmpSumC_vs_Trg);

  TH2F *hAmpSum_vs_Trg = new TH2F("hAmpSum_vs_Trg","Amp sum vs Triggers;SumAmp;Triggers",10000,0,100000,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hAmpSum_vs_Trg,mMapTrgNames,1);
  listSpectraAmp->Add(hAmpSum_vs_Trg);

  TH2F *hAmpSumA_SumC = new TH2F("hAmpSumA_SumC","Amp sum side A vs side C;AmpSumA;AmpSumC",2000,0,100000,2000,0,100000);
  listSpectraAmp->Add(hAmpSumA_SumC);

  TH2F *hAmpSumA_SumC_semicent = new TH2F("hAmpSumA_SumC_semicent","Amp sum side A vs side C(semicent);AmpSumA;AmpSumC",2000,0,20000,2000,0,20000);
  listSpectraAmp->Add(hAmpSumA_SumC_semicent);

  TH2F *hAmpSumA_SumC_cent = new TH2F("hAmpSumA_SumC_cent","Amp sum side A vs side C(cent);AmpSumA;AmpSumC",2000,0,20000,2000,0,20000);
  listSpectraAmp->Add(hAmpSumA_SumC_cent);

  TH2F *hAmpSumA_SumC_vertex = new TH2F("hAmpSumA_SumC_vertex","Amp sum side A vs side C(vertex);AmpSumA;AmpSumC",2000,0,20000,2000,0,20000);
  listSpectraAmp->Add(hAmpSumA_SumC_vertex);


  TH2F *hAmpSumA_vs_vertex = new TH2F("hAmpSumA_vs_vertex","Amp sum per BC(side A), mask B;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumA_vs_vertex);

  TH2F *hAmpSumC_vs_vertex = new TH2F("hAmpSumC_vs_vertex","Amp sum per BC(side C), mask B;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumC_vs_vertex);

  TH2F *hAmpSum_vs_vertex = new TH2F("hAmpSum_vs_vertex","Amp sum per BC, mask B;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSum_vs_vertex);

  TH2F *hAmpSumA_vs_vertex_maskA = new TH2F("hAmpSumA_vs_vertex_maskA","Amp sum per BC(side A), mask A;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumA_vs_vertex_maskA);

  TH2F *hAmpSumC_vs_vertex_maskA = new TH2F("hAmpSumC_vs_vertex_maskA","Amp sum per BC(side C), mask A;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumC_vs_vertex_maskA);

  TH2F *hAmpSum_vs_vertex_maskA = new TH2F("hAmpSum_vs_vertex_maskA","Amp sum per BC, mask A;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSum_vs_vertex_maskA);

  TH2F *hAmpSumA_vs_vertex_maskC = new TH2F("hAmpSumA_vs_vertex_maskC","Amp sum per BC(side A), mask C;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumA_vs_vertex_maskC);

  TH2F *hAmpSumC_vs_vertex_maskC = new TH2F("hAmpSumC_vs_vertex_maskC","Amp sum per BC(side C), mask C;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumC_vs_vertex_maskC);

  TH2F *hAmpSum_vs_vertex_maskC = new TH2F("hAmpSum_vs_vertex_maskC","Amp sum per BC, mask C;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSum_vs_vertex_maskC);

  TH2F *hAmpSumA_vs_vertex_maskE = new TH2F("hAmpSumA_vs_vertex_maskE","Amp sum per BC(side A), mask E;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumA_vs_vertex_maskE);

  TH2F *hAmpSumC_vs_vertex_maskE = new TH2F("hAmpSumC_vs_vertex_maskE","Amp sum per BC(side C), mask E;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSumC_vs_vertex_maskE);

  TH2F *hAmpSum_vs_vertex_maskE = new TH2F("hAmpSum_vs_vertex_maskE","Amp sum per BC, mask E;Vertex[cm];AmpSum",6000,-300,300,2000,0,100000);
  listOutput->Add(hAmpSum_vs_vertex_maskE);


  //Time
  TH2F *hTimePerChID = new TH2F("hTimePerChannel","Time per chID;chID;Time",sNchannelsFT0,0,sNchannelsFT0,4000,-2000,2000);
  listOutput->Add(hTimePerChID);

  TH2F *hTimePerChID_ps = new TH2F("hTimePerChannel_ps","Time per chID;chID;Time [ps]",sNchannelsFT0,0,sNchannelsFT0,4000,-26000,26000);
  listOutput->Add(hTimePerChID_ps);

/*
  TH2F *hTimePerChID_goodPMbits = new TH2F("hTimePerChannel_goodPMbits","Time per chID(IsCFDinADCgate && IsEventInTVDC, no bad PM bits);chID;Time",sNchannelsFT0,0,sNchannelsFT0,4000,-2000,2000);
  listOutput->Add(hTimePerChID_goodPMbits);
*/
  TH2F *hTimePerChIDoffsets = new TH2F("hTimePerChannelOffsets","Time per chID(amp>4 && abs(time)<153 && only good PM bits && vertexTrg);chID;Time",sNchannelsFT0,0,sNchannelsFT0,4000,-2000,2000);
  listOutput->Add(hTimePerChIDoffsets);

  TH2F *hTimePerChIDoffsets_ps = new TH2F("hTimePerChannelOffsets_ps","Time per chID(amp>4 && abs(time)<153 && only good PM bits && vertexTrg);chID;Time [ps]",sNchannelsFT0,0,sNchannelsFT0,4000,-26000,26000);
  listOutput->Add(hTimePerChIDoffsets_ps);


  TH2F *hTimePerBC = new TH2F("hTimePerBC","Time per BC;BC;Time",sNBC,0,sNBC,4000,-2000,2000);
  listSpectraTime->Add(hTimePerBC);

  TH2F *hTimePerBC_sideA = new TH2F("hTimePerBC_sideA","Time per BC(side A);BC;Time",sNBC,0,sNBC,4000,-2000,2000);
  listSpectraTime->Add(hTimePerBC_sideA);

  TH2F *hTimePerBC_sideC = new TH2F("hTimePerBC_sideC","Time per BC(side C);BC;Time",sNBC,0,sNBC,4000,-2000,2000);
  listSpectraTime->Add(hTimePerBC_sideC);

  TH2F *hTimePerBC_ps = new TH2F("hTimePerBC_ps","Time per BC;BC;Time [ps]",sNBC,0,sNBC,4000,-26000,26000);
  listSpectraTime->Add(hTimePerBC_ps);

  TH2F *hTimePerBC_sideA_ps = new TH2F("hTimePerBC_sideA_ps","Time per BC(side A);BC;Time [ps]",sNBC,0,sNBC,4000,-26000,26000);
  listSpectraTime->Add(hTimePerBC_sideA_ps);

  TH2F *hTimePerBC_sideC_ps = new TH2F("hTimePerBC_sideC_ps","Time per BC(side C);BC;Time [ps]",sNBC,0,sNBC,4000,-26000,26000);
  listSpectraTime->Add(hTimePerBC_sideC_ps);

  std::set<unsigned int> setChIDsSpectraPerBC = {41,57,73,89};
  std::bitset<sNchannelsAC> bsActivatedChIDs{};
  std::array<TH2F*, sNchannelsAC> arrHistTimeChID_PerBC{};
  std::array<TH2F*, sNchannelsAC> arrHistAmpChID_PerBC_adc0{};
  std::array<TH2F*, sNchannelsAC> arrHistAmpChID_PerBC_adc1{};

  std::array<TH2F*, sNchannelsAC> arrHistAmpChID_PerBC_adc0_noCuts{};
  std::array<TH2F*, sNchannelsAC> arrHistAmpChID_PerBC_adc1_noCuts{};

  for(const auto &chID: setChIDsSpectraPerBC) {
    if(chID>=sNchannelsAC) {
      std::cout<<"\nWarning! Check channelID set! Cannot find chID "<<chID<<std::endl;
      continue;
    }
    bsActivatedChIDs.set(chID);
    arrHistTimeChID_PerBC[chID] = new TH2F(Form("hTimePerBC_ch%i",chID),Form("Time ch%i per BC;BC;Time",chID),sNBC,0,sNBC,400,-2000,2000);
    listTimePerChIDPerBC->Add(arrHistTimeChID_PerBC[chID]);

    arrHistAmpChID_PerBC_adc0[chID] = new TH2F(Form("hAmpPerBC_ch%i_adc0",chID),Form("Amp(ADC 0) ch%i per BC;BC;Amp",chID),sNBC,0,sNBC,500,0,500);
    listAmpPerChIDPerBC->Add(arrHistAmpChID_PerBC_adc0[chID]);

    arrHistAmpChID_PerBC_adc1[chID] = new TH2F(Form("hAmpPerBC_ch%i_adc1",chID),Form("Amp(ADC 1) ch%i per BC;BC;Amp",chID),sNBC,0,sNBC,500,0,500);
    listAmpPerChIDPerBC->Add(arrHistAmpChID_PerBC_adc1[chID]);

//    arrHistAmpChID_PerBC_adc0_noCuts[chID] = new TH2F(Form("hAmpPerBC_ch%i_adc0_noCuts",chID),Form("Amp(ADC 0) ch%i per BC(no Cuts);BC;Amp",chID),sNBC,0,sNBC,500,0,500);
//    listAmpPerChIDPerBC->Add(arrHistAmpChID_PerBC_adc0_noCuts[chID]);

//    arrHistAmpChID_PerBC_adc1_noCuts[chID] = new TH2F(Form("hAmpPerBC_ch%i_adc1_noCuts",chID),Form("Amp(ADC 1) ch%i per BC(no Cuts);BC;Amp",chID),sNBC,0,sNBC,500,0,500);
//    listAmpPerChIDPerBC->Add(arrHistAmpChID_PerBC_adc1_noCuts[chID]);

  }
  // BC
  TH1F *hBC = new TH1F("hBC","BC distribution;BC;Time",sNBC,0,sNBC);
  listOutput->Add(hBC);

  //Collision time and vertex
  TH1F *hCollisionTime = new TH1F("hCollisionTime","Collision time;Collision time [ns]",1000,-5,5);
  listOutput->Add(hCollisionTime);
  TH1F *hCollisionTime_vrtTrg = new TH1F("hCollisionTime_vrtTrg","Collision time(vertex trigger);Collision time [ns]",1000,-5,5);
  listOutput->Add(hCollisionTime_vrtTrg);
  TH1F *hCollisionTime_noVrtTrg = new TH1F("hCollisionTime_noVrtTrg","Collision time(w/o vertex trigger);Collision time [ns]",1000,-5,5);
  listOutput->Add(hCollisionTime_noVrtTrg);

  TH1F *hVertexNoCut = new TH1F("hVertexNoCut","Vertex position(w/o cuts);Vertex [cm]",2000,-200,200);
  listOutput->Add(hVertexNoCut);

  TH1F *hVertex = new TH1F("hVertex","Vertex position;Vertex [cm]",600,-30,30);
  listOutput->Add(hVertex);
  TH1F *hVertex_vrtTrg = new TH1F("hVertex_vrtTrg","Vertex position (vertex trigger);Vertex [cm]",600,-30,30);
  listOutput->Add(hVertex_vrtTrg);
  TH1F *hVertex_noVrtTrg = new TH1F("hVertex_noVrtTrg","Vertex position (w/o vertex trigger);Vertex [cm]",600,-30,30);
  listOutput->Add(hVertex_noVrtTrg);

//  TH2F *hCollisionTimeVsVertex = new TH2F("hCollisionTimeVsVertex","Collision time vs Vertex position;Vertex [cm];Collision time [ns]",600,-30,30,400,-5,5);


  TH2F *hCollisionTimeVsVertex_noCut = new TH2F("hCollisionTimeVsVertex_noCut","Collision time vs Vertex position(w/o cuts);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut);

  TH2F *hCollisionTimeVsVertex_noCut_inColl = new TH2F("hCollisionTimeVsVertex_noCut_inColl","Collision time vs Vertex position(w/o cuts, in collision);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_inColl);

  TH2F *hCollisionTimeVsVertex_noCut_maskA = new TH2F("hCollisionTimeVsVertex_noCut_maskA","Collision time vs Vertex position(w/o cuts, maskA);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_maskA);

  TH2F *hCollisionTimeVsVertex_noCut_maskC = new TH2F("hCollisionTimeVsVertex_noCut_maskC","Collision time vs Vertex position(w/o cuts, maskC);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_maskC);

  TH2F *hCollisionTimeVsVertex_noCut_maskE = new TH2F("hCollisionTimeVsVertex_noCut_maskE","Collision time vs Vertex position(w/o cuts, maskE);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_maskE);

  TH2F *hCollisionTimeVsVertex_noCut_vertex = new TH2F("hCollisionTimeVsVertex_noCut_vertex","Collision time vs Vertex position(w/o cuts, vertex);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_vertex);

  TH2F *hCollisionTimeVsVertex_central = new TH2F("hCollisionTimeVsVertex_central","Collision time vs Vertex position(central trigger);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_central);

  TH2F *hCollisionTimeVsVertex_semicentral = new TH2F("hCollisionTimeVsVertex_semicentral","Collision time vs Vertex position(semicentral trigger);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_semicentral);


  TH2F *hCollisionTimeVsVertex_noCut_noVertex = new TH2F("hCollisionTimeVsVertex_noCut_noVertex","Collision time vs Vertex position(w/o cuts, w/o vertex trigger);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_noVertex);


  TH2F *hCollisionTimeVsVertex_noCut_highAmpSumA = new TH2F("hCollisionTimeVsVertex_noCut_highAmpSumA","Collision time vs Vertex position(w/o cuts, ampSumA>6000);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_highAmpSumA);

  TH2F *hCollisionTimeVsVertex_noCut_highAmpSumC = new TH2F("hCollisionTimeVsVertex_noCut_highAmpSumC","Collision time vs Vertex position(w/o cuts, ampSumC>2000);Vertex [cm];Collision time [ns]",6000,-300,300,400,-20,20);
  listOutput->Add(hCollisionTimeVsVertex_noCut_highAmpSumC);

  TH1F *hMeanTimeA_noCut = new TH1F("hMeanTimeA_noCut","Mean time, side A(w/o cuts);time [ps]",4000,-26000,26000);
  listOutput->Add(hMeanTimeA_noCut);
  TH1F *hMeanTimeC_noCut = new TH1F("hMeanTimeC_noCut","Mean time, side C(w/o cuts);time [ps]",4000,-26000,26000);
  listOutput->Add(hMeanTimeC_noCut);

  TH1F *hMeanTimeA = new TH1F("hMeanTimeA","Mean time, side A;time [ps]",4000,-26000,26000);
  listOutput->Add(hMeanTimeA);
  TH1F *hMeanTimeC = new TH1F("hMeanTimeC","Mean time, side C;time [ps]",4000,-26000,26000);
  listOutput->Add(hMeanTimeC);



  TH2F *hOrAVsOrC_noCut = new TH2F("hOrAVsOrC_noCut","OrA time vs OrC time (w/o cuts);OrC [channel];OrA time [channel]",1000,-1000,1000,1000,-1000,1000);
  listOutput->Add(hOrAVsOrC_noCut);


  TH2F *hCollisionTimeVsVertex = new TH2F("hCollisionTimeVsVertex","Collision time vs Vertex position;Vertex [cm];Collision time [ns]",1000,-50,50,1000,-50,50);
  listOutput->Add(hCollisionTimeVsVertex);
  TH2F *hCollisionTimeVsVertex_vrtTrg = new TH2F("hCollisionTimeVsVertex_vrtTrg","Collision time vs Vertex position (vertex trigger);Vertex [cm];Collision time [ns]",600,-30,30,400,-5,5);
  listOutput->Add(hCollisionTimeVsVertex_vrtTrg);
  TH2F *hCollisionTimeVsVertex_noVrtTrg = new TH2F("hCollisionTimeVsVertex_noVrtTrg","Collision time vs Vertex position (w/o vertex trigger);Vertex [cm];Collision time [ns]",600,-30,30,400,-5,5);
  listOutput->Add(hCollisionTimeVsVertex_noVrtTrg);

  TH2F *hVertexVsBC_vrtTrg = new TH2F("hVertexVsBC_vrtTrg","Vertex position vs BC (vertex trigger);BC;Vertex [cm]",sNBC,0,sNBC,600,-30,30);
  listOutput->Add(hVertexVsBC_vrtTrg);

  TH2F *hVertexVsBC_vrtTrg_inColl = new TH2F("hVertexVsBC_vrtTrg_inColl","Vertex position vs BC (vertex trigger, collision BC);BC;Vertex [cm]",sNBC,0,sNBC,600,-30,30);
  listOutput->Add(hVertexVsBC_vrtTrg_inColl);

  TH2F *hVertexVsBC_noVrtTrg = new TH2F("hVertexVsBC_noVrt","Vertex position vs BC (w/o vertex trigger);BC;Vertex [cm]",sNBC,0,sNBC,600,-30,30);
  listOutput->Add(hVertexVsBC_noVrtTrg);


  //
  TH1F *hTriggers = new TH1F("hTriggers","Triggers",mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggers,mMapTrgNames,0);
  listTriggers->Add(hTriggers);
  TH2F *hTriggersCorr = new TH2F("hTriggersCorr","Trigger correlations",mMapTrgNames.size(),0,mMapTrgNames.size(),mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersCorr,mMapTrgNames,0);
  HistHelper::makeHistBinNamed(hTriggersCorr,mMapTrgNames,1);
  listTriggers->Add(hTriggersCorr);

  TH2F *hTriggersVsBC = new TH2F("hTriggersVsBC","Triggers vs BC;BC;Triggers",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersVsBC,mMapTrgNames,1);
  listTriggers->Add(hTriggersVsBC);

  TH2F *hTriggersVsBC_inColl = new TH2F("hTriggersVsBC_inColl","Triggers vs BC(in collision BC);BC;Triggers",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersVsBC_inColl,mMapTrgNames,1);
  listTriggers->Add(hTriggersVsBC_inColl);

  TH2F *hTriggersVsBC_outColl = new TH2F("hTriggersVsBC_outColl","Triggers vs BC(out of collision BC);BC;Triggers",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersVsBC_outColl,mMapTrgNames,1);
  listTriggers->Add(hTriggersVsBC_outColl);


  TH1F *hBC_trgVrt = new TH1F("hBC_trgVrt","BC (Vertex trigger);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_trgVrt);

  TH1F *hNChanA = new TH1F("hNChanA","Number of channels side A;nChannels",sNchannelsA,0,sNchannelsA);
  listOutput->Add(hNChanA);

  TH1F *hNChanC = new TH1F("hNChanC","Number of channels side C;nChannels",sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanC);

  TH2F *hNChanAvsC = new TH2F("hNChanAvsC","Number of channels side A vs C;nChannelsA;nChannelsC",sNchannelsC,0,sNchannelsC,sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanAvsC);

  TH2F *hNChanAvsC_vrtTrg = new TH2F("hNChanAvsC_vrtTrg","Number of channels side A vs C(vertex trigger);nChannelsA;nChannelsC",sNchannelsC,0,sNchannelsC,sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanAvsC_vrtTrg);

  TH1F *hNChannels = new TH1F("hNChannels","Number of channels;nChannels",sNchannelsFT0,0,sNchannelsFT0);
  listOutput->Add(hNChannels);


  TH2F *hNChanA_vertex = new TH2F("hNChanA_vertex","Vertex vs Number of channels side A;Vertex [cm];nChannels",2000,-100,100,sNchannelsA,0,sNchannelsA);
  listOutput->Add(hNChanA_vertex);

  TH2F *hNChanC_vertex = new TH2F("hNChanC_vertex","Vertex vs Number of channels side C;Vertex [cm];nChannels",2000,-100,100,sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanC_vertex);

  TH2F *hNChan_vertex = new TH2F("hNChan_vertex","Vertex vs Number of channels;Vertex [cm];nChannels",2000,-100,100,sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChan_vertex);

  //Amp
  TH2F *hAmpAfterCollision = new TH2F("hAmpAfterCollision","Amplitudes after collision BC;BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listAmpReflection->Add(hAmpAfterCollision);

  TH2F *hAmpCollBC_afterCollision = new TH2F("hAmpCollBC_afterCollision","Amplitudes in collision BC vs next BC(closest to collision BC in given channel);Next BC;Amp(Collision BC)",sNBC,0,sNBC,4100,-100,4000);
  listAmpReflection->Add(hAmpCollBC_afterCollision);

  TH2F *hAmpCollBC_afterCollision_sideA = new TH2F("hAmpCollBC_afterCollision_sideA","Amplitudes in collision BC vs next BC(closest to collision BC in given channel) sideA;Next BC;Amp(Collision BC)",sNBC,0,sNBC,4100,-100,4000);
  listAmpReflection->Add(hAmpCollBC_afterCollision_sideA);

  TH2F *hAmpCollBC_afterCollision_sideC = new TH2F("hAmpCollBC_afterCollision_sideC","Amplitudes in collision BC vs next BC(closest to collision BC in given channel) sideC;Next BC;Amp(Collision BC)",sNBC,0,sNBC,4100,-100,4000);
  listAmpReflection->Add(hAmpCollBC_afterCollision_sideC);

  //Time
  TH2F *hTimeAfterCollision = new TH2F("hTimeAfterCollision","Times after collision BC;BC;Amp",sNBC,0,sNBC,4000,-2000,2000);
  listTimeReflection->Add(hTimeAfterCollision);

  TH2F *hTimeAfterCollision_sideA = new TH2F("hTimeAfterCollision_sideA","Times after collision BC, sideA;BC;Amp",sNBC,0,sNBC,4000,-2000,2000);
  listTimeReflection->Add(hTimeAfterCollision_sideA);

  TH2F *hTimeAfterCollision_sideC = new TH2F("hTimeAfterCollision_sideC","Times after collision BC, sideC;BC;Amp",sNBC,0,sNBC,4000,-2000,2000);
  listTimeReflection->Add(hTimeAfterCollision_sideC);

  TH2F *hTimeCollBC_afterCollision = new TH2F("hTimeCollBC_afterCollision","Timees in collision BC vs next BC(closest to collision BC in given channel);Next BC;Time(Collision BC)",sNBC,0,sNBC,4000,-2000,2000);
  listTimeReflection->Add(hTimeCollBC_afterCollision);
/*
  TH2F *hChID_afterCollision = new TH2F("hChID_afterCollision","ChannelID after collision BC;BC;Amp",sNBC,0,sNBC,sNchannelsFT0,0,sNchannelsFT0);
  listOutput->Add(hChID_afterCollision);
*/
/*
  TH2F *hBCvsTrg_OutOfColl = new TH2F("hBCvsTrg_OutOfColl","BC vs Trg, out of collision;BC",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hBCvsTrg_OutOfColl,mMapTrgNames,1);
  listTriggers->Add(hBCvsTrg_OutOfColl);
*/
  TH1F *hBC_VrtTrg_OutOfColl = new TH1F("hBC_VrtTrg_OutOfColl","BC(Vertex Trigger, out of collision)",sNBC,0,sNBC);
  listOutput->Add(hBC_VrtTrg_OutOfColl);
/*
  TH2F *hAmpVsPMbits = new TH2F("hAmpVsPMbits","Amplitude vs PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4100,-100,4000);
  listOutput->Add(hAmpVsPMbits);
  HistHelper::makeHistBinNamed(hAmpVsPMbits,mMapPMbits,0);

  TH2F *hAmpVsNotPMbits = new TH2F("hAmpVsNotPMbits","Amplitude vs negative PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4100,-100,4000);
  listOutput->Add(hAmpVsNotPMbits);
  HistHelper::makeHistBinNamed(hAmpVsNotPMbits,mMapPMbits,0);

  TH2F *hTimeVsPMbits = new TH2F("hTimeVsPMbits","Time vs PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4000,-2000,4000);
  listOutput->Add(hTimeVsPMbits);
  HistHelper::makeHistBinNamed(hTimeVsPMbits,mMapPMbits,0);

  TH2F *hTimeVsNotPMbits = new TH2F("hTimeVsNotPMbits","Time vs negative PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4000,-2000,4000);
  listOutput->Add(hTimeVsNotPMbits);
  HistHelper::makeHistBinNamed(hTimeVsNotPMbits,mMapPMbits,0);
*/


  std::array<TH2F*,sNchannelsAC> arrHistAmpPerChIDPerBC_afterCollission{};
  std::array<TH2F*,sNchannelsAC> arrHistAmpPerChIDPerCollBC_afterCollission{};
  std::set setChID{1,26};
  for(const auto &chID : setChID) {
    TH2F *hist = new TH2F(Form("hAmpBC_afterCollision%i",chID),Form("Amplitude ch%i after collision BC;BC;Amp",chID),sNBC,0,sNBC,4100,-100,4000);
    arrHistAmpPerChIDPerBC_afterCollission[chID]=hist;
    listAmpPerChIDPerBC->Add(hist);

    hist = new TH2F(Form("hAmpCollBC_afterCollision%i",chID),Form("Amplitude ch%i in collision BC vs next BC(closest to collision BC in given channel);Next BC;Amp(Collision BC)",chID),sNBC,0,sNBC,4100,-100,4000);
    arrHistAmpPerChIDPerCollBC_afterCollission[chID]=hist;
    listAmpPerChIDPerBC->Add(hist);
  }

/*
  for(int iCh=0;iCh<1;iCh++) {
    TH2F *hist = new TH2F(Form("hAmpPerBC_ch%i",iCh),Form("Amplitude ch%i per BC;BC;Amp",iCh),sNBC,0,sNBC,4100,-100,4000);
    arrHistAmpPerChIDPerBC[iCh]=hist;
    listAmpPerChIDPerBC->Add(hist);
  }
*/
  TH1F *hBC_OrA_notVrt_collBC = new TH1F("hBC_OrA_notVrt_collBC","BC (only colliding BCs, OrA && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrA_notVrt_collBC);

  TH1F *hBC_OrC_notVrt_collBC = new TH1F("hBC_OrC_notVrt_collBC","BC (only colliding BCs, OrC && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrC_notVrt_collBC);

  TH1F *hBC_OrA_notVrt_nonCollBC = new TH1F("hBC_OrA_notVrt_nonCollBC","BC (only non-colliding BCs, OrA && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrA_notVrt_nonCollBC);

  TH1F *hBC_OrC_notVrt_nonCollBC = new TH1F("hBC_OrC_notVrt_nonCollBC","BC (only non-colliding BCs, OrC && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrC_notVrt_nonCollBC);

  //Counts and efficiency
  TH1F *hTriggersEmu = new TH1F("hTriggersEmu","Triggers(emulation)",mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersEmu,mMapTrgNames,0);
  listTriggers->Add(hTriggersEmu);

  ////
  using DigitFT0 = o2::ft0::Digit;
  using ChannelDataFT0 = o2::ft0::ChannelData;
  std::vector<DigitFT0> vecDigits;
  std::vector<DigitFT0> *ptrVecDigits = &vecDigits;
  std::vector<ChannelDataFT0> vecChannelData;
  std::vector<ChannelDataFT0> *ptrVecChannelData = &vecChannelData;
  std::size_t mCntEvents{};
  std::size_t mNTFs{};
  std::vector<std::string> vecFilenames{};
  vecFilenames.push_back(filepathSrc);// One can parse later line with filelist
  for(const auto &filepathInput:vecFilenames) {
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
      uint32_t orbitOld{};
      std::array<uint16_t, sNchannelsAC> arrChID_lastBC{};
      arrChID_lastBC.fill(0xffff);
      std::array<std::bitset<sNBC>, sNchannelsAC> arrChID_BC{};

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
        
        /*
        const auto &trgBits = digit.mTriggers.triggersignals;
        const auto &nChA = digit.mTriggers.nChanA;
        const auto &nChC = digit.mTriggers.nChanC;
        const auto &sumAmpA = digit.mTriggers.amplA;
        const auto &sumAmpC = digit.mTriggers.amplC;
        const auto &averageTimeA = digit.mTriggers.timeA;
        const auto &averageTimeC = digit.mTriggers.timeC;
*/
        /*
        **PUT HERE CODE FOR PROCESSING DIGITS
        */
//        if(!collBC.test(bc)) continue;
        std::set<uint8_t> setFEEmodules{};
        if(trgBits!=0) setFEEmodules.insert(mTCMhash);
        if(orbit!=orbitOld) {
          //new orbit
          orbitOld=orbit;
          arrChID_lastBC.fill(0xffff); // reset last BC per channel
          std::for_each(arrChID_BC.begin(),arrChID_BC.end(),[](auto &entry) {entry.reset();});
        }
        const double secSinceSOR = 1.*(orbit*sNBC+bc)/sNBCperSec;
        const bool isEventVertex = (trg.getVertex() && trg.getDataIsValid() && !trg.getOutputsAreBlocked());
/*        if(nChA>0 && nChC>0 && trg.getVertex()) {
          const float collTime = (averageTimeA + averageTimeC)/2 * sNSperTimeChannel;
          const float vrtPos = (averageTimeA - averageTimeC)/2 * sNSperTimeChannel * sNS2Cm;
          hCollisionTimeVsVertex->Fill(collTime,vrtPos);
        }
*/
        std::vector<uint8_t> vecPMbits{};
        const bool isCollision = collBC.test(bc);
        const bool isMaskA = collBC_A.test(bc);
        const bool isMaskC = collBC_C.test(bc);
        const bool isMaskE = !(isCollision || isMaskA || isMaskC);
        hBC->Fill(bc);
        std::vector<uint8_t> vecTrgActivated{};
        for(int i=0;i<8;i++)  {
          if(trgBits & (1<<i)) {
            hTriggers->Fill(i);
            hTriggersVsBC->Fill(bc,i);
            vecTrgActivated.push_back(i);
            if(!isCollision) {
              hTriggersVsBC_outColl->Fill(bc,i);
              //hBCvsTrg_OutOfColl->Fill(bc,i);
            }
            else {
              hTriggersVsBC_inColl->Fill(bc,i);
            }
            for(int j=i+1;j<8;j++) {
              if(trgBits & (1<<j)) hTriggersCorr->Fill(i,j);
            }
          }
        }
        std::bitset<sNchannelsFT0> bsChID{};
        int nChanA{};
        int nChanC{};
        double meanTimeA{};
        double meanTimeC{};
        double ampSum_sideA{0};
        double ampSum_sideC{0};

        int nChanA_noCut{};
        int nChanC_noCut{};
        double meanTimeA_noCut{};
        double meanTimeC_noCut{};
        double ampSum_sideA_noCut{0};
        double ampSum_sideC_noCut{0};

        //
        //
        for(const auto &channelData: channels) {
          //Iterating over ChannelData(PM data) per given Event(Digit)
          //VARIABLEES TO USE
          const auto &amp = channelData.QTCAmpl;
          const auto &timeSrc = channelData.CFDTime;
          const auto &chID = channelData.ChId;
          const auto &pmBits = channelData.ChainQTC;

          if(chID>=sNchannelsAC) continue;
          if(std::abs(calib[chID])>(153)) continue;
          const double time = timeSrc - calib[chID];
         // std::cout<<std::endl<<timeSrc<<"|"<<time<<std::endl;
          const double timePs = time * 13.0;
          /*
          **PUT HERE ANALYSIS CODE
          */
          //vecAmpsVsBC[chID]->Fill(bc,amp);
          //vecAmpsTrend[chID]->Fill(secSinceSOR,amp);
          /*
          if(chID==0) {
            arrHistAmpPerChIDPerBC[chID]->Fill(bc,amp);
          }
          arrHistTimeChID_PerBC
          */
          setFEEmodules.insert(mChID2PMhash[chID]);
          if(bsActivatedChIDs.test(chID)) {
            arrHistTimeChID_PerBC[chID]->Fill(bc,time);
            if(pmBits & (1<<ChannelDataFT0::kNumberADC)) {
              arrHistAmpChID_PerBC_adc1[chID]->Fill(bc,amp);
            }
            else {
              arrHistAmpChID_PerBC_adc0[chID]->Fill(bc,amp);
            }
          }
          if(chID<sNchannelsA) {
            nChanA_noCut++;
            meanTimeA_noCut+=time;
            ampSum_sideA_noCut+=amp;
            hTimePerBC_sideA->Fill(bc,time);
            hTimePerBC_sideA_ps->Fill(bc,timePs);
            
            //hAmpPerBC_sideA->Fill(bc,amp);
          }
          else if(chID<sNchannelsAC){
            nChanC_noCut++;
            meanTimeC_noCut+=time;
            ampSum_sideC_noCut+=amp;
            hTimePerBC_sideC->Fill(bc,time);
            hTimePerBC_sideC_ps->Fill(bc,timePs);

            //hAmpPerBC_sideC->Fill(bc,amp);
          }
          hTimePerChID->Fill(chID,time);
          hTimePerChID_ps->Fill(chID,timePs);

          hAmpPerChID_noCuts->Fill(chID,amp);
/*
          if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
          //{
            hAmpPerChID_cfdInADC->Fill(chID,amp);

          }
*/
          if(std::abs(time)<sOrGate && ((pmBits & pmBitsToCheck) == pmBitsGood)) {
          //if(pmBits & (1<<ChannelDataFT0::kIsCFDinADCgate)) {
          //{
            hAmpPerChID_cfdInADC->Fill(chID,amp);
            if(isEventVertex) {
              hTimePerChIDoffsets->Fill(chID,time);
              hTimePerChIDoffsets_ps->Fill(chID,timePs);
            }
            vecPMbits.push_back(pmBits);
            if(chID<sNchannelsA && !chIDA_toTurnOff.test(chID)) {
              nChanA++;
              meanTimeA+=time;
              ampSum_sideA+=amp;
              if(pmBits & (1<<ChannelDataFT0::kNumberADC)) {
                hAmpADC1_BC_sideA->Fill(bc,amp);
              }
              else {
                hAmpADC0_BC_sideA->Fill(bc,amp);
              }
            }
            else if(chID>=sNchannelsA&&chID<sNchannelsAC){
              nChanC++;
              meanTimeC+=time;
              ampSum_sideC+=amp;
              if(pmBits & (1<<ChannelDataFT0::kNumberADC)) {
                hAmpADC1_BC_sideC->Fill(bc,amp);
              }
              else {
                hAmpADC0_BC_sideC->Fill(bc,amp);
              }

            }
          }
          bsChID.set(chID);
/*          if((pmBits & pmBitsToCheck) == pmBitsGood) {
            hTimePerChID_goodPMbits->Fill(chID,time);
          }
*/
          hTimePerBC->Fill(bc,time);
          hTimePerBC_ps->Fill(bc,timePs);
          if((pmBits & pmBitsToCheck) == pmBitsGood) {
            hAmpPerChID->Fill(chID,amp);
            //hAmpPerBC->Fill(bc,amp);
          }
          if(isCollision) {
            arrChID_BC[chID].set(bc);
          }
          
          else if(arrChID_lastBC[chID]!=0xffff) {
            if(collBC.test(arrChID_lastBC[chID])) {
              hAmpAfterCollision->Fill(bc,amp);
              hTimeAfterCollision->Fill(bc,time);
              hAmpCollBC_afterCollision->Fill(bc,arrAmp[chID]);
              hTimeCollBC_afterCollision->Fill(bc,arrTime[chID]);
              if(chID<96) {
                hTimeAfterCollision_sideA->Fill(bc,time);
                hAmpCollBC_afterCollision_sideA->Fill(bc,arrAmp[chID]);
              }
              else if(chID<208) {
                hTimeAfterCollision_sideC->Fill(bc,time);
                hAmpCollBC_afterCollision_sideC->Fill(bc,arrAmp[chID]);
              }

              //hChID_afterCollision->Fill(bc,chID);
              if(setChID.find(chID)!=setChID.end()) {
                arrHistAmpPerChIDPerBC_afterCollission[chID]->Fill(bc,amp);
                arrHistAmpPerChIDPerCollBC_afterCollission[chID]->Fill(bc,arrAmp[chID]);
              }
            }
          }
          
          /*
          for(int i=0;i<8;i++) {
            if(pmBits & (1<<i)) {
              hAmpVsPMbits->Fill(i,amp);
              hTimeVsPMbits->Fill(i,time);
            }
            else {
              hAmpVsNotPMbits->Fill(i,amp);
              hTimeVsNotPMbits->Fill(i,time);
            }
          }
          */
          arrChID_lastBC[chID]=bc;
          arrAmp[chID] = amp;
          arrTime[chID] = time;
          //
        }
        for (const auto& feeHash : setFEEmodules) {
          hBCvsFEE->Fill(static_cast<double>(bc), static_cast<double>(feeHash));
          if(isCollision) {
            hBCvsFEE_inColl->Fill(static_cast<double>(bc), static_cast<double>(feeHash));
          }
          else {
            hBCvsFEE_outColl->Fill(static_cast<double>(bc), static_cast<double>(feeHash));
          }
        }

        if(ampSum_sideA+ampSum_sideC>0) {
          for(const auto &trgPos:vecTrgActivated) {
            hAmpSum_vs_Trg->Fill(ampSum_sideA+ampSum_sideC,trgPos);
          }
          hAmpSumA_SumC->Fill(ampSum_sideA,ampSum_sideC);
          if(trg.getSCen()) hAmpSumA_SumC_semicent->Fill(ampSum_sideA,ampSum_sideC);
          if(trg.getCen()) hAmpSumA_SumC_cent->Fill(ampSum_sideA,ampSum_sideC);
          if(trg.getVertex()) hAmpSumA_SumC_vertex->Fill(ampSum_sideA,ampSum_sideC);
        }
        if(nChanA>0) meanTimeA = meanTimeA/nChanA;
        if(nChanC>0) meanTimeC = meanTimeC/nChanC;
        const double collTime = (meanTimeA + meanTimeC)/2 * sNSperTimeChannel;
        const double vrtPos = (meanTimeC - meanTimeA)/2 * sNSperTimeChannel * sNS2Cm;
        if(nChanA_noCut>0) meanTimeA_noCut = meanTimeA_noCut/nChanA_noCut;
        if(nChanC_noCut>0) meanTimeC_noCut = meanTimeC_noCut/nChanC_noCut;
        const double collTime_noCut = (meanTimeA_noCut + meanTimeC_noCut)/2 * sNSperTimeChannel;
        const double vrtPos_noCut = (meanTimeC_noCut - meanTimeA_noCut)/2 * sNSperTimeChannel * sNS2Cm;
        if(ampSum_sideA>0) {
          hAmpSumA_BC->Fill(bc,ampSum_sideA);
          for(const auto &trgPos:vecTrgActivated) {
            hAmpSumA_vs_Trg->Fill(ampSum_sideA,trgPos);
            if(ampSum_sideC>0) {
              hAmpSum_vs_Trg->Fill(ampSum_sideA+ampSum_sideC,trgPos);
            }
          }
        }

        if(ampSum_sideC>0) {
          hAmpSumC_BC->Fill(bc,ampSum_sideC);
          for(const auto &trgPos:vecTrgActivated) {
            hAmpSumC_vs_Trg->Fill(ampSum_sideC,trgPos);
          }
        }
        if(nChanA_noCut>0 || nChanC_noCut>0) {
          if(isCollision) {
            hAmpSum_BC->Fill(bc,ampSum_sideA_noCut+ampSum_sideC_noCut);
            hAmpSum_vs_vertex->Fill(vrtPos_noCut,ampSum_sideA_noCut+ampSum_sideC_noCut);
            
          }
          else if(isMaskA) {
            hAmpSum_BC_maskA->Fill(bc,ampSum_sideA_noCut+ampSum_sideC_noCut);
            hAmpSum_vs_vertex_maskA->Fill(vrtPos_noCut,ampSum_sideA_noCut+ampSum_sideC_noCut);
          }
          else if(isMaskC){
            hAmpSum_BC_maskC->Fill(bc,ampSum_sideA_noCut+ampSum_sideC_noCut);
            hAmpSum_vs_vertex_maskC->Fill(vrtPos_noCut,ampSum_sideA_noCut+ampSum_sideC_noCut);
          }
          else if(isMaskE){
            hAmpSum_BC_maskE->Fill(bc,ampSum_sideA_noCut+ampSum_sideC_noCut);
            hAmpSum_vs_vertex_maskE->Fill(vrtPos_noCut,ampSum_sideA_noCut+ampSum_sideC_noCut);
          }
        }
        if(nChanA_noCut>0) {
          if(isCollision) {
            hAmpSumA_BC->Fill(bc,ampSum_sideA_noCut);
            hAmpSumA_vs_vertex->Fill(vrtPos_noCut,ampSum_sideA_noCut);
            hMeanTimeA_noCut->Fill(meanTimeA_noCut * 13.02);
          }
          else if(isMaskA){
            hAmpSumA_BC_maskA->Fill(bc,ampSum_sideA_noCut);
            hAmpSumA_vs_vertex_maskA->Fill(vrtPos_noCut,ampSum_sideA_noCut);
          }
          else if(isMaskC){
            hAmpSumA_BC_maskC->Fill(bc,ampSum_sideA_noCut);
            hAmpSumA_vs_vertex_maskC->Fill(vrtPos_noCut,ampSum_sideA_noCut);
          }
          else if(isMaskE){
            hAmpSumA_BC_maskE->Fill(bc,ampSum_sideA_noCut);
            hAmpSumA_vs_vertex_maskE->Fill(vrtPos_noCut,ampSum_sideA_noCut);
          }
        }
        if(nChanC_noCut>0) {
          if(isCollision) {
            hAmpSumC_BC->Fill(bc,ampSum_sideC_noCut);
            hMeanTimeC_noCut->Fill(meanTimeC_noCut * 13.02);
          }
          else if(isMaskA){
            hAmpSumC_BC_maskA->Fill(bc,ampSum_sideC_noCut);
            hAmpSumC_vs_vertex_maskA->Fill(vrtPos_noCut,ampSum_sideC_noCut);
          }
          else if(isMaskC){
            hAmpSumC_BC_maskC->Fill(bc,ampSum_sideC_noCut);
            hAmpSumC_vs_vertex_maskC->Fill(vrtPos_noCut,ampSum_sideC_noCut);
          }
          else if(isMaskE){
            hAmpSumC_BC_maskE->Fill(bc,ampSum_sideC_noCut);
            hAmpSumC_vs_vertex_maskE->Fill(vrtPos_noCut,ampSum_sideC_noCut);
          }
          hAmpSumC_vs_vertex->Fill(vrtPos_noCut,ampSum_sideC_noCut);
        }

        if(nChanA_noCut>0 && nChanC_noCut>0) {
          hVertexNoCut->Fill(vrtPos_noCut);
          hNChan_vertex->Fill(vrtPos_noCut,nChanA_noCut+nChanC_noCut);
          hNChanA_vertex->Fill(vrtPos_noCut,nChanA_noCut);
          hNChanC_vertex->Fill(vrtPos_noCut,nChanC_noCut);

          if(trg.getVertex()) {
            hCollisionTimeVsVertex_noCut_vertex->Fill(vrtPos_noCut,collTime_noCut);
          }
          else {
            hCollisionTimeVsVertex_noCut_noVertex->Fill(vrtPos_noCut,collTime_noCut);
          }
          if(trg.getCen())hCollisionTimeVsVertex_central->Fill(vrtPos_noCut,collTime_noCut);
          if(trg.getSCen())hCollisionTimeVsVertex_semicentral->Fill(vrtPos_noCut,collTime_noCut);
          if(isCollision) {
            hCollisionTimeVsVertex_noCut_inColl->Fill(vrtPos_noCut,collTime_noCut);
          }
          else if(isMaskA){
            hCollisionTimeVsVertex_noCut_maskA->Fill(vrtPos_noCut,collTime_noCut);
          }
          else if(isMaskC){
            hCollisionTimeVsVertex_noCut_maskC->Fill(vrtPos_noCut,collTime_noCut);
          }
          else if(isMaskE){
            hCollisionTimeVsVertex_noCut_maskE->Fill(vrtPos_noCut,collTime_noCut);
          }


          if(ampSum_sideA>6000)hCollisionTimeVsVertex_noCut_highAmpSumA->Fill(vrtPos_noCut,collTime_noCut);
          if(ampSum_sideC>2000)hCollisionTimeVsVertex_noCut_highAmpSumC->Fill(vrtPos_noCut,collTime_noCut);
          hCollisionTimeVsVertex_noCut->Fill(vrtPos_noCut,collTime_noCut);
          
          hOrAVsOrC_noCut->Fill(meanTimeC_noCut,meanTimeA_noCut);
        }
        if(nChanA>0) {
          hTriggersEmu->Fill(static_cast<double>(o2::fit::Triggers::bitA));
          if(isCollision) {
            hMeanTimeA->Fill(meanTimeA * 13.02);
          }
          if(nChanC>0) {
            if(vrtPos>-50&&vrtPos<50)hTriggersEmu->Fill(static_cast<double>(o2::fit::Triggers::bitVertex));
            hCollisionTimeVsVertex->Fill(vrtPos,collTime);
            hCollisionTime->Fill(collTime);
            hVertex->Fill(vrtPos);

            if(isEventVertex) {
              hCollisionTimeVsVertex_vrtTrg->Fill(vrtPos,collTime);
              hCollisionTime_vrtTrg->Fill(collTime);
              hVertex_vrtTrg->Fill(vrtPos);
              hVertexVsBC_vrtTrg->Fill(bc,vrtPos);
              if(isCollision) {
                hVertexVsBC_vrtTrg_inColl->Fill(bc,vrtPos);
              }
            }
            else {
              hCollisionTimeVsVertex_noVrtTrg->Fill(vrtPos,collTime);
              hCollisionTime_noVrtTrg->Fill(collTime);
              hVertex_noVrtTrg->Fill(vrtPos);
              hVertexVsBC_noVrtTrg->Fill(bc,vrtPos);
/*
            for(const auto &entry: vecPMbits) {
              for(int i=0;i<8;i++) {
                if(entry & (1<<i)) {
                  hVertexVsPMbits_noVrtTrg->Fill(vrtPos,i);
                }
              }
            }
*/
            }
          }
        }
        if(nChanC>0) {
          hTriggersEmu->Fill(static_cast<double>(o2::fit::Triggers::bitC));
          if(isCollision) {
            hMeanTimeC->Fill(meanTimeC * 13.02);
          }
        }
        if(trg.getVertex()) {
          hBC_trgVrt->Fill(bc);
          hNChanAvsC_vrtTrg->Fill(nChanA,nChanC);
          if(!isCollision) {
            hBC_VrtTrg_OutOfColl->Fill(bc);
          }
        }
        else{
          if(trg.getOrA()) {
            if(isCollision) {
              hBC_OrA_notVrt_collBC->Fill(bc);
            }
            else {
              hBC_OrA_notVrt_nonCollBC->Fill(bc);
            }
          }
          if(trg.getOrC()) {
            if(isCollision) {
              hBC_OrC_notVrt_collBC->Fill(bc);
            }
            else {
              hBC_OrC_notVrt_nonCollBC->Fill(bc);
            }
          }
        }
        hNChanA->Fill(nChanA);
        hNChanC->Fill(nChanC);
        hNChanAvsC->Fill(nChanA,nChanC);
        hNChannels->Fill(nChanA+nChanC);//vrtPos_noCut,collTime_noCut
/*
        const bool isSpotFT0_1 = (collTime_noCut>-3.5)
                              && (collTime_noCut<-1.5)
                              && (vrtPos_noCut>-90.)
                              && (vrtPos_noCut<-70.) && (nChanA_noCut>0) && (nChanC_noCut>0);
        if(isSpotFT0_1) hNChanAvsC->Fill(nChanA_noCut,nChanC_noCut);
*/
      }
      /*
      **PUT HERE CODE FOR PROCESSING TF
      */

      //

    }
    delete treeInput;
    fileInput.Close();
  }
  // Postproc
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
  //Writing data
  const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(listOutput,filepathOutput);
//  const auto isDataWrittenAmpBC = utilities::AnalysisUtils::writeObjToFile(listOutputAmpBC,filepathOutputAmpBC);
//  const auto isDataWrittenTimeBC = utilities::AnalysisUtils::writeObjToFile(listOutputTimeBC,filepathOutputTimeBC);
  delete listGarbage;
  std::cout<<std::endl;
}


