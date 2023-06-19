#ifndef CutObjectManager_h
#define CutObjectManager_h
#include <bitset>
#include <set>
#include <algorithm>
#include <iterator>
#include <regex>
#include <iostream>
#include <memory>
#include <utility>
//Cut object
template<typename DataType,typename PredicateType>
struct CutBitBase {
  typedef DataType Data_t;
  typedef PredicateType Predicate_t;
  CutBitBase(Data_t &data,unsigned int id,std::string cutName,std::string cutTitle,Predicate_t predicate):
    mCutName(cutName),mCutTitle(cutTitle),mRefData(data),mPredicate(predicate),mID(id) {}
  //Name or Descriptor
  std::string mCutName;
  //Title
  std::string mCutTitle;
  //Reference for variable;
  std::reference_wrapper<Data_t> mRefData;
  //Predicate which returns if cut passes or not
  Predicate_t mPredicate;
  //Bit position
  unsigned int mID;
  bool operator<(const CutBitBase &cutStruct) const {
    return mID<cutStruct.mID;
  }
};

template<typename DataType>
using CutBit = CutBitBase<DataType,std::function<bool(const DataType &)>>;

//Event cut ID
template<size_t NBITS_MAX>
struct EventCutID:public std::bitset<NBITS_MAX> {
  template<typename... ArgType>
  EventCutID(ArgType... args):std::bitset<NBITS_MAX>(args...){}
  constexpr static size_t sNbitsMax = NBITS_MAX;
  struct Comparer {
    bool operator()(const EventCutID<sNbitsMax>& eventCutID1,const EventCutID<sNbitsMax>& eventCutID2) const {
      return eventCutID1.to_ullong()<eventCutID2.to_ullong();
    }
  };

  template<typename CutBitType>
  void fillCutBit(const CutBitType &cutBit) {
    this->set(cutBit.mID,cutBit.mPredicate(cutBit.mRefData));
  }
  /*******************************************************************************************************************/

  bool isAcceptable(const EventCutID<sNbitsMax> &eventCutID) const{
    return ((*this)&eventCutID)==(*this);
  }
  /*******************************************************************************************************************/

  template<typename EventCutIDtype,typename CutBitType>
  static void setCutBit(EventCutIDtype &eventCutID,const CutBitType &cutBit,std::string delimeter)  {
    if(cutBit.mID>=eventCutID.sNbitsMax) return;
    if(eventCutID[cutBit.mID]==true) return;
    eventCutID.set(cutBit.mID,true);
    if(eventCutID.mEventCutIDname=="") {
      eventCutID.mEventCutIDname=cutBit.mCutName;
    }
    else {
      eventCutID.mEventCutIDname+=delimeter;
      eventCutID.mEventCutIDname+=cutBit.mCutName;
    }
  }

  std::string mEventCutIDname;
  template<typename ValueType>
  using EventCutIDmap = std::map<EventCutID<sNbitsMax>,ValueType,typename EventCutID<sNbitsMax>::Comparer>;
  template<typename ValueType>
  using EventCutIDmultimap = std::multimap<EventCutID<sNbitsMax>,ValueType,typename EventCutID<sNbitsMax>::Comparer,std::allocator<std::pair<const EventCutID<sNbitsMax>,ValueType>> >;
};

//Cut object manager
template<typename DataType,typename CutBitType>
struct CutObjectManagerBase {
  typedef CutBitType CutBit_t;
  typedef DataType Data_t;
  typedef typename CutBit_t::Predicate_t Predicate_t;
  typedef typename DataType::EventCutID_t EventCutID_t;
  CutObjectManagerBase(Data_t &data):mRefData(data),mNbits(0) {}
  ~CutObjectManagerBase()=default;



  void makeCutBit(std::string cutName,std::string cutTitle,Predicate_t predicate) {
    CutBit_t cutObject(mRefData,mNbits,cutName,cutTitle,predicate);
    makeCutBit(cutObject);
    mNbits++;
  }
  /*******************************************************************************************************************/

  void fillEventCutID() {
    mRefData.get().mEventCutID.reset();
    for(const auto &cut: mCuts) {
      mRefData.get().mEventCutID.fillCutBit(cut);
    }
  }
  /*******************************************************************************************************************/

  std::string getEventCutIDname(EventCutID_t eventCutID,std::string delimeter = " && ") {
    std::string eventCutIDname="";
    for(const auto &entry:mCuts)  {
      if(eventCutID[entry.mID]==true) {
        if(eventCutIDname=="") {
          eventCutIDname = entry.mCutName;
        }
        else {
          eventCutIDname+=delimeter;
          eventCutIDname+=entry.mCutName;
        }
      }
    }
    return eventCutIDname;
  }
  /*******************************************************************************************************************/

  EventCutID_t makeEventCutID(EventCutID_t eventCutID,std::string delimeter = " && ") {
    EventCutID_t resultEventCutID = eventCutID;
    resultEventCutID.mEventCutIDname = getEventCutIDname(resultEventCutID,delimeter);
    return resultEventCutID;
  }
  /*******************************************************************************************************************/
EventCutID_t makeNamedEventCutID(std::string cutNames,std::string delimeter = " && ") {
    std::vector<std::string> vecCutNames;
    const std::regex ws_re("\\s+"); // whitespace
    std::copy( std::sregex_token_iterator(cutNames.begin(), cutNames.end(), ws_re, -1)
               ,std::sregex_token_iterator()
               ,std::back_inserter(vecCutNames));
    EventCutID_t resultEventCutID{};
    for(const auto& cutName:vecCutNames) {
      //auto it = mCuts.find(cutName);
      auto it = std::find_if(mCuts.begin()
                             ,mCuts.end()
                             ,[&cutName](const auto&entry)->bool{return (bool)(entry.mCutName==cutName);}
                            );
      if(it!=mCuts.end())  {
        CutBit_t cutBit = *it;
        //resultEventCutID.set(cutBit.mID);
        EventCutID_t::setCutBit(resultEventCutID,cutBit,delimeter);
      }
      else {
        std::cout<<"\nWARNING! CANNOT FIND CUT BIT: "<<cutName<<std::endl;
      }
    }
    return resultEventCutID;
  }
  /*******************************************************************************************************************/
  /*
  EventCutID_t makeEventCutID(std::string eventCutID,std::string delimeter = " && ") {
    EventCutID_t resultEventCutID(eventCutID);
    resultEventCutID.mEventCutIDname = getEventCutIDname(resultEventCutID,delimeter);
    return resultEventCutID;
    //return makeEventCutID(resultEventCutID,delimeter);
  }
  */
  unsigned int mNbits;
  std::reference_wrapper<Data_t> mRefData;
  std::set<CutBit_t> mCuts;

//  EventCutID_t mEventCutID;
 protected:

  void makeCutBit(const CutBit_t &cutObject)  {
    mCuts.insert(cutObject);
  }
};

template<typename DataType>
using CutObjectManager = CutObjectManagerBase<DataType,CutBit<DataType>>;
#endif
