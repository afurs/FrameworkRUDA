#include <algorithm>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <tuple>
#include <set>
#include <regex>

#include <TH1.h>
#include <TH2.h>
#include <TTree.h>
#include <TFile.h>
#include <TSystem.h>

#include "DataFormatsParameters/GRPLHCIFData.h"
#include "CCDB/BasicCCDBManager.h"
#include "DataFormatsFT0/SpectraInfoObject.h"
#include "DataFormatsTOF/CalibLHCphaseTOF.h"
#include "TOFBase/Geo.h"

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/DynamicTable.h"

#include "O2_RUDA/FunctorField.h"
#include "O2_RUDA/TimestampInfo.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;
using CalibObject = o2::ft0::TimeSpectraInfoObject;
using namespace functors;
void makeDynTable(const std::string &pathToSrc) {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  //Input parameters
  //Default runnum
  const std::string filepathOutput = "tableTimeOfsset_new.csv";
  const std::string filepathOutput_paths ="filepathTimeOfsset_new.csv";
  const std::string filenameMask = "o2-ft0-TimeSpectraInfoObject_%llu_%llu_%llu_%llu.root";
  const int nArg = 4;
  ///////////////////////////////////////////////////
  std::function<uint64_t(uint64_t)>  getValue64Bit = [](uint64_t value) { return value; };
  std::function<std::string(const std::string *)>  getValueString = [](const std::string *str) { return *str; };

  std::function<unsigned int(const std::string *)>  getRunnumFromFilepath = [](const std::string *ptrFilepath)->unsigned int {
    const std::string mask{"/FT0/Calib/"};
    const auto pos = ptrFilepath->find(mask);
    if(pos>5) {
     /// std::cout<<std::endl<<ptrFilepath->substr(pos+4,6)<<std::endl
      return static_cast<unsigned int>(std::stoul((ptrFilepath->substr(pos-6,6))));
    }
    else {
      return 0;
    }
  };

  std::function<std::vector<float>(const CalibObject *)>  getVecCalibValues = [](const CalibObject *calibObject) {
    std::vector<float> vecResult{};

    int refChA{0};
    int refChC{96};
    const float refTimeA = (refChA>-1&&refChA<96) ? calibObject->mTime[refChA].mGausMean * 13.02 : 0.;
    const float refTimeC = (refChC>95&&refChC<208) ? calibObject->mTime[refChC].mGausMean * 13.02 : 0.;

    for(int iCh=0;iCh<208;iCh++) {
      vecResult.push_back(13.02*calibObject->mTime[iCh].mGausMean);
/*      if(iCh==refChA||iCh==refChC) {
        vecResult.push_back(13.02*calibObject->mTime[iCh].mGausMean);
      }
      else if(iCh<96){
        vecResult.push_back(13.02*calibObject->mTime[iCh].mGausMean-refTimeA);
      }
      else if(iCh>95){
        vecResult.push_back(13.02*calibObject->mTime[iCh].mGausMean-refTimeC);
      }
*/
    }
    vecResult.push_back(13.02*calibObject->mTimeA.mGausMean);
    vecResult.push_back(13.02*calibObject->mTimeC.mGausMean);
    vecResult.push_back(13.02*calibObject->mSumTimeAC.mGausMean);
    vecResult.push_back(13.02*calibObject->mDiffTimeCA.mGausMean);
    return vecResult;
  };
  std::function<float(uint64_t)>  getLHCphase_TOF = [](uint64_t timestamp) {
    const o2::dataformats::CalibLHCphaseTOF* phasePtr = o2::ccdb::BasicCCDBManager::instance().getForTimeStamp<o2::dataformats::CalibLHCphaseTOF>("TOF/Calib/LHCphase", timestamp);
    float phase = phasePtr->getLHCphase(0);
    int buf = int((phase+5000.)*o2::tof::Geo::BC_TIME_INPS_INV);
    phase -= buf*o2::tof::Geo::BC_TIME_INPS;
    return phase;
  };
  ///////////////////////////////////////////////////
  auto dynTable = common::DynamicTable(getRunnum,getValue64Bit, getValue64Bit,getValue64Bit,getVecCalibValues,getLHCphase_TOF);
  dynTable.mArrFieldNames.push_back("runnum");
  dynTable.mArrFieldNames.push_back("chunk");
  dynTable.mArrFieldNames.push_back("tsStart");
  dynTable.mArrFieldNames.push_back("tsEnd");
  for(int iCh=0;iCh<208;iCh++) {
    dynTable.mArrFieldNames.push_back(std::string{"ch"+std::to_string(iCh)});
  }
  dynTable.mArrFieldNames.push_back("meanTimeA");
  dynTable.mArrFieldNames.push_back("meanTimeC");
  dynTable.mArrFieldNames.push_back("collisionTime");
  dynTable.mArrFieldNames.push_back("vertexTime");
  dynTable.mArrFieldNames.push_back("LHCphase_TOF");
  ///////////////////////////////////////////////////
  auto dynTableFilepths = common::DynamicTable(getRunnum,getValue64Bit, getValue64Bit,getValueString);
  dynTableFilepths.mArrFieldNames.push_back("runnum");
  dynTableFilepths.mArrFieldNames.push_back("tsStart");
  dynTableFilepths.mArrFieldNames.push_back("tsEnd");
  dynTableFilepths.mArrFieldNames.push_back("filepaths");
  ///////////////////////////////////////////////////
  auto dynTableFileList = common::DynamicTable(getValueString);
  //dynTableFilepths.mArrFieldNames.push_back("runnum");
  //Get map run number->file
  auto getNentries = [](const CalibObject *calibObject)->double {
     double nEntries{0};
     for(int iCh=0;iCh<208;iCh++) {
       nEntries+=calibObject->mTime[iCh].mStat;
     }
     return nEntries;
  };
  auto vecRunToFilepaths = Utils::makeVecFilepaths(pathToSrc,std::regex{".*o2-ft0-TimeSpectraInfoObject_.*\\.root"});
  struct FileInfo {
    FileInfo(const std::string filepath,unsigned int runnum,double nentries):mRunnum(runnum),mTsInfo(Utils::getFilename(filepath)),mFilepath(filepath),mNentries(nentries)
    {
      
    }
    unsigned int mRunnum;
    std::string mFilepath;
    double mNentries{0};
    TimestampInfo mTsInfo;
    void print() const {
      std::cout<<"\nFilepath: "<<mFilepath;
      std::cout<<"\nRunnum: "<<mRunnum;
      std::cout<<"\nCreation time: "<<mTsInfo.mTsCreation;
      std::cout<<"\nStart time: "<<mTsInfo.mTsStart;
      std::cout<<"\nEnd time: "<<mTsInfo.mTsEnd;
      std::cout<<"\nChunk: "<<mTsInfo.mChunk;
      std::cout<<"\n";
    }
  };
  std::vector<FileInfo> vecFilepathInfo;
  std::set<uint64_t> setCreationTime{};
  std::set<uint64_t> setStartTime{};
  for(const auto &filepathInput : vecRunToFilepaths) {
    TFile fileInput(filepathInput.c_str(),"READ");
    if(!fileInput.IsOpen()) {
      std::cout<<"\nWARNING! CANNOT OPEN FILE: "<<filepathInput<<std::endl;
      continue;
    }
    const CalibObject* calibObject = (CalibObject* )fileInput.Get("ccdb_object");
    if(calibObject == nullptr) {
      std::cout<<"\nWARNING! Cannot extract calibration object!\n";
      fileInput.Close();
      continue;
    }
    const auto nentries = getNentries(calibObject);
    if(nentries==0) {
      std::cout<<"\nWARNING! No entries in file: "<<filepathInput<<"\n";
      continue;
    }
    FileInfo fInfo(filepathInput,getRunnumFromFilepath(&filepathInput),nentries);
//    if(/*setCreationTime.find(fInfo.mTsInfo.mTsCreation)==setCreationTime.end()&&*/setStartTime.find(fInfo.mTsInfo.mTsStart)==setStartTime.end()) {
//      setCreationTime.insert(fInfo.mTsInfo.mTsCreation);
//      setStartTime.insert(fInfo.mTsInfo.mTsStart);
    vecFilepathInfo.push_back(fInfo);
    //fInfo.print();
    fileInput.Close();
    delete calibObject;
//    }
  }

  std::sort(vecFilepathInfo.begin(),vecFilepathInfo.end(), [&] (const auto &f1,const auto &f2) {
    return std::tie(f1.mTsInfo.mTsStart,f1.mNentries)<std::tie(f2.mTsInfo.mTsStart,f2.mNentries);
//    return std::tie(f1.mTsInfo.mTsStart,f1.mTsInfo.mTsCreation)<std::tie(f2.mTsInfo.mTsStart,f1.mTsInfo.mTsCreation);
});

  //Distribution reference offsets
  
  //Garbage list
  TList *listGarbage = new TList();
  listGarbage->SetOwner(true);
  listGarbage->SetName("output");
  std::map<std::string,TList *> mapOutput2Store{};
/*
  //Output lists
  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");
  {
    const std::string filepathOutputBuf = "distrLHC22o.root";
    mapOutput2Store.insert({filepathOutputBuf, listOutput});
  }
  listGarbage->Add(listOutput);
  std::vector<TH1F *> vecHists{};
  for(int iCh=0;iCh<208;iCh++) {
    vecHists.push_back(new TH1F(Form("hDistr_ch%i",iCh),Form("hDistr_ch%i",iCh),2000,-200,200));
    listOutput->Add(vecHists.back());
  }
  */
  //Processing
  for(const auto &entry: vecFilepathInfo) {
    const auto &filepathInput = entry.mFilepath;
    const auto &runnum = entry.mRunnum;
    const auto &tsInfo = entry.mTsInfo;
    if(setStartTime.find(tsInfo.mTsStart)!=setStartTime.end()) continue;
    setStartTime.insert(tsInfo.mTsStart);
      std::cout<<"\nProcessing file: "<<filepathInput<<std::endl;
      entry.print();
      //continue;
      TFile fileInput(filepathInput.c_str(),"READ");
      if(!fileInput.IsOpen()) {
        std::cout<<"\nWARNING! CANNOT OPEN FILE: "<<filepathInput<<std::endl;
        continue;
      }
      const CalibObject* calibObject = (CalibObject* )fileInput.Get("ccdb_object");
      if(calibObject == nullptr) {
        std::cout<<"\nWARNING! Cannot extract calibration object!\n";
        continue;
      }
      uint64_t tsTOF{tsInfo.mTsStart+1000};
      dynTable.setCurrentArg<0>(runnum);
//================================================================
//================================================================
      dynTable.setCurrentArg<1>(tsInfo.mChunk);
      dynTable.setCurrentArg<2>(tsInfo.mTsStart);
      dynTable.setCurrentArg<3>(tsInfo.mTsEnd);
      dynTable.setCurrentArg<4>(calibObject);
      dynTable.setCurrentArg<5>(tsTOF);
//================================================================
      dynTable.fillTable();

        dynTableFilepths.setCurrentArg<0>(runnum);
        dynTableFilepths.setCurrentArg<1>(tsInfo.mTsStart);
        dynTableFilepths.setCurrentArg<2>(tsInfo.mTsEnd);
        dynTableFilepths.setCurrentArg<3>(&filepathInput);
        dynTableFilepths.fillTable();
        
     dynTableFileList.setCurrentArg<0>(&filepathInput);
     dynTableFileList.fillTable();
/*
      const auto &newEntry = dynTable.mTable.back();
      const auto &vecCalibOffset = std::get<4>(newEntry);
      if(vecCalibOffset[208]!=0&&vecCalibOffset[209]!=0) {
        for(int iCh=0;iCh<208;iCh++) {
          if(vecCalibOffset[iCh]!=0) vecHists[iCh]->Fill(vecCalibOffset[iCh]);
        }
        dynTableFilepths.setCurrentArg<0>(runnum);
        dynTableFilepths.setCurrentArg<1>(tsInfo.mTsStart);
        dynTableFilepths.setCurrentArg<2>(tsInfo.mTsEnd);
        dynTableFilepths.setCurrentArg<3>(&filepathInput);
        dynTableFilepths.fillTable();
      }
*/
      delete calibObject;
      fileInput.Close();

  }
  dynTable.print();
  dynTable.toCSV(filepathOutput,true);

  dynTableFilepths.toCSV(filepathOutput_paths,true);

  dynTableFileList.toCSV("fileList.txt",true);
  std::cout<<std::endl;
  
  for(const auto &entry: mapOutput2Store) {
    const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(entry.second,entry.first);
  }
  delete listGarbage;
  std::cout<<std::endl;
}

