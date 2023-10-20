#ifndef OutputHistManager_h
#define OutputHistManager_h

#include <memory>
#include <iostream>
#include <string>
#include <map>
#include <set>

#include "AnalysisUtils.h"
#include "HelperHists.h"
#include "OutputManager.h"

#include "TList.h"
#include "TString.h"

namespace utilities {

class OutputHistManager : public OutputManager  {
 public:
  template<typename... Args>
  OutputHistManager(Args&&... args):OutputManager(std::forward<Args>(args)...) {

  }
  using Axis = helpers::hists::Axis;
/*
  void setCurrentListStatus(const std::string &currentMainList,const std::string &currentSubList="") {
    mCurrentList = getList(currentMainList,currentSubList);
    if(mMapMainLists.find(currentMainList)!=mMapMainLists.end()) {
      mCurrentMainList = currentMainList;
    }
    else {
      mCurrentMainList = "";
    }
    if(mMapSubLists.find(currentSubList)!=mMapSubLists.end()) {
      mCurrentSubList = currentSubList;
    }
    else {
      mCurrentSubList = "";
    }
  }
*/
/*
  template<typename HistType, typename... Args>
  auto addHist(const std::string &mainList,const std::string &subList, Args&&... argsHist) {
    TList *currentList = getList(currentMainList,currentSubList);
    if(currentList==nullptr) {
      return nullptr;
    }
    auto hist = makeHist<std::decay_t<HistType>>(std::forward<Args>(argsHist)...);
    currentList->Add(hist);
    return hist;
  }
*/


///////////////////////////////////////////////////////////////////////////
  template<typename HistType, typename... Args>
  std::decay_t<HistType> * addHist(TList *listHists, const std::string &name, const std::string &title, Args&&... argsHist) {
    if(listHists==nullptr) {
      return nullptr;
    }
    auto hist = makeHist<std::decay_t<HistType> > (name,title,std::forward<Args>(argsHist)...);
    listHists->Add(hist);
    return hist;
  }

  template<typename HistType, typename... Args>
  std::vector<std::decay_t<HistType> *>  addHist(TList *listHists, const std::vector< std::pair <std::string, std::string> > &vecNameTitle, Args&&... argsHist) {
    std::vector<std::decay_t<HistType> *> vecResult{};
    if(listHists==nullptr) {
      return vecResult;
    }

    for(const auto &pairNameTitle: vecNameTitle) {
      auto hist = makeHist<std::decay_t<HistType>>(pairNameTitle.first,pairNameTitle.second,std::forward<Args>(argsHist)...);
      vecResult.push_back(hist);
      listHists->Add(hist);
    }
    return vecResult;
  }
///////////////////////////////////////////////////////////////////////////
  template<typename HistType, typename... Args>
  auto addHist(const std::string &mainList,const std::string &subList, const std::string &name, const std::string &title,Args&&... argsHist) {
    return addHist<std::decay_t<HistType> >(getList(mainList,subList),name,title,std::forward<Args>(argsHist)...);
  }

  template<typename HistType, typename... Args>
  auto addHist(const std::string &mainList,const std::string &subList, const std::vector< std::pair <std::string, std::string> > &vecNameTitle, Args&&... argsHist) {
    return addHist<std::decay_t<HistType> >(getList(mainList,subList),vecNameTitle,std::forward<Args>(argsHist)...);
  }

  template<typename HistType, typename... Args>
  auto registerHist(const std::string &name, const std::string &title,Args&&... argsHist) {
    return addHist<std::decay_t<HistType>>(mCurrentList,name,title,std::forward<Args>(argsHist)...);
  }

