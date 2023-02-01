#ifndef EventChDataParams_H
#define EventChDataParams_H

#include "DetectorFIT.h"
#include "Constants.h"

#include <iostream>
namespace digits
{
template<typename DetectorFITtype>
struct EventChDataParamsFIT {
  typedef DetectorFITtype DetectorFIT_t;
  constexpr static std::size_t sNchannelsA = DetectorFIT_t::sNchannelsA;
  constexpr static std::size_t sNchannelsC = DetectorFIT_t::sNchannelsC;
  constexpr static std::size_t sNchannelsAC = DetectorFIT_t::sNchannelsAC;
  //constexpr static double sNS2Cm = constants::sNS2Cm; // light NS to Centimetres
  //constexpr static double sNSperTimeChannel = constants::sTDC2NS;
  constexpr static int sDummyValue = -5000;
  int mAmpSumA{0};
  int mAmpSumC{0};
  int mAmpSum{0};
  float mMeanTimeA{0};
  float mMeanTimeC{0};
  int mNchanA{0};
  int mNchanC{0};
  double mCollTime{sDummyValue};
  double mVrtPos{sDummyValue};


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
    const auto side = DetectorFIT_t::getSide(chID);
    if(DetectorFIT_t::isSideA(side)) {
      mNchanA++;
      mMeanTimeA+=time;
      mAmpSumA+=amp;
    }
    else if(DetectorFIT_t::isSideC(side)){
      mNchanC++;
      mMeanTimeC+=time;
      mAmpSumC+=amp;
    }
  }
  void calculate() {
    if constexpr(sNchannelsA>0) {
      if(mNchanA>0) {
        mMeanTimeA = mMeanTimeA/mNchanA;
        mIsReadyA=true;
        if(mNchanC>0) {
          mIsReadyAC=true;
        }
      }
      else {
         mMeanTimeA = sDummyValue;
         mAmpSumA = sDummyValue;
      }
    }
    if constexpr(sNchannelsC>0) {
      if(mNchanC>0) {
        mMeanTimeC = mMeanTimeC/mNchanC;
        mIsReadyC=true;
      }
      else {
         mMeanTimeC = sDummyValue;
         mAmpSumC = sDummyValuel;
      }
    }
    if constexpr(sNchannelsA>0 && sNchannelsC>0) {
     if(mNchanA>0 && mNchanC>0) {
        mCollTime = (mMeanTimeA + mMeanTimeC)/2 * constants::sTDC2NS;
        mVrtPos = (mMeanTimeC - mMeanTimeA)/2 * constants::sTDC2Cm;
      }
    }
    else if constexpr(sNchannelsA>0) {
      mCollTime = mMeanTimeA * constants::sTDC2NS;
    }
    else if constexpr(sNchannelsC>0) {
      mCollTime = mMeanTimeC * constants::sTDC2NS;
    }
  }
/*  double getCollisionTimeNS() const {
    return mCollTime * constants::sTDC2NS;
  }
*/
};

using EventChDataParamsFDD = EventChDataParamsFIT<detectorFIT::DetectorFDD>;
using EventChDataParamsFT0 = EventChDataParamsFIT<detectorFIT::DetectorFT0>;
using EventChDataParamsFV0 = EventChDataParamsFIT<detectorFIT::DetectorFV0>;

} // namespace digits
#endif