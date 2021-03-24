#ifndef HelperHists_h
#define HelperHists_h

#include "TH1.h"
#include "TH2.h"
#include "TH3.h"


namespace helpers {
namespace hists
{

  ///Not a hist
  template<typename Hist_t
           ,typename BinValueType=unsigned int
           ,typename = void>
  struct HistHelper {
    static constexpr std::size_t Ndims=0;
    typedef std::tuple<> Bin_t;
    struct HistParam {};
  };
  ///1 Dim
  template<typename Hist_t,typename BinValueType>
  struct HistHelper<Hist_t,BinValueType
      ,typename std::enable_if<std::is_base_of<TH1,Hist_t>::value
                                                     &&!std::is_base_of<TH2,Hist_t>::value
                                                     &&!std::is_base_of<TH3,Hist_t>::value>::type>
  {
    static constexpr std::size_t Ndims=1;
    typedef std::tuple<BinValueType> Bin_t;
    struct HistParam {
      HistParam() = default;
      HistParam(const HistParam&) = default;
      std::string mHistName;
      std::string mHistTitle;
      Int_t mNBinsX;
      Double_t mLowBinX;
      Double_t mUpBinX;
      /*
      static auto makeHist()->Hist_t* {
        return new Hist_t(mHistName.c_str(),mHistTitle.c_str(),mNBinsX,mLowBinX,mUpBinX);
      }
      */
    };
  };
  ///2 Dim
  template<typename Hist_t,typename BinValueType>
  struct HistHelper<Hist_t,BinValueType
      ,typename std::enable_if<std::is_base_of<TH2,Hist_t>::value
      &&!std::is_base_of<TH3,Hist_t>::value>::type>
  {
    static constexpr std::size_t Ndims=2;
    typedef std::tuple<BinValueType,BinValueType> Bin_t;
    struct HistParam {
      HistParam() = default;
      HistParam(const HistParam&) = default;
      std::string mHistName;
      std::string mHistTitle;
      Int_t mNBinsX;
      Double_t mLowBinX;
      Double_t mUpBinX;
      Int_t mNBinsY;
      Double_t mLowBinY;
      Double_t mUpBinY;
    };
  };
  ///3 Dim
  template<typename Hist_t,typename BinValueType>
  struct HistHelper<Hist_t,BinValueType
      ,typename std::enable_if<std::is_base_of<TH3,Hist_t>::value>::type>
  {
    static constexpr std::size_t Ndims=3;
    typedef std::tuple<BinValueType,BinValueType,BinValueType> Bin_t;

    struct HistParam {
      HistParam() = default;
      HistParam(const HistParam&) = default;
      std::string mHistName;
      std::string mHistTitle;
      Int_t mNBinsX;
      Double_t mLowBinX;
      Double_t mUpBinX;
      Int_t mNBinsY;
      Double_t mLowBinY;
      Double_t mUpBinY;
      Int_t mNBinsZ;
      Double_t mLowBinZ;
      Double_t mUpBinZ;
    };
  };
} //namespace hists
} //namespace helpers
#endif
