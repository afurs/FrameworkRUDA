#ifndef Constants_H
#define Constants_H

namespace constants {
  constexpr double sNS2Cm = 29.97; // light NS to Centimetres
  constexpr double sTDC2NS = 0.01302;
  constexpr double sTDC2Cm = sNS2Cm * sTDC2NS;
};

#endif