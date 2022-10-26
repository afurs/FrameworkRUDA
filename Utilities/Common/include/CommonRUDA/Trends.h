#ifndef Trends_h
#define Trends_h

#include <string>
#include <functional>
#include <iostream>

#include "TList.h"

namespace trends
{

template<typename InputType,typename HistOutputType>
struct HistTrends {
  typedef InputType Input_t;
  typedef HistOutputType Hist_t;
  std::string srcHistname{};
  std::string dstHistname{};
  std::function<double(const Input_t &obj)> funcGetValue;
  std::function<double(const Input_t &obj)> funcGetError = [](const Input_t &obj)->double{return {};};
  bool useError{false};
  Hist_t *hist{nullptr};
  void init(TList *listOutput,std::size_t nBins) {
    hist=new Hist_t(dstHistname.c_str(),dstHistname.c_str(),nBins,0,nBins);
    listOutput->Add(hist);
  }
  void fill(unsigned int binPos,const Input_t* inputObj) {
    if(inputObj==nullptr) {
      std::cout<<"\nWARNING! CANNOT FIND OBJECT: "<<srcHistname<<std::endl;
      return;
    }
    hist->SetBinContent(static_cast<int>(binPos),funcGetValue(*inputObj));
    if(useError) {
      hist->SetBinError(static_cast<int>(binPos),funcGetError(*inputObj));
    }
  }
};

template<typename std::tuple<typename... EntryTypes> TupleEntryType, typename std::tuple<typename... InitObjTypes> TupleInitObjType>
struct Table {
  typedef TupleEntryType TupleEntry_t;
  typedef TupleInitObjType TupleInitObj_t;
  
};

}