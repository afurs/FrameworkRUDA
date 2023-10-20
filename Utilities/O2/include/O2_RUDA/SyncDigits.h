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
  
  using DigitCTP = o2::ctp::CTPDigit;
  template<typename VecDigitEntryType=uint32_t>
  struct SyncParam {
    enum EDetectorBit { kFDD,
                        kFT0,
                        kFV0,
                        kCTP
    };
    typedef std::map<o2::InteractionRecord,SyncParam<VecDigitEntryType> > MapIR2PosInData_t;
    constexpr static std::size_t sNdetectors=4;
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
    bool isCTP() const {
      return mActiveDets.test(EDetectorBit::kCTP);
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
    VecDigitEntryType getIndexCTP() const {
      return mVecDigitEntry[EDetectorBit::kCTP];
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

    static void fillSyncMapCTP(MapIR2PosInData_t &mapIR2PosInData, const std::vector<DigitCTP> &vecDigitsCTP) {
      std::size_t index{0};
      for(const auto &digit: vecDigitsCTP) {
        const auto ir = digit.intRecord;
        auto itMap = mapIR2PosInData.insert({ir,{}});
        itMap.first->second.mActiveDets.set(EDetectorBit::kCTP);
        itMap.first->second.mVecDigitEntry[EDetectorBit::kCTP]=index;
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

template<typename DetectorType>
struct ManagerDigits {
  typedef DetectorType Detector_t;
  ManagerDigits(const std::string &filepath, const std::string &treeName = "o2sim"):mFilepath(filepath),mTreeName(treeName) {
    init();
  }
  using Digit = typename Detector_t::Digit_t;
  using ChannelData = typename Detector_t::ChannelData_t;

  std::unique_ptr<TFile> mFileInput{nullptr};
  std::unique_ptr<TTree> mTreeInput{nullptr};
  std::string mFilepath{""};
  std::string mTreeName{"o2sim"};

  std::vector<Digit> *mPtrVecDigits = nullptr;
  std::vector<ChannelData> *mPtrVecChannelData = nullptr;

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
    if(mFilepath.size()>0) {
      prepareTree(mFilepath,mTreeName,mFileInput,mTreeInput);
    }
  }
  void setVars(std::vector<Digit> &vecDigits, std::vector<ChannelData> &vecChannelData) {
    mPtrVecDigits = &vecDigits;
    mPtrVecChannelData = &vecChannelData;
    if(mTreeInput != nullptr) {
      mTreeInput->SetBranchAddress(Detector_t::sDigitBranchName, &mPtrVecDigits);
      mTreeInput->SetBranchAddress(Detector_t::sChannelDataBranchName, &mPtrVecChannelData);
    }
  }
  void getEntry(long long entryID) {
    if(mTreeInput != nullptr) {
      mTreeInput->GetEntry(entryID);
    }
  }
  long long  getEntries() const{
    if(mTreeInput != nullptr) {
      return mTreeInput->GetEntries();
    }
    else {
      return 0;
    }
  }

};

template<typename... ManagerDigitsType>
struct ManagerDigitsFIT: public ManagerDigitsType... {
 public:
  template<typename... Args>
  ManagerDigitsFIT(Args&&... args):ManagerDigitsType(std::forward<Args>(args))... {
    (static_cast<void>(setNEntries(ManagerDigitsType::getEntries())), ...);
  }
  template<typename DetectorType,typename... Args>
  void setVars(Args&&... args) {
    static_cast<ManagerDigits<DetectorType>*>(this)->setVars(std::forward<Args>(args)...);
  }
  void getEntry(int entryID) {
    (static_cast<void>(ManagerDigitsType::getEntry(entryID)), ...);
  }
  void setNEntries(uint64_t entries){
    mNentries=std::min(mNentries,entries);
  }
  uint64_t mNentries{0xffffffffffffffff};
};

struct ManagerSyncFIT {
  ManagerSyncFIT(const std::string &filepathFDD,const std::string &filepathFT0,const std::string &filepathFV0,const std::string &filepathCTP="", const std::string &treeName = "o2sim") :
  mFilepathFDD(filepathFDD), mFilepathFT0(filepathFT0), mFilepathFV0(filepathFV0),mFilepathCTP(filepathCTP),mTreeName(treeName) {
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
    if(mFilepathFDD.size()>0) {
      prepareTree(mFilepathFDD,mTreeName,mFileInputFDD,mTreeInputFDD);
    }
    if(mFilepathFT0.size()>0) {
      prepareTree(mFilepathFT0,mTreeName,mFileInputFT0,mTreeInputFT0);
    }
    if(mFilepathFV0.size()>0) {
      prepareTree(mFilepathFV0,mTreeName,mFileInputFV0,mTreeInputFV0);
    }
    if(mFilepathCTP.size()>0) {
      prepareTree(mFilepathCTP,mTreeName,mFileInputCTP,mTreeInputCTP);
    }

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
  std::unique_ptr<TFile> mFileInputCTP{nullptr};

  std::unique_ptr<TTree> mTreeInputFDD{nullptr};
  std::unique_ptr<TTree> mTreeInputFT0{nullptr};
  std::unique_ptr<TTree> mTreeInputFV0{nullptr};
  std::unique_ptr<TTree> mTreeInputCTP{nullptr};


  std::string mFilepathFDD{""};
  std::string mFilepathFT0{""};
  std::string mFilepathFV0{""};
  std::string mFilepathCTP{""};

  std::string mTreeName{"o2sim"};

  std::vector<DigitFDD> *mPtrVecDigitsFDD = nullptr;
  std::vector<ChannelDataFDD> *mPtrVecChannelDataFDD = nullptr;
  std::vector<DigitFT0> *mPtrVecDigitsFT0 = nullptr;
  std::vector<ChannelDataFT0> *mPtrVecChannelDataFT0 = nullptr;
  std::vector<DigitFV0> *mPtrVecDigitsFV0 = nullptr;
  std::vector<ChannelDataFV0> *mPtrVecChannelDataFV0 = nullptr;
  std::vector<DigitCTP> *mPtrVecDigitsCTP = nullptr;


  void setVars(std::vector<DigitFDD> &vecDigitsFDD, std::vector<ChannelDataFDD> &vecChannelDataFDD,
                 std::vector<DigitFT0> &vecDigitsFT0, std::vector<ChannelDataFT0> &vecChannelDataFT0,
                 std::vector<DigitFV0> &vecDigitsFV0, std::vector<ChannelDataFV0> &vecChannelDataFV0) {
    //FDD
    mPtrVecDigitsFDD = &vecDigitsFDD;
    mPtrVecChannelDataFDD = &vecChannelDataFDD;
    if(mTreeInputFDD != nullptr) {
      mTreeInputFDD->SetBranchAddress(detectorFIT::DetectorFDD::sDigitBranchName, &mPtrVecDigitsFDD);
      mTreeInputFDD->SetBranchAddress(detectorFIT::DetectorFDD::sChannelDataBranchName, &mPtrVecChannelDataFDD);
    }
    //FT0
    mPtrVecDigitsFT0 = &vecDigitsFT0;
    mPtrVecChannelDataFT0 = &vecChannelDataFT0;
    if(mTreeInputFT0 != nullptr) {
      mTreeInputFT0->SetBranchAddress(detectorFIT::DetectorFT0::sDigitBranchName, &mPtrVecDigitsFT0);
      mTreeInputFT0->SetBranchAddress(detectorFIT::DetectorFT0::sChannelDataBranchName, &mPtrVecChannelDataFT0);
    }
    //FV0
    mPtrVecDigitsFV0 = &vecDigitsFV0;
    mPtrVecChannelDataFV0 = &vecChannelDataFV0;
    if(mTreeInputFV0 != nullptr) {
      mTreeInputFV0->SetBranchAddress(detectorFIT::DetectorFV0::sDigitBranchName, &mPtrVecDigitsFV0);
      mTreeInputFV0->SetBranchAddress(detectorFIT::DetectorFV0::sChannelDataBranchName, &mPtrVecChannelDataFV0);
    }
  }
  void setVarsCTP(std::vector<DigitCTP> &vecDigitsCTP) {
    //CTP
    mPtrVecDigitsCTP = &vecDigitsCTP;
    if(mTreeInputCTP != nullptr) {
      mTreeInputCTP->SetBranchAddress(detectorFIT::DetectorCTP::sDigitBranchName, &mPtrVecDigitsCTP);
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
    if(mTreeInputCTP != nullptr) {
      mTreeInputCTP->GetEntry(entryID);
    }

  }
  long long getEntries() {
    if(mTreeInputFT0!=nullptr) {
      return mTreeInputFT0->GetEntries();
    }
    else {
      std::cout<<"\nEmpty FT0 tree!\n";
      return 0;
    }
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