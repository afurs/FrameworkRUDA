#ifndef EventChDataParams_H
#define EventChDataParams_H

#include <iostream>
namespace digits
{
template<std::size_t NChannelsA,std::size_t NChannelsC>
struct EventChDataParams {
  constexpr static std::size_t sNchannelsA = NChannelsA;
  constexpr static std::size_t sNchannelsC = NChannelsC;
  constexpr static std::size_t sNchannelsAC = sNchannelsA + sNchannelsC;
  constexpr static double sNS2Cm = 29.97; // light NS to Centimetres
  constexpr static double sNSperTimeChannel = 0.01302;
  int mAmpSumA{};
  int mAmpSumC{};
  int mAmpSum{};
  float mMeanTimeA{};
  float mMeanTimeC{};
  int mNchanA{};
  int mNchanC{};
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
  }
};
} // namespace digits
#endif