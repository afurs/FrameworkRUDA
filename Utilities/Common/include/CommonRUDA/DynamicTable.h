#ifndef Trends_h
#define Trends_h

#include <string>
#include <functional>
#include <iostream>
#include <fstream>

#include "TList.h"

#include "CommonRUDA/HelperCommon.h"
#include "CommonRUDA/FunctionTraits.h"

namespace common
{
  template<typename... Functors>
  struct DynamicTable
  {
    typedef std::tuple<Functors ...> TupleFunctors_t;
    typedef FunctionTraits<TupleFunctors_t> TupleFunctorsWrapper_t;
    constexpr static std::size_t sNfields = sizeof...(Functors);

    DynamicTable(Functors... functors): mTupleFunctors(std::make_tuple(functors ...)),mTupleFunctorsWrapper(std::make_tuple(functors ...)),mCurrentTupleArgs({}) {}
    DynamicTable(const std::string &filepath,Functors... functors):mFilepathTable(filepath),  mTupleFunctors(std::make_tuple(functors ...)),mTupleFunctorsWrapper(std::make_tuple(functors ...)),mCurrentTupleArgs({}) {}

    void fillTable(const typename TupleFunctorsWrapper_t::TupleArgsFunc_t &argsForEntry,bool fillTextTable=true) {
      mTable.push_back(mTupleFunctorsWrapper.eval(argsForEntry));
      if(fillTextTable) {
        mTextTable.push_back(entryAsString(mTable.back(),mDelimeter));
      }
    }
    std::string getTextEntry() const {return mTextTable.back();}
    void fillTable(bool fillTextTable=true) {
      fillTable(mCurrentTupleArgs,fillTextTable);
    }
    static std::string entryAsString(const typename TupleFunctorsWrapper_t::TupleResultsFunc_t &tupleResultsFunc, const std::string &delimeter) {
      const auto vecArgsAsStrs = TupleFunctorsWrapper_t::resultsAsVecOfStrs(tupleResultsFunc,delimeter);
      if(vecArgsAsStrs.size()==0) {
        return std::string{""};
      }
      else if(vecArgsAsStrs.size()==1) {
        return vecArgsAsStrs[0];
      }
      else {
        std::string entryAsStr{vecArgsAsStrs[0]};
        for(int i=1;i<vecArgsAsStrs.size();i++) {
          entryAsStr+=delimeter;
          entryAsStr+=vecArgsAsStrs[i];
        }
        return entryAsStr;
      }
    }
    void print() const {
      std::cout<<std::endl;
      for(const auto &entry: mTextTable) {
        std::cout<<entry;
        std::cout<<std::endl;
      }
    }
    std::string headerToEntry() {
      std::string header{};
      if(mArrFieldNames.size()>0) {
        header+=mArrFieldNames[0];
      }
      for(int iElem=1;iElem<mArrFieldNames.size();iElem++) {
        header+=mDelimeter;
        header+=mArrFieldNames[iElem];
      }
      return header;
    }
    void toCSV (const std::string &filename = "output.csv", bool useHeader = false) {
      std::ofstream outputFile(filename);
      if(outputFile.is_open()) {
        if(useHeader) {
          outputFile << headerToEntry() << std::endl;
        }
        for(const auto &entry: mTextTable) {
          outputFile << entry << std::endl;
        }
      }
      else {
        std::cout<<"\nWarning! Cannot create file " << filename << std::endl;
      }
      outputFile.close();
    }
    void toCSV (bool useHeader = false) {
      toCSV(mFilepathTable,useHeader);
    }

    template<size_t N, typename ...Args>
    void setCurrentArg(const Args&... args) {
      std::get<N>(mCurrentTupleArgs) = std::make_tuple(args...);
    }
    void setFilepath(const std::string &filepath) {mFilepathTable=filepath;}
    TupleFunctorsWrapper_t mTupleFunctorsWrapper;
    TupleFunctors_t mTupleFunctors;
    typename TupleFunctorsWrapper_t::TupleArgsFunc_t mCurrentTupleArgs;
    std::vector<typename TupleFunctorsWrapper_t::TupleResultsFunc_t> mTable;
    std::vector<std::string> mTextTable;
    //std::array<std::string,sNfields> mArrFieldNames;
    std::vector<std::string> mArrFieldNames;
    std::string mDelimeter{";"};
    std::string mFilepathTable{"table.csv"};
  };


}
#endif