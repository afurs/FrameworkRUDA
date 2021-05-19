#ifndef DataModel_h
#define DataModel_h
#include "HelperHists.h"

#include <string>
#include <vector>
#include <functional>
#include <iostream>
template<typename DataType,typename EventCutIDtype>
struct DataInput:DataType {
  typedef DataType Data_t;
  typedef EventCutIDtype EventCutID_t;
  EventCutID_t mEventCutID;
};

template<typename DataType,typename DataOutputType>
class DataOutputObject {
 public:
  typedef DataOutputObject<DataType,DataOutputType> DataOutputObject_t;
  typedef DataType Data_t;
  typedef typename DataType::EventCutID_t EventCutID_t;
  typedef DataOutputType DataOutput_t;
  typedef std::function<void(const Data_t&,DataOutputObject_t *)> Functor_t;
  typedef typename EventCutID_t::template EventCutIDmap<unsigned int> MapEventCutIDtoBin_t;
  DataOutputObject(Data_t &data,std::string name,std::string title,const std::vector<EventCutID_t> &vecEventCutIDtoBin,const EventCutID_t eventCutID = {}):
    mRefData(data),mEventCutID(eventCutID)
  {
    mIsEventCutIDCounter=true;
    prepareStatHist<DataOutputType>(name,title,vecEventCutIDtoBin);
  }
  /*******************************************************************************************************************/
  DataOutputObject(Data_t &data,DataOutput_t &dataOutput,const Functor_t &func):
    mRefData(data),mDataOutput(dataOutput), mFuncDataFill(func),mEventCutID(EventCutID_t{})
  {
     mIsEventCutIDCounter=false;
  }
  DataOutputObject(Data_t &data,DataOutput_t &dataOutput,const Functor_t &func,const EventCutID_t &eventCutID):
    mRefData(data),mDataOutput(dataOutput), mFuncDataFill(func),mEventCutID(eventCutID)
  {
    mIsEventCutIDCounter=false;
  }
  /*******************************************************************************************************************/
  template<typename T,
    typename std::enable_if_t<helpers::hists::HistHelper<T>::Ndims==1,bool> = true>
  void prepareStatHist(std::string name,std::string title,const std::vector<EventCutID_t> &vecEventCutIDtoBin) {
    mFuncDataFill = [](const Data_t& data,DataOutputObject_t *outputPtr)->void {outputPtr->fillData<T>(data.mEventCutID);};
    int nBin=1;
    for(const auto& entry: vecEventCutIDtoBin)  {
      auto [it, isNew] = mMapEventCutIDToBin.insert({entry,nBin});
      if(isNew) nBin++;
    }
    nBin--;
    mDataOutput = DataOutput_t{name.c_str(),title.c_str(),nBin,0,static_cast<double>(nBin)};
    for(const auto& entry: mMapEventCutIDToBin) {
      mDataOutput.GetXaxis()->SetBinLabel(entry.second,entry.first.mEventCutIDname.c_str());
    }
  }

//  template<typename std::enable_if_t<helpers::hists::HistHelper<DataOutputType>::Ndims!=1,bool> = true>
  template<typename T,
      typename std::enable_if_t<helpers::hists::HistHelper<T>::Ndims!=1,bool> = true  >
  void prepareStatHist(std::string name,std::string title,const std::vector<EventCutID_t> &vecEventCutIDtoBin) {}

  //Reference for data;
  std::reference_wrapper<Data_t> mRefData;
  //Reference for output;
  //std::reference_wrapper<DataOutput_t> mDataOutput;
  DataOutput_t mDataOutput;
  //EventCutID for filling data
  const EventCutID_t mEventCutID;
  Functor_t mFuncDataFill;
  /*
  const Functor_t mFuncDataFillEventCutIDbins = [](const Data_t& data,DataOutputObject_t *outputPtr) {
    for(const auto& entry: (outputPtr->mMapEventCutIDToBin)) {
      if(entry.first.isAcceptable(data.mEventCutID)) {
       outputPtr->mDataOutput.Fill(outputPtr->mDataOutput.GetBinCenter(entry.second));
      }
    }
  };
  */
  bool mIsEventCutIDCounter;
  MapEventCutIDtoBin_t mMapEventCutIDToBin;
  void print() const {mDataOutput.Print();}


  /*******************************************************************************************************************/
  //template<typename = typename std::enable_if_t<helpers::hists::HistHelper<DataOutputType>::Ndims!=1 > >
  //void fillData(const EventCutID_t &eventCutID);

