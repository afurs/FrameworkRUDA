#ifndef O2_RUDA_TRIGGERS_H
#define O2_RUDA_TRIGGERS_H

#include "DataFormatsFIT/Triggers.h"

#include <string>
#include <map>

namespace triggers {
struct Trigger {
  static const inline std::map<unsigned int, std::string> sMapTrgBitNamesFDD = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Central"},
    {o2::fit::Triggers::bitSCen+1, "Semicentral"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"}};

  static const inline std::map<unsigned int, std::string> sMapTrgBitNamesFT0 = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Central"},
    {o2::fit::Triggers::bitSCen+1, "Semicentral"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"}};

  static const inline std::map<unsigned int, std::string> sMapTrgBitNamesFV0 = {
    {o2::fit::Triggers::bitA+1, "OrA" },
    {o2::fit::Triggers::bitAOut+1, "OrAOut" },
    {o2::fit::Triggers::bitTrgNchan+1, "TrgNChan" },
    {o2::fit::Triggers::bitTrgCharge+1, "TrgCharge" },
    {o2::fit::Triggers::bitAIn+1, "OrAIn" }};
  static const inline unsigned int sTrgAny = sMapTrgBitNamesFT0.size();


  static std::map<unsigned int, std::string> makeMapPlusLast(const std::map<unsigned int, std::string> &mapSrc,unsigned int elPos,const std::string &elName) {
    std::map<unsigned int, std::string> mapResult = mapSrc;
    mapResult.insert({elPos+1,elName});
    return mapResult;
  }
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFDD = makeMapPlusLast(sMapTrgBitNamesFDD,sTrgAny,"AnyTrg");
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFT0 = makeMapPlusLast(sMapTrgBitNamesFT0,sTrgAny,"AnyTrg");
  static const inline std::map<unsigned int, std::string> sMapAllTrgBitNamesFV0 = makeMapPlusLast(sMapTrgBitNamesFV0,sTrgAny,"AnyTrg");

  enum EBeamMask {
    kEmpty,
    kBeam,
    kBeamA, // beamA = beam 0,
    kBeamC, // beamC = beam 1
    kAny
  };
  static const inline std::map<unsigned int,std::string> mMapBeamMask{{EBeamMask::kEmpty+1,"Empty"},
                                                          {EBeamMask::kBeam+1,"BeamBeam"},
                                                          {EBeamMask::kBeamA+1,"BeamA"},
                                                          {EBeamMask::kBeamC+1,"BeamC"},
                                                          {EBeamMask::kAny+1,"AnyBeam"}};
  static const inline unsigned int sNbeamMasks = mMapBeamMask.size();
  static const inline unsigned int sNallTrgBits = sMapAllTrgBitNamesFT0.size();
  
  static const inline std::map<unsigned int,std::string> mMapBeamMaskBasic{{EBeamMask::kEmpty+1,"Empty"},
                                                          {EBeamMask::kBeam+1,"BeamBeam"},
                                                          {EBeamMask::kBeamA+1,"BeamA"},
                                                          {EBeamMask::kBeamC+1,"BeamC"}};

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
    beamMaskName = mMapBeamMask.find(beamMask+1)->second;
    trgBitName = sMapAllTrgBitNamesFDD.find(trgBit+1)->second;
  }
  static void getBeamMaskTrgBitNamesFT0(unsigned int hashValue, std::string &beamMaskName,std::string &trgBitName) {
    unsigned int beamMask;
    unsigned int trgBit;
    getBeamMaskTrgBit(hashValue,beamMask,trgBit);
    beamMaskName = mMapBeamMask.find(beamMask+1)->second;
    trgBitName = sMapAllTrgBitNamesFT0.find(trgBit+1)->second;
  }
  static void getBeamMaskTrgBitNamesFV0(unsigned int hashValue, std::string &beamMaskName,std::string &trgBitName) {
    unsigned int beamMask;
    unsigned int trgBit;
    getBeamMaskTrgBit(hashValue,beamMask,trgBit);
    beamMaskName = mMapBeamMask.find(beamMask+1)->second;
    trgBitName = sMapAllTrgBitNamesFV0.find(trgBit+1)->second;
  }

};
}//namespace triggers

#endif