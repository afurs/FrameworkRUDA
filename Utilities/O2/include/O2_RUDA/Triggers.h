#ifndef O2_RUDA_TRIGGERS_H
#define O2_RUDA_TRIGGERS_H

#include "DataFormatsFIT/Triggers.h"

#include <string>
#include <map>

namespace triggers {
  enum EBeamMask {
    kEmpty,
    kBeam,
    kBeamA, // beamA = beam 0,
    kBeamC, // beamC = beam 1
    kAny
  };
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
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFDD = makeMapPlusLast(sMapTrgBitNamesFDD,sTrgAny,"AnyTrg");
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFT0 = makeMapPlusLast(sMapTrgBitNamesFT0,sTrgAny,"AnyTrg");
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFV0 = makeMapPlusLast(sMapTrgBitNamesFV0,sTrgAny,"AnyTrg");

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

};
}//namespace triggers

#endif