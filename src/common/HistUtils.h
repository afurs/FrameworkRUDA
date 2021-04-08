#ifndef HistUtils_h
#define HistUtils_h
#include <iostream>
#include <regex>

#include "TList.h"

#include "HelperHists.h"
#include "HelperCommon.h"

//#include "../helpers/HelperHists.h"
//#include "../helpers/HelperCommon.h"


namespace utilities
{
class Hists {
  public:

//  template<typename... T>
//  using HistHelper = typename helpers::hists::HistHelper<T...>;

//  template<typename... T>
//  using IsSpecOf = typename helpers::common::IsSpecOf<T...>;

  Hists()=delete;
  /*******************************************************************************************************************/
/*
  template<typename HistOutput_t,typename HistInput_t,typename>
  static TList* decomposeHist(HistInput_t *hist,bool setErrorBin=false);

  template<typename Hist_t>
  static TList* makeDividedHists(TList &listInput
                                       , std::vector<std::tuple<std::string, std::string, std::string> > vecTupleNames
                                       , Option_t *option="",bool copyBinNames=false);


  template<typename Hist_t
           ,typename = typename std::enable_if<std::is_base_of<TH1,Hist_t>::value>::type>
  static void makeHistBinNamed(Hist_t *hist, const std::map<unsigned int,std::string> &mapBinNames);

  template<typename Hist_t, typename Map_t, typename, typename>
  static auto makeHist1DFromMap(const Map_t &mapValues,std::string histName,std::string histTitle,bool doLabeling=false)->Hist_t*;

  template<typename Hist_t,typename Map_t,typename,typename>
  static auto makeHistFromMap(const Map_t &mapValues,std::string histName,std::string histTitle,bool doLabeling=false)->Hist_t*;
  template<typename Value_t=double,typename Hist_t>
  static auto makeMapFromHist(const Hist_t &hist);
  */
  template<typename HistOutput_t
           ,typename HistInput_t
           ,typename = typename std::enable_if<helpers::hists::HistHelper<HistInput_t>::Ndims
                                               ==helpers::hists::HistHelper<HistOutput_t>::Ndims+1
                                               && ((helpers::hists::HistHelper<HistOutput_t>::Ndims)>0) >::type>
  static TList* decomposeHist(HistInput_t *hist,bool setErrorBin) {
    if(hist==nullptr) return nullptr;
    std::string nameSrcHist = hist->GetName();
    std::string titleSrcHist = hist->GetTitle();
    unsigned int nBinsX = hist->GetXaxis()->GetNbins();
    double minX = hist->GetXaxis()->GetXmin();
    double maxX = hist->GetXaxis()->GetXmax();
    unsigned int nBinsY = hist->GetYaxis()->GetNbins();
    double minY = hist->GetYaxis()->GetXmin();
    double maxY = hist->GetYaxis()->GetXmax();
    TList *listOutput = new TList();
    for(int iBinX=1;iBinX<nBinsX+1;iBinX++) {
      std::string nameDestHist = nameSrcHist+Form("_b%i",iBinX);
      std::string titleDestHist = titleSrcHist;
      HistOutput_t *histOutput = new HistOutput_t(nameDestHist.c_str(),titleDestHist.c_str(),nBinsY,minY,maxY);
      listOutput->Add(histOutput);
      for(int iBinY=1;iBinY<nBinsY+1;iBinY++) {
        histOutput->SetBinContent(iBinY,hist->GetBinContent(iBinX,iBinY));
        if(setErrorBin) histOutput->SetBinError(iBinY,hist->GetBinError(iBinX,iBinY));
        auto nEntries = histOutput->GetEntries()+hist->GetBinContent(iBinX,iBinY)-1;
        histOutput->SetEntries(nEntries);
      }
    }
    return listOutput;
  }
  /*******************************************************************************************************************/
  template<typename Hist_t>
  static TList* makeDividedHists(TList &listInput
                                       , std::vector<std::tuple<std::string, std::string, std::string> > vecTupleNames
                                       , Option_t *option,bool copyBinNames)
  {
    TList *listResult = new TList();
    for(const auto&entry:vecTupleNames) {
      Hist_t *h1 = dynamic_cast<Hist_t*>(listInput.FindObject(std::get<0>(entry).c_str()));
      if(h1==nullptr) continue;
      if(h1->GetEntries()==0) continue;
      Hist_t *h2 = dynamic_cast<Hist_t*>(listInput.FindObject(std::get<1>(entry).c_str()));
      if(h2==nullptr) continue;
      if(h2->GetEntries()==0) continue;
      TAxis *axis = h1->GetXaxis();
      Hist_t *h3 = dynamic_cast<Hist_t *>(h1->Clone(std::get<2>(entry).c_str()));
      h3->SetTitle(std::get<2>(entry).c_str());
      for(int iBin=1;iBin<axis->GetNbins()+1;iBin++)  {
        TString stBinName = axis->GetBinLabel(iBin);
        if(stBinName.EqualTo("")||!copyBinNames) continue;
        h3->GetXaxis()->SetBinLabel(iBin,stBinName);
      }
      listResult->Add(h3);
      h3->Divide(h1,h2,1,1,option);
    }
    return listResult;
  }
  /*******************************************************************************************************************/
  template<typename Hist_t
           ,typename = typename std::enable_if<std::is_base_of<TH1,Hist_t>::value>::type>
  static void makeHistBinNamed(Hist_t *hist, const std::map<unsigned int,std::string> &mapBinNames) {
    if(hist==nullptr||mapBinNames.size()==0)  return;
    TAxis *axis = hist->GetXaxis();
    for(const auto &binName:mapBinNames) {
      axis->SetBinLabel(binName.first,binName.second.c_str());
    }
  }
  /*******************************************************************************************************************/
  template<typename Hist_t
           ,typename Map_t
           ,typename = typename std::enable_if<helpers::hists::HistHelper<Hist_t>::Ndims==1>::type
           ,typename = typename std::enable_if<std::is_arithmetic<typename Map_t::key_type>::value
                                               ||std::is_same<const char*,typename Map_t::key_type>::value
                                               ||std::is_base_of<TString,typename Map_t::key_type>::value
                                               ||std::is_base_of<std::string,typename Map_t::key_type>::value
                                               ||helpers::common::IsSpecOf<std::tuple,typename Map_t::mapped_type>::value
                                               >::type
           >
  static auto makeHist1DFromMap(const Map_t &mapValues,std::string histName,std::string histTitle,bool doLabeling)->Hist_t* {
    if(mapValues.size()==0) return nullptr;
    Hist_t *resultHist;
    resultHist = new Hist_t(histName.c_str(),histTitle.c_str(),mapValues.size(),0,mapValues.size());
    unsigned int binPos=1;
    for(const auto &entry:mapValues) {
      if constexpr(!(helpers::common::IsSpecOf<std::tuple,typename Map_t::mapped_type>::value)) {
        resultHist->SetBinContent(binPos,entry.second);
      }
      else if constexpr(std::tuple_size<typename Map_t::mapped_type>::value==2)  {
        resultHist->SetBinContent(binPos,std::get<0>(entry.second));
        resultHist->SetBinError(binPos,std::get<1>(entry.second));
      }
      if(doLabeling)  {
        if constexpr(std::is_arithmetic<typename Map_t::key_type>::value)
        {
          resultHist->GetXaxis()->SetBinLabel(binPos,std::to_string(entry.first).c_str());
        }
        else if constexpr(std::is_same<const char*,typename Map_t::key_type>::value
                          ||std::is_base_of<TString,typename Map_t::key_type>::value)
        {
          resultHist->GetXaxis()->SetBinLabel(binPos,entry.first);
        }
        else if constexpr(std::is_base_of<std::string,typename Map_t::key_type>::value) {
          resultHist->GetXaxis()->SetBinLabel(binPos,(entry.first).c_str());
        }
        else {
          std::cout<<"\nWarning! Incomfortable type for bin labeling!\n";
        }
      }
      binPos++;
    }
    return resultHist;
  }
  /*******************************************************************************************************************/
  template<typename Hist_t
           ,typename Map_t
           ,typename = typename std::enable_if<std::is_base_of<TH1,Hist_t>::value>::type
           ,typename = typename std::enable_if<std::is_same<typename helpers::hists::HistHelper<Hist_t>::Bin_t
                                                            ,typename Map_t::key_type>::value>::type
           >
  static auto makeHistFromMap(const Map_t &mapValues,std::string histName,std::string histTitle,bool doLabeling)->Hist_t*{
    return Hist_t{};
  }
  /*******************************************************************************************************************/
  template<typename Value_t=double,typename Hist_t>
  auto makeMapFromHist(const Hist_t &hist) {
    if constexpr(std::is_base_of<TH3,Hist_t>::value) {
      typedef std::tuple<unsigned int,unsigned int,unsigned int> Bin_t;
      typedef std::map<Bin_t,Value_t> Map_t;
      Map_t mapOutput;
      unsigned int nBinX = hist.GetXaxis()->GetNbins();
      unsigned int nBinY = hist.GetYaxis()->GetNbins();
      unsigned int nBinZ = hist.GetZaxis()->GetNbins();
      for(int iBinX=1;iBinX<nBinX+1;iBinX++) {
        for(int iBinY=1;iBinY<nBinY+1;iBinY++) {
          for(int iBinZ=1;iBinZ<nBinZ+1;iBinZ++) {
            double value = hist.GetBinContent(iBinX,iBinY,iBinZ);
            mapOutput.insert({{iBinX,iBinY,iBinZ},value});
          }
        }
      }
      return std::move(mapOutput);
    }
    else if constexpr(std::is_base_of<TH2,Hist_t>::value) {
      typedef std::tuple<unsigned int,unsigned int> Bin_t;
      typedef std::map<Bin_t,Value_t> Map_t;
      Map_t mapOutput;
      unsigned int nBinX = hist.GetXaxis()->GetNbins();
      unsigned int nBinY = hist.GetYaxis()->GetNbins();
      for(int iBinX=1;iBinX<nBinX+1;iBinX++) {
        for(int iBinY=1;iBinY<nBinY+1;iBinY++) {
          double value = hist.GetBinContent(iBinX,iBinY);
          mapOutput.insert({{iBinX,iBinY},value});
        }
      }
      return std::move(mapOutput);
    }
    else if constexpr(std::is_base_of<TH1,Hist_t>::value) {
      typedef std::tuple<unsigned int> Bin_t;
      typedef std::map<Bin_t,Value_t> Map_t;
      Map_t mapOutput;
      unsigned int nBinX = hist.GetXaxis()->GetNbins();
      for(int iBinX=1;iBinX<nBinX+1;iBinX++) {
        double value = hist.GetBinContent(iBinX);
        mapOutput.insert({{iBinX},value});
      }
      return std::move(mapOutput);
    }
    else {
      return std::nullptr_t{};
    }
  }
  /*******************************************************************************************************************
  template<typename Value_t=double,typename Hist_t>
  static auto makeMapFromHist(const Hist_t &hist) {
    if constexpr(std::is_base_of<TH3,Hist_t>::value) {
      typedef std::tuple<unsigned int,unsigned int,unsigned int> Bin_t;
      typedef std::map<Bin_t,Value_t> Map_t;
      Map_t mapOutput;
      unsigned int nBinX = hist.GetXaxis()->GetNbins();
      unsigned int nBinY = hist.GetYaxis()->GetNbins();
      unsigned int nBinZ = hist.GetZaxis()->GetNbins();
      for(int iBinX=1;iBinX<nBinX+1;iBinX++) {
        for(int iBinY=1;iBinY<nBinY+1;iBinY++) {
          for(int iBinZ=1;iBinZ<nBinZ+1;iBinZ++) {
            double value = hist.GetBinContent(iBinX,iBinY,iBinZ);
            mapOutput.insert({{iBinX,iBinY,iBinZ},value});
          }
        }
      }
      return std::move(mapOutput);
    }
    else if constexpr(std::is_base_of<TH2,Hist_t>::value) {
      typedef std::tuple<unsigned int,unsigned int> Bin_t;
      typedef std::map<Bin_t,Value_t> Map_t;
      Map_t mapOutput;
      unsigned int nBinX = hist.GetXaxis()->GetNbins();
      unsigned int nBinY = hist.GetYaxis()->GetNbins();
      for(int iBinX=1;iBinX<nBinX+1;iBinX++) {
        for(int iBinY=1;iBinY<nBinY+1;iBinY++) {
          double value = hist.GetBinContent(iBinX,iBinY);
          mapOutput.insert({{iBinX,iBinY},value});
        }
      }
      return std::move(mapOutput);
    }
    else if constexpr(std::is_base_of<TH1,Hist_t>::value) {
      typedef std::tuple<unsigned int> Bin_t;
      typedef std::map<Bin_t,Value_t> Map_t;
      Map_t mapOutput;
      unsigned int nBinX = hist.GetXaxis()->GetNbins();
      for(int iBinX=1;iBinX<nBinX+1;iBinX++) {
        double value = hist.GetBinContent(iBinX);
        mapOutput.insert({{iBinX},value});
      }
      return std::move(mapOutput);
    }
    else {
      return std::nullptr_t{};
    }
  }
  *******************************************************************************************************************/
  static TList * makeListHists1D(std::map<std::string,std::string> mapHistNameTitle,int nBins, double xMin,double xMax);
};
}
#endif
