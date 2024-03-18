#ifndef HelperHists_h
#define HelperHists_h

#include <map>
#include <string>

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
  struct Axis {
    Axis(int nBins,double lowBin, double upBin, const std::string &title="", const std::map<unsigned int,std::string> &binLabels={}) :
    mNBins(nBins),mLowBin(lowBin),mUpBin(upBin), mTitle(title), mBinLabels(binLabels) {}
    Axis(const std::string &title="",const std::map<unsigned int,std::string> &binLabels={}) :
    mNBins(binLabels.size()),mLowBin(0),mUpBin(binLabels.size()), mTitle(title), mBinLabels(binLabels) {}

    ~Axis() = default;
    int mNBins;
    double mLowBin;
    double mUpBin;
    std::string mTitle{};
    std::map<unsigned int,std::string> mBinLabels{};
    void prepareAxis(TAxis *axis) const {
      if(mTitle.size()>0) {
        axis->SetTitle(mTitle.c_str());
      }
      for(const auto &binEntry: mBinLabels) {
        const auto &binIdx = binEntry.first;
        const auto &binName = binEntry.second;
        axis->SetBinLabel(binIdx + 1,binName.c_str());
      }
    }
  };
  template<typename HistType> std::decay_t<HistType>* makeHist (const std::string &name, const std::string &title, const Axis &axisX) {
    auto hist = new std::decay_t<HistType> (name.c_str(), title.c_str(), axisX.mNBins, axisX.mLowBin, axisX.mUpBin);
    axisX.prepareAxis(hist->GetXaxis());
    return hist;
  }

  template<typename HistType> std::decay_t<HistType>* makeHist (const std::string &name, const std::string &title, const Axis &axisX, const Axis &axisY) {
    auto hist = new std::decay_t<HistType> (name.c_str(), title.c_str(), axisX.mNBins, axisX.mLowBin, axisX.mUpBin, axisY.mNBins, axisY.mLowBin, axisY.mUpBin);
    axisX.prepareAxis(hist->GetXaxis());
    axisY.prepareAxis(hist->GetYaxis());

    return hist;
  }

  template<typename HistType> std::decay_t<HistType>* makeHist (const std::string &name, const std::string &title, const Axis &axisX, const Axis &axisY, const Axis &axisZ) {
    auto hist = new std::decay_t<HistType> (name.c_str(), title.c_str(), axisX.mNBins, axisX.mLowBin, axisX.mUpBin, axisY.mNBins, axisY.mLowBin, axisY.mUpBin, axisZ.mNBins, axisZ.mLowBin, axisZ.mUpBin);
    axisX.prepareAxis(hist->GetXaxis());
    axisY.prepareAxis(hist->GetYaxis());
    axisZ.prepareAxis(hist->GetZaxis());
    return hist;
  }

} //namespace hists
} //namespace helpers
#endif
