// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef O2_FT0_TIMESPECTRAINFOOBJECT_H
#define O2_FT0_TIMESPECTRAINFOOBJECT_H

#include <array>
#include "Rtypes.h"
#include "FT0Base/Geometry.h"

namespace o2::ft0
{

struct TimeSpectraInfoObject {

  std::array<float, o2::ft0::Geometry::Nchannels> mTimeFitMean{}; // Peak of Gausian fitting
  std::array<float, o2::ft0::Geometry::Nchannels> mTimeFitRMS{}; // RMS of Gausian fitting
  std::array<float, o2::ft0::Geometry::Nchannels> mTimeFitChi2{}; // Chi2 of Gausian fitting
  std::array<float, o2::ft0::Geometry::Nchannels> mTimeStatMean{}; // Spectra mean
  std::array<float, o2::ft0::Geometry::Nchannels> mTimeStatRMS{}; // Spectra RMD
  std::array<float, o2::ft0::Geometry::Nchannels> mTimeStat{}; // Statistic
  std::array<uint8_t, o2::ft0::Geometry::Nchannels> mTimeStatusBits{} // Status bits for extra info

  static constexpr const char* getObjectPath() { return "FT0/Calib/ChannelTimeOffset"; }
  float mTimeA;
  float mTimeC;
  ClassDefNV(FT0ChannelTimeCalibrationObject, 2);
};

} // namespace o2::ft0

#endif // O2_FT0_TIMESPECTRAINFOOBJECT_H
