#ifndef RUDA_O2_TIMESERIES_H
#define RUDA_O2_TIMESERIES_H

#include "CCDB.h"
#include "CommonRUDA/HistUtils.h"

namespace utilities {
namespace timeseries {

struct TimeseriesRun {

  void init(const utilities::ccdb::EntryCCDB &entryCCDB) {
  
  
  }
  constexpr static double sOrbitToSec = 25.e-9 * 3564; // factor for converting orbits to seconds
  uint32_t mFirstOrbit {0};
  uint32_t mLastOrbit {0};
  
  //Orbit limits for timeseries
  const uint32_t firstOrbit = entryCCDB.mFirstOrbit - entryCCDB.mFirstOrbit%sOrbitPerTF;//start from beginning of first TF
  const uint32_t lastOrbit = entryCCDB.mLastOrbit + entryCCDB.mLastOrbit%sOrbitPerTF;
  auto orbit2Sec = [](uint32_t orbit) {
//    uint64_t sec = orbit/(sOrbitPerTF * sTFrate);
    uint64_t sec = uint64_t(1e-9*orbit*3564.*25.);
    return sec;
  };

  const uint64_t firstTF = firstOrbit/sOrbitPerTF - 1;
  const uint64_t lastTF = lastOrbit/sOrbitPerTF + 1;
  const uint64_t firstSec = orbit2Sec(firstOrbit);
  const uint64_t lastSec = orbit2Sec(lastOrbit);

  auto getAxisTs = [] (int binWidth,uint64_t firstSec,uint64_t lastSec,const std::string &title) {
    const auto resudials = (lastSec-firstSec)%binWidth;
    const auto nBins=(lastSec-firstSec)/binWidth+int(resudials>0);
    return Axis(nBins,firstSec,lastSec-resudials+binWidth,title);
  };

};
} //namespace timeseries
} //namespace utilities

#endif