  //template<typename = typename std::enable_if_t<helpers::hists::HistHelper<DataOutputType>::Ndims==1 > >
  template<typename T>
  auto fillData(const EventCutID_t &eventCutID)-> std::enable_if_t<helpers::hists::HistHelper<T>::Ndims==1 >
  {
    for(const auto& entry: mMapEventCutIDToBin) {
      if(entry.first.isAcceptable(eventCutID)) {
        mDataOutput.Fill(mDataOutput.GetBinCenter(entry.second));
      }
    }
  }

  auto fillData(const EventCutID_t &eventCutID)-> void
  {}
  /*******************************************************************************************************************/

  void fillData() {
    mFuncDataFill(mRefData,this);
  }
};

template<typename DataOutputType>
class DataOutputManagerBase {
 public:
  typedef DataOutputType DataOutput_t;
  typedef typename DataOutputType::EventCutID_t EventCutID_t;
  typedef typename DataOutputType::Data_t Data_t;
  DataOutputManagerBase(Data_t &data):mRefData(data) {}
/* {
    //this->makeDataOutput(DataOutput_t{mRefData,dataOutputArgs...});
    DataOutput_t dataOutput{mRefData,dataOutputArgs...};
    std::cout<<"\nMAKING DATA OUTPUT\n";
    dataOutput.print();
    if(dataOutput.mEventCutID==EventCutID_t{}) {
      mVecDataOutput.push_back(dataOutput);
    }
    else {
      auto [it, isNew] = mMapEventCutIDToDataOutput.insert({dataOutput.mEventCutID,std::vector<DataOutput_t>{}});
      it->second.push_back(dataOutput);
  //      mMapEventCutIDToDataOutput.insert({dataOutput.mEventCutID,dataOutput});
    }
  }*/
  /*
  template<typename... DataOutputConstructorArgs>
  void makeDataOutput(DataOutputConstructorArgs... dataOutputArgs) {
    this->makeDataOutput(DataOutput_t{mRefData,dataOutputArgs...});
  }
  */
  /*
  void setMapEventCutIDtoBin(const typename DataOutput_t::MapEventCutIDtoBin_t &mapEventCutIDtoBin) {
    for(auto &entry: mVecDataOutput) {
      if(entry.mIsEventCutIDCounter) entry.mMapEventCutIDToBin=mapEventCutIDtoBin;
    }
    for(auto& entryCountID: mMapEventCutIDToDataOutput) {
      for(auto& entry:entryCountID.second) {
        entry.mMapEventCutIDToBin=mapEventCutIDtoBin;
      }
    }
  }
  */
  std::reference_wrapper<Data_t> mRefData;
  //Vector of output data withous cuts
  std::vector<DataOutput_t> mVecDataOutput;
  //Map of vector with output data(std::vector<DataOutput_t> as key) correcpondong to given EventCutID_t(key)
  typename EventCutID_t::template EventCutIDmap<std::vector<DataOutput_t>> mMapEventCutIDToDataOutput;



  void fillData() {
    for(auto& entry:mVecDataOutput )  entry.fillData();
    for(auto& entryCountID: mMapEventCutIDToDataOutput) {
      if(entryCountID.first.isAcceptable(mRefData.get().mEventCutID)) {
        for(auto& entry:entryCountID.second)  entry.fillData();
        //entryCountID.second.fillData();
      }
    }
  }
  /*******************************************************************************************************************/
  void init() {

  }
  /*******************************************************************************************************************/

  template<typename... DataOutputConstructorArgs>
  void makeDataOutput(DataOutputConstructorArgs... dataOutputArgs) {
    //this->makeDataOutput(DataOutput_t{mRefData,dataOutputArgs...});
    DataOutput_t dataOutput{mRefData,dataOutputArgs...};
    std::cout<<"\nMAKING DATA OUTPUT\n";
    dataOutput.print();
    if(dataOutput.mEventCutID==EventCutID_t{}) {
      mVecDataOutput.push_back(dataOutput);
    }
    else {
      auto [it, isNew] = mMapEventCutIDToDataOutput.insert({dataOutput.mEventCutID,std::vector<DataOutput_t>{}});
      it->second.push_back(dataOutput);
  //      mMapEventCutIDToDataOutput.insert({dataOutput.mEventCutID,dataOutput});
    }
  }
 protected:
  //void makeDataOutput(DataOutput_t dataOutput);
};

template<typename DataType,typename DataOutputType>
using DataOutputManager =DataOutputManagerBase<DataOutputObject<DataType,DataOutputType>>;

#endif
