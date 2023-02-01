#ifndef SyncDigits_H
#define SyncDigits_H

#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"

#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"

#include "CommonDataFormat/InteractionRecord.h"

#include "DetectorFIT.h"

#include "TTree.h"
#include "TFile.h"

#include <bitset>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <memory>

namespace digits
{
  using DigitFDD = o2::fdd::Digit;
  using ChannelDataFDD = o2::fdd::ChannelData;
  using TriggersFDD = o2::fdd::Triggers;

  using DigitFT0 = o2::ft0::Digit;
  using ChannelDataFT0 = o2::ft0::ChannelData;
  using TriggersFT0 = o2::ft0::Triggers;

  using DigitFV0 = o2::fv0::Digit;
  using ChannelDataFV0 = o2::fv0::ChannelData;
  using TriggersFV0 = o2::fv0::Triggers;
  template<typename VecDigitEntryType=uint32_t>
  struct SyncParam {
    enum EDetectorBit { kFDD,
                        kFT0,
                        kFV0
    };
    typedef std::map<o2::InteractionRecord,SyncParam<VecDigitEntryType> > MapIR2PosInData_t;
    constexpr static std::size_t sNdetectors=3;
    VecDigitEntryType mVecDigitEntry[sNdetectors];
    std::bitset<sNdetectors> mActiveDets;
    bool isFDD() const {
      return mActiveDets.test(EDetectorBit::kFDD);
    }
    bool isFT0() const {
      return mActiveDets.test(EDetectorBit::kFT0);
    }
    bool isFV0() const {
      return mActiveDets.test(EDetectorBit::kFV0);
    }
    VecDigitEntryType getIndexFDD() const {
      return mVecDigitEntry[EDetectorBit::kFDD];
    }
    VecDigitEntryType getIndexFT0() const {
      return mVecDigitEntry[EDetectorBit::kFT0];
    }
    VecDigitEntryType getIndexFV0() const {
      return mVecDigitEntry[EDetectorBit::kFV0];
    }

    template<typename DigitType>
    static void fillSyncMap(MapIR2PosInData_t &mapIR2PosInData, const std::vector<DigitType> &vecDigits) {
      std::size_t index{0};
      for(const auto &digit: vecDigits) {
        const auto ir = digit.getIntRecord();
        auto itMap = mapIR2PosInData.insert({ir,{}});
        if constexpr (std::is_same<DigitFDD,DigitType>::value) {
          itMap.first->second.mActiveDets.set(EDetectorBit::kFDD);
          itMap.first->second.mVecDigitEntry[EDetectorBit::kFDD]=index;
        }
        else if (std::is_same<DigitFT0,DigitType>::value) {
          itMap.first->second.mActiveDets.set(EDetectorBit::kFT0);
          itMap.first->second.mVecDigitEntry[EDetectorBit::kFT0]=index;
        }
        else if (std::is_same<DigitFV0,DigitType>::value) {
          itMap.first->second.mActiveDets.set(EDetectorBit::kFV0);
          itMap.first->second.mVecDigitEntry[EDetectorBit::kFV0]=index;
        }
        else {
          std::cout<<"\nWARNING! UNKNOWN DIGIT TYPE!\n";
        }
        index++;
      }
    }
    template<typename DigitFDD_Type, typename DigitFT0_Type, typename DigitFV0_Type>
    static MapIR2PosInData_t makeSyncMap(const std::vector<DigitFDD_Type> &vecDigitsFDD,
                                         const std::vector<DigitFT0_Type> &vecDigitsFT0,
                                         const std::vector<DigitFV0_Type> &vecDigitsFV0) {
      MapIR2PosInData_t mapIR2PosInData{};
      fillSyncMap(mapIR2PosInData, vecDigitsFDD);
      fillSyncMap(mapIR2PosInData, vecDigitsFT0);
      fillSyncMap(mapIR2PosInData, vecDigitsFV0);
      return mapIR2PosInData;
    }
  };
struct ManagerSyncFIT {
  ManagerSyncFIT(const std::string &filepathFDD,const std::string &filepathFT0,const std::string &filepathFV0, const std::string &treeName = "o2sim") :
  mFilepathFDD(filepathFDD), mFilepathFT0(filepathFT0), mFilepathFV0(filepathFV0),mTreeName(treeName) {
    init();
  }
/*
  virtual ~ManagerSyncFIT() {
    clear();
  }
*/
  void init() {
    auto prepareTree = [] (const std::string &filepath,const std::string &treeName,std::unique_ptr<TFile> &inputFile, std::unique_ptr<TTree> &inputTree) {
      inputFile = std::unique_ptr<TFile>(TFile::Open(filepath.c_str()));
      if(inputFile != nullptr) {
        if(inputFile->IsOpen()) {
          inputTree = std::unique_ptr<TTree>(dynamic_cast<TTree*>(inputFile->Get(treeName.c_str())));
        }
        else {
          std::cout<<"\nError! There are no digit file at address: "<<filepath<<std::endl;
        }
      }
      else {
        std::cout<<"\nError! There are no digit file at address: "<<filepath<<std::endl;
      }
    };
    prepareTree(mFilepathFDD,mTreeName,mFileInputFDD,mTreeInputFDD);
    prepareTree(mFilepathFT0,mTreeName,mFileInputFT0,mTreeInputFT0);
    prepareTree(mFilepathFV0,mTreeName,mFileInputFV0,mTreeInputFV0);
  }
/*
  TFile *mFileInputFDD{nullptr};
  TFile *mFileInputFT0{nullptr};
  TFile *mFileInputFV0{nullptr};

  TTree* mTreeInputFDD{nullptr};
  TTree* mTreeInputFT0{nullptr};
  TTree* mTreeInputFV0{nullptr};
*/
  std::unique_ptr<TFile> mFileInputFDD{nullptr};
  std::unique_ptr<TFile> mFileInputFT0{nullptr};
  std::unique_ptr<TFile> mFileInputFV0{nullptr};

