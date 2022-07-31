#ifndef AnalysisBase_h
#define AnalysisBase_h
#include <iostream>
#include <vector>
#include <string>
//#include "AnalysisUtils.cxx"
//#include "../helpers/HelperCommon.h"
//#include "../utilities/AnalysisUtils.h"
//#include "CommonRUDA/HelperCommon.h"
//#include "CommonRUDA/AnalysisUtils.h"

#include "TFile.h"
#include "TTree.h"
#include "TList.h"

//#ifndef MACRO_RUN
#include "CommonRUDA/HelperCommon.h"
#include "CommonRUDA/AnalysisUtils.h"
//#elseif
//#include "../common/HelperCommon.cxx"
//#include "../common/AnalysisUtils.cxx"
//#endif
//AnalysisClass should contain:
//void init();
//void initFile();
//void initTree(TTree *treeInput);
//void processEvent();
//void updatePerTree();
//void updateFinish();

//Uses CRTP(static polyporphism)
/*
struct AnalysisHelper {
  template <typename T>
  class initState_t;
  template <typename T>
  class has_initState;

};*/

struct AnalysisHelper{
  template <typename T>
  using init_t = decltype(std::declval<T>().init());
  template <typename T>
  using has_init = helpers::common::Detect<T, init_t, void>;
  //Checks if analysis class has initFile() method
  template <typename T>
  using initFile_t = decltype(std::declval<T>().initFile());
  template <typename T>
  using has_initFile = helpers::common::Detect<T, initFile_t, void>;
  //Checks if analysis class has checkTree(const TTree &)
  template <typename T>
  using checkTree_t = decltype(std::declval<T>().checkTree(std::declval<const TTree &>()));
  template <typename T>
  using has_checkTree = helpers::common::Detect<T,checkTree_t,bool >;
  //Checks if analysis class has updateFinish() method
  template <typename T>
  using updateFinish_t = decltype(std::declval<T>().updateFinish());
  template <typename T>
  using has_updateFinish = helpers::common::Detect<T, updateFinish_t, void>;
  //Checks if analysis class has updatePerTree() method
  template <typename T>
  using updatePerTree_t = decltype(std::declval<T>().updatePerTree());
  template <typename T>
  using has_updatePerTree = helpers::common::Detect<T, updatePerTree_t, void>;
  //Checks if there are initState method
  template <typename T>
  using initState_t = decltype(std::declval<T>().initState());
  template <typename T>
  using has_initState = helpers::common::Detect<T, initState_t, void>;

};

template<typename AnalysisClass>
class AnalysisBase
{
public:
  using AnalysisUtils = utilities::AnalysisUtils;
  typedef AnalysisClass Analysis_t;
  AnalysisBase()=default;
  ~AnalysisBase()=default;
  void setInputData(std::vector<std::string> &vecInputFilepaths) {mVecInputFilepaths = vecInputFilepaths;}
  void setOutputData(std::string filepathResult="result.root")	{mFilepathResult=filepathResult;}

  void run()  {
    mListResult.SetName("output");
    //mListResult.SetOwner(kTRUE);
    if constexpr(AnalysisHelper::has_init<AnalysisClass>::value)
        static_cast<AnalysisClass*>(this)->init();
    mNumAll = mVecInputFilepaths.size();
    if(mNumAll==0) return;
    mNumSuccess = 0;
    unsigned int nFilesProccesed=0;
    for(const auto &filepathEntry: mVecInputFilepaths) {
      std::cout<<"\n################################################################";
      std::cout<<"\n#Proccesing file "<<nFilesProccesed+1<<"/"<<mNumAll;
      bool result = processFile(filepathEntry);
      //bool result = true;
      if(result)  mNumSuccess++;
      nFilesProccesed++;
    }
    std::cout<<"\n#Success: "<<100.*mNumSuccess/mNumAll<<"%\n";
    if constexpr(AnalysisHelper::has_updateFinish<AnalysisClass>::value)
        static_cast<AnalysisClass*>(this)->updateFinish();

    if(mFilepathResult=="") mFilepathResult="result.root";
  //  if(nSuccess>0) saveResult();
  }
  /*******************************************************************************************************************/

  bool processFile(std::string inputFilepath)  {
    if constexpr(AnalysisHelper::has_initFile<AnalysisClass>::value)
        static_cast<AnalysisClass*>(this)->initFile();

    std::cout<<"\n#File path: "<<inputFilepath;
    std::cout<<"\n################################################################\n";
    TFile *fileInput = TFile::Open(inputFilepath.c_str());
    bool result = true;
    if(fileInput->IsZombie()||!fileInput->IsOpen()) {
      std::cout<<"\nWARNING! CORRUPTED FILE(IsZombie and not IsOpen): "<<inputFilepath.c_str()<<std::endl;
      return false;
    }
    TList *listInput = AnalysisUtils::getListObjFromFile<TTree>(*fileInput);
    listInput->SetOwner(kTRUE);
    for(const auto&entry:(*listInput)) {
      TTree *treeInput = dynamic_cast<TTree *>(entry);
      if(treeInput==nullptr) {
        std::cout<<"\nWarning! Something wrong with tree: "<<entry->GetName()<<std::endl;
        continue;
      }
      if constexpr(AnalysisHelper::has_checkTree<AnalysisClass>::value) {
        if(!(static_cast<AnalysisClass*>(this)->checkTree(treeInput))) {
          std::cout<<"\nWarning! Tree \""<<entry->GetName()<<"\" doesn't pass through checker!"<<std::endl;
          continue;
        }
      }
      result &= processTree(treeInput);
      //delete treeInput;
    }
    fileInput->Close();
    //treeInput->Print();
    delete listInput;
    return result;
  }
  /*******************************************************************************************************************/

  bool processTree(TTree *treeInput)  {
    if(treeInput==nullptr)  return false;
    if(treeInput->GetEntries()==0)  return false;
    static_cast<AnalysisClass*>(this)->initTree(treeInput);
    for(int iEvent=0;iEvent<treeInput->GetEntries();iEvent++) {
      treeInput->GetEvent(iEvent);
      if(iEvent%10000 == 0)		{
        std::cout<<" Event: "<<iEvent<<"   of "<<treeInput->GetEntries()<<" events \n";
      }
      static_cast<AnalysisClass*>(this)->processEvent();
    }

    if constexpr(AnalysisHelper::has_updatePerTree<AnalysisClass>::value)
        static_cast<AnalysisClass*>(this)->updatePerTree();
    return true;
  }
  /*******************************************************************************************************************/

  void saveResult() {
    if(mNumSuccess==0) {
      std::cout<<"\nNothing to save!\n";
      return;
    }
    TFile *tf = new TFile(mFilepathResult.c_str(),"RECREATE");
    mListResult.Sort();
    AnalysisUtils::writeObjToFile(&mListResult,tf);
    tf->Close();
    delete tf;
    //AnalysisUtils::writeListResultToFile(&mListResult,mFilepathResult);
  }


  unsigned int mNumSuccess;
  unsigned int mNumAll;
  std::vector<std::string> mVecInputFilepaths;
  std::string mFilepathResult;
  unsigned int mEventStepMonitoring;
  TList mListResult;
};

#endif
