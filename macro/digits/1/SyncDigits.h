#ifndef SyncDigits_H
#define SyncDigits_H

#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"

#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"

#include "CommonDataFormat/InteractionRecord.h"

#include <bitset>
#include <map>
#include <vector>
#include <iostream>
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
} // namespace digits

#endif