  std::unique_ptr<TTree> mTreeInputFDD{nullptr};
  std::unique_ptr<TTree> mTreeInputFT0{nullptr};
  std::unique_ptr<TTree> mTreeInputFV0{nullptr};

  std::string mFilepathFDD{""};
  std::string mFilepathFT0{""};
  std::string mFilepathFV0{""};

  std::string mTreeName{"o2sim"};

  void setVars(std::vector<DigitFDD> &vecDigitsFDD, std::vector<ChannelDataFDD> &vecChannelDataFDD,
                 std::vector<DigitFT0> &vecDigitsFT0, std::vector<ChannelDataFT0> &vecChannelDataFT0,
                 std::vector<DigitFV0> &vecDigitsFV0, std::vector<ChannelDataFV0> &vecChannelDataFV0) {
    //FDD
    std::vector<DigitFDD> *ptrVecDigitsFDD = &vecDigitsFDD;
    std::vector<ChannelDataFDD> *ptrVecChannelDataFDD = &vecChannelDataFDD;
    if(mTreeInputFDD != nullptr) {
      mTreeInputFDD->SetBranchAddress(detectorFIT::DetectorFDD::sDigitBranchName, &ptrVecDigitsFDD);
      mTreeInputFDD->SetBranchAddress(detectorFIT::DetectorFDD::sChannelDataBranchName, &ptrVecChannelDataFDD);
    }
    //FT0
    std::vector<DigitFT0> *ptrVecDigitsFT0 = &vecDigitsFT0;
    std::vector<ChannelDataFT0> *ptrVecChannelDataFT0 = &vecChannelDataFT0;
    if(mTreeInputFT0 != nullptr) {
      mTreeInputFT0->SetBranchAddress(detectorFIT::DetectorFT0::sDigitBranchName, &ptrVecDigitsFT0);
      mTreeInputFT0->SetBranchAddress(detectorFIT::DetectorFT0::sChannelDataBranchName, &ptrVecChannelDataFT0);
    }
    //FV0
    std::vector<DigitFV0> *ptrVecDigitsFV0 = &vecDigitsFV0;
    std::vector<ChannelDataFV0> *ptrVecChannelDataFV0 = &vecChannelDataFV0;
    if(mTreeInputFV0 != nullptr) {
      mTreeInputFV0->SetBranchAddress(detectorFIT::DetectorFV0::sDigitBranchName, &ptrVecDigitsFV0);
      mTreeInputFV0->SetBranchAddress(detectorFIT::DetectorFV0::sChannelDataBranchName, &ptrVecChannelDataFV0);
    }
  }
  void getEntry(int entryID) {
    if(mTreeInputFDD != nullptr) {
      mTreeInputFDD->GetEntry(entryID);
    }
    if(mTreeInputFT0 != nullptr) {
      mTreeInputFT0->GetEntry(entryID);
    }
    if(mTreeInputFV0 != nullptr) {
      mTreeInputFV0->GetEntry(entryID);
    }
  }
  auto getEntries() {
    return mTreeInputFT0->GetEntries();
  }
/*
  void clear() {
    auto clearObjects = [] (TFile *inputFile, TTree *inputTree) {
      if(inputTree != nullptr) {
         delete inputTree;
         inputTree = nullptr;
      }
      if(inputFile != nullptr) {
        inputFile->Close();
        delete inputFile;
        inputFile = nullptr;
      }
    };
    clearObjects(mFileInputFDD, mTreeInputFDD);
    clearObjects(mFileInputFT0, mTreeInputFT0);
    clearObjects(mFileInputFV0, mTreeInputFV0);
  }
*/
};
} // namespace digits

#endif