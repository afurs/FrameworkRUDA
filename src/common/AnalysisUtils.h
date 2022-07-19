#ifndef AnalysisUtils_h
#define AnalysisUtils_h
#include <iostream>
#include <regex>
#include <type_traits>
#include <string>
#include <map>
#include <vector>
#include <boost/filesystem.hpp>

#include "TFile.h"
#include "TKey.h"
#include "TObject.h"
#include "TTree.h"
#include "TList.h"
#include "TCanvas.h"
namespace utilities
{
class AnalysisUtils {
 public:
  AnalysisUtils()=delete;
  static unsigned int getRunNum(std::string path);
  static std::string getFilename(std::string filepath);
  template<typename ObjectType
           ,typename = typename std::enable_if<std::is_base_of<TObject,ObjectType>::value>::type>
  static void getObjectRecursively(TList *listObjects,TList *coll) {
    if(listObjects==nullptr||coll==nullptr) return;
    for(const auto &entry:(*coll))  {
      TObject *obj = (TObject *)entry;
      if(dynamic_cast<TKey *>(obj)!=nullptr)  {
       obj=(TObject *) dynamic_cast<TKey *>(obj)->ReadObj();
      }
      if(dynamic_cast<TDirectoryFile *>(obj)!=nullptr) {
        obj = dynamic_cast<TObject *> (dynamic_cast<TDirectoryFile *>(obj)->GetListOfKeys());
      }
      if(dynamic_cast<ObjectType *>(obj)!=nullptr) {
        listObjects->Add(obj);
      }
      if(dynamic_cast<TList *>(obj)!=nullptr) {
        getObjectRecursively<ObjectType>(listObjects,dynamic_cast<TList *>(obj));
      }
    }
  }
  template<typename ObjectType>
  static TList* getListObjFromFile(const TFile &inputFile)  {
    TList *listResult = new TList();
    getObjectRecursively<ObjectType>(listResult,dynamic_cast<TList *>(inputFile.GetListOfKeys()));
    return listResult;
  }
  static void makePics(TList &listObjects,std::string path
                       ,std::function<void(TObject*)> changeHistState=[](TObject *obj){},Option_t *option = "");
  static std::map<std::string,std::string> makeMapNameTitle(std::string name,std::string title,unsigned int nEntries);
  static std::map<std::string,std::string> makeMapNameTitle(std::string name,std::string title,std::vector<int> vecEntries);
  static std::map<unsigned int,std::vector<std::string>> makeMapRunsToFilepaths(std::string path, std::regex regExpr);
  static std::map<unsigned int,std::vector<std::string>> makeMapRunsToFilepathsROOT(std::string path) {
    return makeMapRunsToFilepaths(path,std::regex{".*\\.root"});
  }
  static std::vector<std::string> makeVecFilepaths(std::string path, std::regex regExpr);
  static std::vector<std::string> makeVecFilepaths(std::string path);
  static std::vector<std::string> makeVecFilepathsROOT(std::string path) {return makeVecFilepaths(path,std::regex{".*\\.root"});}
  static bool writeObjToFile(TObject *objOutput, TFile *fileOutput);
};
}
#endif

