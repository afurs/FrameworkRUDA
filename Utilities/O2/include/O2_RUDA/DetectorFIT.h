#ifndef DetectorFIT_H
#define DetectorFIT_H

#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"

#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"

#include "CommonDataFormat/InteractionRecord.h"

#include <array>
#include <utility>
#include <type_traits>

namespace detectorFIT {

enum EDetectorFIT { kFDD, kFT0, kFV0};
enum ESide { kNothing, kSideA, kSideC};

using DigitFDD = o2::fdd::Digit;
using ChannelDataFDD = o2::fdd::ChannelData;
using TriggersFDD = o2::fdd::Triggers;

using DigitFT0 = o2::ft0::Digit;
using ChannelDataFT0 = o2::ft0::ChannelData;
using TriggersFT0 = o2::ft0::Triggers;

using DigitFV0 = o2::fv0::Digit;
using ChannelDataFV0 = o2::fv0::ChannelData;
using TriggersFV0 = o2::fv0::Triggers;


template<int DetID,int NchannelsA, int NchannelsC,int NchannelsNone, typename DigitType, typename ChannelDataType,bool isChIDdirectForSides >
class BaseDetectorFIT {
 public:
  constexpr static int sDetFIT_ID = DetID;
  constexpr static int sNchannelsA = NchannelsA;
  constexpr static int sNchannelsC = NchannelsC;
  constexpr static int sNchannelsNone = NchannelsNone;
  constexpr static int sNchannelsAC = sNchannelsA+sNchannelsC;
  constexpr static int sNchannelsAll = sNchannelsAC+sNchannelsNone;
  constexpr static bool sIsChIDdirectForSides = isChIDdirectForSides;
  typedef DigitType Digit_t;
  typedef ChannelDataType ChannelData_t;

  constexpr static int getSide(int chID) {
    if constexpr(isChIDdirectForSides) {
      if(chID<sNchannelsA) {
        return ESide::kSideA;
      }
      else if (chID<sNchannelsAC) {
        return ESide::kSideC;
      }
    }
    else {
      if (chID<sNchannelsC) {
        return ESide::kSideC;
      }
      else if(chID<sNchannelsAC) {
        return ESide::kSideA;
      }
    }
    return ESide::kNothing;
  }
  constexpr static bool isSideA(int side) {
    return side == ESide::kSideA;
  }
  constexpr static bool isSideC(int side) {
    return side == ESide::kSideC;
  }

  constexpr static std::array<int,sNchannelsAll> setChIDs2Side() {
    std::array<int,sNchannelsAll> chID2side{};
    for(int iCh=0;iCh<chID2side.size();iCh++) {
      chID2side[iCh]=getSide(iCh);
    }
  }
  constexpr static std::array<int,sNchannelsAll> sArrChID2Side = setChIDs2Side();
};

template<int DetID>
class DetectorFIT:BaseDetectorFIT<DetID,0,0,0,void,void,true> {
};

template<>
struct DetectorFIT<EDetectorFIT::kFDD>: public BaseDetectorFIT<EDetectorFIT::kFDD,16,16,3,DigitFDD,ChannelDataFDD,false> {
  static constexpr const char* sDigitBranchName="FDDDigit";
  static constexpr const char* sChannelDataBranchName="FDDDigitCh";

  template<typename ChannelDataType>
  static auto amp(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.mChargeADC)> & {
    return channelData.mChargeADC;
  }
  template<typename ChannelDataType>
  static auto time(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.mTime)> & {
    return channelData.mTime;
  }
  template<typename ChannelDataType>
  static auto channelID(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.mPMNumber)> & {
    return channelData.mPMNumber;
  }
  template<typename ChannelDataType>
  static auto pmBits(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.mFEEBits)> & {
    return channelData.mFEEBits;
  }

};

template<>
struct DetectorFIT<EDetectorFIT::kFT0>: public BaseDetectorFIT<EDetectorFIT::kFT0,96,112,4,DigitFT0,ChannelDataFT0,true> {
  static constexpr const char* sDigitBranchName="FT0DIGITSBC";
  static constexpr const char* sChannelDataBranchName="FT0DIGITSCH";
  template<typename ChannelDataType>
  static auto amp(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.QTCAmpl)> & {
    return channelData.QTCAmpl;
  }
  template<typename ChannelDataType>
  static auto time(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.CFDTime)> & {
    return channelData.CFDTime;
  }
  template<typename ChannelDataType>
  static auto channelID(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.ChId)> & {
    return channelData.ChId;
  }
  template<typename ChannelDataType>
  static auto pmBits(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.ChainQTC)> & {
    return channelData.ChainQTC;
  }
};

template<>
struct DetectorFIT<EDetectorFIT::kFV0>: public BaseDetectorFIT<EDetectorFIT::kFV0,48,0,1,DigitFV0,ChannelDataFV0,true> {
  static constexpr const char* sDigitBranchName="FV0DigitBC";
  static constexpr const char* sChannelDataBranchName="FV0DigitCh";

  template<typename ChannelDataType>
  static auto amp(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.QTCAmpl)> & {
    return channelData.QTCAmpl;
  }
  template<typename ChannelDataType>
  static auto time(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.CFDTime)> & {
    return channelData.CFDTime;
  }
  template<typename ChannelDataType>
  static auto channelID(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.ChId)> & {
    return channelData.ChId;
  }
  template<typename ChannelDataType>
  static auto pmBits(ChannelDataType &&channelData)-> const std::decay_t<decltype(channelData.ChainQTC)> & {
    return channelData.ChainQTC;
  }
};

using DetectorFDD = DetectorFIT<EDetectorFIT::kFDD>;
using DetectorFT0 = DetectorFIT<EDetectorFIT::kFT0>;
using DetectorFV0 = DetectorFIT<EDetectorFIT::kFV0>;


}// namespace detectorFIT

#endif