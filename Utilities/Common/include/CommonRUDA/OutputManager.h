#ifndef OutputManager_h
#define OutputManager_h

#include <memory>
#include <iostream>
#include <string>
#include <map>
#include <set>

#include "AnalysisUtils.h"

#include "TList.h"
namespace utilities {

class OutputManager {
 public:
  OutputManager(const std::string &filepathPrefix="",const std::string &filepathSuffix=".root"):mFilepathPrefix(filepathPrefix),mFilepathSuffix(filepathSuffix) {
  }
  // Register final list which will be written to file as container, listKeyName is used as key for map and as suffix for filename
  void registerFinalList (const std::string &listKeyName="output", const std::string &listName = "output") {
    auto itPair = mMapMainLists.insert({listKeyName,std::make_unique<TList>()});
    if(itPair.second) {
      itPair.first->second->SetName(listName.c_str());
      itPair.first->second->SetOwner(true);
    }
  }
  template<typename... Args>
  void registerAllFinalLists (Args... args) {
    (static_cast<void>(registerFinalList(std::string{args})),...);
  }

  TList *addSubList (const std::string &subListName, const std::string &masterListName) {
    const auto &itPair = mMapSubList2Main.insert({subListName,masterListName});
    if(itPair.second) {
      TList *subList = new TList();
      subList->SetName(subListName.c_str());
      subList->SetOwner(true);
      auto itMaster = mMapMainLists.find(masterListName);
      if(itMaster!=mMapMainLists.end()) {
        mMapSubLists.insert({subListName,subList});
        itMaster->second->Add(subList);
        return subList;
      }
      else {
        std::cout << "\n Error! Master list " << masterListName << " is not found!" << std::endl;
        return nullptr;
      }
    }
    else {
      std::cout << "\n Error! Sub list " << subListName << " already exists in master list " << masterListName << std::endl;
      return nullptr;
    }
  }

  bool registerSubList (const std::string &subListName, const std::string &masterListName) {
    TList *addedList = addSubList (subListName, masterListName);
    if(addedList==nullptr) {
      return false;
    }
    else {
      return true;
    }
  }

  void registerAllSubLists(const std::map<std::string, std::set<std::string> > &mapListHierarchy) {
    for(const auto &mainList: mapListHierarchy) {
      const auto &mainListName = mainList.first;
      const auto &subListSet = mainList.second;
      registerFinalList(mainListName);
      for(const auto &subListName: subListSet) {
        const bool isAdded = addSubList(subListName,mainListName);
      }
    }
  }

  void storeOutput() {
    for(auto &entry: mMapMainLists) {
      const std::string path = mFilepathPrefix + entry.first + mFilepathSuffix;
      const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(entry.second.get(),path);
    }
  }
  static std::string getFilepathPrefix(const std::string &fullFilepath,const std::string &extFileoutput = ".root") {
    std::string filepathPrefix{};
    const auto posExt=fullFilepath.find_last_of(extFileoutput)-extFileoutput.size() + 1;
    if(posExt != std::string::npos) {
      filepathPrefix = fullFilepath.substr(0,posExt);
    }
    else {
      std::cout<<"Warning! Check output filepath! There are no extension: "<<extFileoutput<<std::endl;
    }
    return filepathPrefix;
  }
  void setFilepathPrefix(const std::string &fullFilepath,const std::string &extFileoutput = ".root") {
    mFilepathPrefix = getFilepathPrefix(fullFilepath, extFileoutput);
  }
  TList* getList(const std::string &mainList, const std::string &subList="") {
    auto itMain = mMapMainLists.find(mainList);
    auto itSub = mMapSubLists.find(subList);
    if(subList.size()==0 && itMain != mMapMainLists.end()) {
      return itMain->second.get();
    }
    else if(itMain != mMapMainLists.end() && itSub != mMapSubLists.end()) {
      return itSub->second;
    }
    std::cout<<"\nCannot find lists "<<mainList<<" | "<<subList<<std::endl;
    return nullptr;
  }
  std::string mFilepathPrefix{};
  std::string mFilepathSuffix{".root"};
  std::map<std::string, std::string> mMapSubList2Main; // sub list name to master list name mapping
  std::map<std::string, TList *> mMapSubLists; // sub list mapping
  std::map<std::string, std::unique_ptr<TList> > mMapMainLists; // Lists which will be written to files, name -> list mapping

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

  void registerCurrentList(const std::string &currentMainList,const std::string &currentSubList="") {
    registerFinalList(currentMainList);
    if(currentSubList.size()>0) {
      const auto result = registerSubList(currentSubList,currentMainList);
    }
    setCurrentListStatus(currentMainList,currentSubList);
  }

 protected:
  TList *mCurrentList{nullptr};
  std::string mCurrentMainList{};
  std::string mCurrentSubList{};
};
}// namespace utilities

#endif