  template<typename HistType, typename... Args>
  auto registerHist(const std::vector< std::pair <std::string, std::string> > &vecNameTitle,Args&&... argsHist) {
    return addHist<std::decay_t<HistType>>(mCurrentList,vecNameTitle,std::forward<Args>(argsHist)...);
  }



/*
  template<typename HistType, typename... Args>
  std::decay_t<HistType> * addHist(const std::string &mainList,const std::string &subList, const std::string &name, const std::string &title, Args&&... argsHist) {
    TList *currentList = getList(currentMainList,currentSubList);
    if(currentList==nullptr) {
      return nullptr;
    }
    auto hist = makeHist<std::decay_t<HistType>>(name,title,std::forward<Args>(argsHist)...);
    currentList->Add(hist);
    return hist;
  }

  template<typename HistType, typename... Args>
  std::vector<std::decay_t<HistType> *>  addHist(const std::string &mainList,const std::string &subList,const std::vector< std::pair <std::string, std::string> > &vecNameTitle, Args&&... argsHist) {
    TList *currentList = getList(currentMainList,currentSubList);
    std::vector<std::decay_t<HistType> *> vecResult{};
    if(currentList==nullptr) {
      return vecResult;
    }

    for(const auto &pairNameTitle: vecNameTitle) {
      auto hist = makeHist<std::decay_t<HistType>>(pairNameTitle.first,pairNameTitle.second,std::forward<Args>(argsHist)...);
      vecResult.push_back(hist);
      currentList->Add(hist);
    }
    return vecResult;
  }
*/
///////////////////////////////////////////////////////////////////////////
  template<typename HistType, typename... Args>
  static std::decay_t<HistType> * makeHist(const std::string &name, const std::string &title, Args&&... argsHist) {
    std::decay_t<HistType> * hist = nullptr;
    if constexpr((sizeof...(Args))==0) {
      std::cout<<"\n Error! There are no args for hist registration!\n";
      return hist;
    }
    if constexpr(std::conjunction_v<std::is_same<std::decay_t<Args>, Axis>... >) {
      hist = helpers::hists::makeHist<std::decay_t<HistType> >(name.c_str(),title.c_str(),std::forward<Args>(argsHist)...);
    }
    else {
      hist = new std::decay_t<HistType>(name.c_str(),title.c_str(),std::forward<Args>(argsHist)...);
    }
    return hist;
  }

  template<typename HistType, typename... Args>
  static std::vector<std::decay_t<HistType> *> makeHist(const std::vector< std::pair <std::string, std::string> > &vecNameTitle, Args&&... argsHist) {
    std::vector<std::decay_t<HistType> *> vecResult{};
    if constexpr((sizeof...(Args))==0) {
      std::cout<<"\n Error! There are no args for hist registration!\n";
      return vecResult;
    }
    for(const auto &pairNameTitle: vecNameTitle) {
      vecResult.push_back(helpers::hists::makeHist<std::decay_t<HistType> >(pairNameTitle.first,pairNameTitle.second,std::forward<Args>(argsHist)...));
    }
    return vecResult;
  }
  template<typename HistType, typename MapType, typename... Args>
  auto makeVecHists( const MapType &mapLabels,const std::string &namePrefix,const std::string &titlePrefix,Args&&... argsHist)->std::vector<HistType*> {
    std::vector<HistType*> vecResult{};
    for(const auto &entry : mapLabels) {
      const std::string &name = namePrefix + entry.second;
      const std::string &title = titlePrefix + entry.second;
      vecResult.push_back(registerHist<HistType>(name,title,std::forward<Args>(argsHist)...));
    }
    return vecResult;
  };

  template<typename HistType, typename MapType, typename... Args>
  auto makeMapHists(const MapType &mapLabels,const std::string &namePrefix,const std::string &titlePrefix,Args&&... argsHist)->std::map<typename MapType::key_type,HistType*,typename MapType::key_compare> {
    std::map<typename MapType::key_type,HistType*,typename MapType::key_compare> mapResult{};
    for(const auto &entry : mapLabels) {
      const std::string &name = namePrefix + entry.second;
      const std::string &title = titlePrefix + entry.second;
      mapResult.insert({entry.first,registerHist<HistType>(name,title,std::forward<Args>(argsHist)...)});
    }
    return mapResult;
  };


/*
  template<typename HistType, typename... Args>
  static std::vector<std::decay_t<HistType> *> makeHist(const std::vector<int> &vecNums, const std::string &name, const std::string &title,  Args&&... argsHist) {
    std::vector< std::pair <std::string, std::string> > vecNameTitle{};
    for(const auto& num: vecNums) {
      vecNameTitle.emplace_back(std::string{Form(name.c_str(),num)},std::string{Form(title.c_str(),num)});
    }
    return std::move(makeHist<std::decay_t<HistType> >(vecNameTitle,std::forward<Args>(argsHist)...));
  }
*/
};
} //namespace utilities
#endif