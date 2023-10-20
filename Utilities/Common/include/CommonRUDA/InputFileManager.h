#ifndef IntputFileManager_h
#define IntputFileManager_h

#include <regex>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <cstdio>
#include <iostream>

#include "CommonRUDA/AnalysisUtils.h"
namespace utilities {


class InputFileManager {
 public:
  // Simple entry for FIT detector filepaths
  struct EntryFilepathFIT {
    std::string mFilepathInputFDD{};
    std::string mFilepathInputFT0{};
    std::string mFilepathInputFV0{};
    std::string mFilepathInputCTP{};

    void print() const {
      std::cout<<std::endl;
      std::cout<<"\n FDD input: "<<mFilepathInputFDD;
      std::cout<<"\n FT0 input: "<<mFilepathInputFT0;
      std::cout<<"\n FV0 input: "<<mFilepathInputFV0;
      std::cout<<"\n CTP input: "<<mFilepathInputCTP;

      std::cout<<std::endl;
    }
  };

  struct SyncEntryFilenamesFIT {
    unsigned int mRunnum{};
    int mChunkIndex{-1};// starts from 0, if -1 then no chunk index
    std::array<std::string,4> mEntryFilepathsFIT{}; // 0 - FDD, 1 - FT0, 2 - FV0, 3 - CTP
    std::string getFDD() const { return mEntryFilepathsFIT[0];}
    std::string getFT0() const { return mEntryFilepathsFIT[1];}
    std::string getFV0() const { return mEntryFilepathsFIT[2];}
    std::string getCTP() const { return mEntryFilepathsFIT[3];}
  };

  struct Parameters {
    unsigned int mRunnum{};
    std::vector<EntryFilepathFIT> mFilepathInputFIT{};
    std::string mFilepathOutput{};
  };

  InputFileManager()=default;
  ~InputFileManager()=default;

  void fillMap(unsigned int runnum, const std::string& filepath);
  void addPathSrc(const std::string &pathToSrc);
  std::vector<Parameters> getFileChunks(unsigned int chunksPerRun, const std::string &pathToDst, const std::string &filenamePrefix);

  std::map<unsigned int, std::map<int, SyncEntryFilenamesFIT> > mMapRunnum2Index2Filepath{};
  std::set<unsigned int> mSetRunnums{};

  // Get detector index from filepath, in format "_d[1-3]_" if there are no such format, index will belong to FT0 by default
  std::function<int(const std::string&)> mGetDetIndex = [] (const std::string& filepath) {
    auto regRunNum = std::regex{"_d[1-4]{1}_"};
    std::smatch sm;
    bool searchResult = std::regex_search(filepath,sm,regRunNum);
    if(searchResult) {
      const std::string st = sm.str();
      const std::string stForm{"_d%u_"};
      int res{};
      std::sscanf(st.c_str(),stForm.c_str(),&res);
      return res;
    }
    else {
      return -1;
    }
  };
  // Get chunk index from filepath, in format "_chunkIndex.root", returns -1 in case of no index.
  std::function<int(const std::string&)> mGetChunkIndex = [] (const std::string& filepath) {
    auto regRunNum = std::regex{"_[0-9]{1,}.root"};
    std::smatch sm;
    bool searchResult = std::regex_search(filepath,sm,regRunNum);
    if(searchResult) {
      const std::string st = sm.str();
      const std::string stForm{"_%u.root"};
      int res{};
      std::sscanf(st.c_str(),stForm.c_str(),&res);
      return res;
    }
    else {
      return -1;
    }
  };
};
}// namespace utilities

#endif