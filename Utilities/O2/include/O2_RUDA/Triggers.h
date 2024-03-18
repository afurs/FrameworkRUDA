#ifndef O2_RUDA_TRIGGERS_H
#define O2_RUDA_TRIGGERS_H

#include "DataFormatsFIT/Triggers.h"
#include "DataFormatsFT0/ChannelData.h"

#include <string>
#include <map>
#include <utility>
namespace triggers {

template<typename... TrgBits>
constexpr uint64_t MakeTrgWord(TrgBits&&... trgBits) {
  return ((1ull<<std::forward<TrgBits>(trgBits)) | ...);
}
template<typename TrgWord, typename... TrgBits>
bool CheckBits(TrgWord &&trigger_word, TrgBits&&... trgBits) {
  const auto checkWord = MakeTrgWord(trgBits...);
  return (trigger_word & checkWord) == checkWord;
}

template<typename TrgWord, typename... TrgBits>
bool CheckExactBits(TrgWord &&trigger_word, TrgBits&&... trgBits) {
  const auto checkWord = MakeTrgWord(trgBits...);
  return (trigger_word & checkWord) == trigger_word;
}



struct Trigger {
  static const inline std::map<unsigned int, std::string> sMapTrgBitNamesFDD = {
    {o2::fit::Triggers::bitA, "OrA"},
    {o2::fit::Triggers::bitC, "OrC"},
    {o2::fit::Triggers::bitCen, "Central"},
    {o2::fit::Triggers::bitSCen, "Semicentral"},
    {o2::fit::Triggers::bitVertex, "Vertex"}};

  static const inline std::map<unsigned int, std::string> sMapTrgBitNamesFT0 = {
    {o2::fit::Triggers::bitA, "OrA"},
    {o2::fit::Triggers::bitC, "OrC"},
    {o2::fit::Triggers::bitCen, "Central"},
    {o2::fit::Triggers::bitSCen, "Semicentral"},
    {o2::fit::Triggers::bitVertex, "Vertex"}};

  static const inline std::map<unsigned int, std::string> sMapTrgBitNamesFV0 = {
    {o2::fit::Triggers::bitA, "OrA" },
    {o2::fit::Triggers::bitAOut, "OrAOut" },
    {o2::fit::Triggers::bitTrgNchan, "TrgNChan" },
    {o2::fit::Triggers::bitTrgCharge, "TrgCharge" },
    {o2::fit::Triggers::bitAIn, "OrAIn" }};
  static const inline unsigned int sTrgAny = sMapTrgBitNamesFT0.size();


  static std::map<unsigned int, std::string> makeMapPlusLast(const std::map<unsigned int, std::string> &mapSrc,unsigned int elPos,const std::string &elName) {
    std::map<unsigned int, std::string> mapResult = mapSrc;
    mapResult.insert({elPos,elName});
    return mapResult;
  }

  static std::map<unsigned int, std::string> makeMapAddLast(const std::map<unsigned int, std::string> &mapSrc,unsigned int elPos,const std::string &elName) {
    const auto elemPos = mapSrc.size() > 0 ? (--mapSrc.end())->first : 0;
    return makeMapPlusLast(mapSrc, elPos, elName);
  }

  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFDD = makeMapPlusLast(sMapTrgBitNamesFDD,sTrgAny,"AnyTrg");
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFT0 = makeMapPlusLast(sMapTrgBitNamesFT0,sTrgAny,"AnyTrg");
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFV0 = makeMapPlusLast(sMapTrgBitNamesFV0,sTrgAny,"AnyTrg");
/*
  static const inline std::map<unsigned int,std::string> mMapBeamMask{{EBeamMask::kEmpty,"Empty"},
                                                          {EBeamMask::kBeam,"BeamBeam"},
                                                          {EBeamMask::kBeamA,"BeamA"},
                                                          {EBeamMask::kBeamC,"BeamC"},
                                                          {EBeamMask::kAny,"AnyBeam"}};
  static const inline unsigned int sNbeamMasks = mMapBeamMask.size();
  static const inline unsigned int sNallTrgBits = sMapAllTrgBitNamesFT0.size();
  
  static const inline std::map<unsigned int,std::string> mMapBeamMaskBasic{{EBeamMask::kEmpty,"Empty"},
                                                          {EBeamMask::kBeam,"BeamBeam"},
                                                          {EBeamMask::kBeamA,"BeamA"},
                                                          {EBeamMask::kBeamC,"BeamC"}};

  static unsigned int hashBeamMaskTrgBit(unsigned int beamMask,unsigned int trgBitPos) {
    return beamMask * sNallTrgBits + trgBitPos;
  }

  static void getBeamMaskTrgBit(unsigned int hashValue, unsigned int &beamMask,unsigned int &trgBit) {
    beamMask = hashValue / sNallTrgBits;
    trgBit = hashValue % sNallTrgBits;
  }

  static void getBeamMaskTrgBitNamesFDD(unsigned int hashValue, std::string &beamMaskName,std::string &trgBitName) {
    unsigned int beamMask;
    unsigned int trgBit;
    getBeamMaskTrgBit(hashValue,beamMask,trgBit);
    beamMaskName = mMapBeamMask.find(beamMask)->second;
    trgBitName = sMapAllTrgBitNamesFDD.find(trgBit)->second;
  }
  static void getBeamMaskTrgBitNamesFT0(unsigned int hashValue, std::string &beamMaskName,std::string &trgBitName) {
    unsigned int beamMask;
    unsigned int trgBit;
    getBeamMaskTrgBit(hashValue,beamMask,trgBit);
    beamMaskName = mMapBeamMask.find(beamMask)->second;
    trgBitName = sMapAllTrgBitNamesFT0.find(trgBit)->second;
  }
  static void getBeamMaskTrgBitNamesFV0(unsigned int hashValue, std::string &beamMaskName,std::string &trgBitName) {
    unsigned int beamMask;
    unsigned int trgBit;
    getBeamMaskTrgBit(hashValue,beamMask,trgBit);
    beamMaskName = mMapBeamMask.find(beamMask)->second;
    trgBitName = sMapAllTrgBitNamesFV0.find(trgBit)->second;
  }
*/
};
struct PMbits {
  static const inline std::map<unsigned int, std::string>  sMapPMbits = {
    {o2::ft0::ChannelData::kNumberADC, "NumberADC" },
    {o2::ft0::ChannelData::kIsDoubleEvent, "IsDoubleEvent" },
    {o2::ft0::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" },
    {o2::ft0::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" },
    {o2::ft0::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" },
    {o2::ft0::ChannelData::kIsAmpHigh, "IsAmpHigh" },
    {o2::ft0::ChannelData::kIsEventInTVDC, "IsEventInTVDC" },
    {o2::ft0::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" }};
  };
}//namespace triggers

#endif