#include <algorithm>
#include <vector>
#include <map>
#include <functional>
#include <set>
#include <string>
#include <tuple>
#include <memory>

#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TSystem.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/DynamicTable.h"

using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;

template<typename Functor, typename... Args>
void fullRunAnalysis(const std::string &srcPath, const std::string &dstPath,const std::string &prefixDstFilename, const Functor &func,Args&&... args) {
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  TH1::AddDirectory(kFALSE);
  auto putPreffixInFilename = [] (const std::string &filename,const std::string &prefix, const std::string &ext="root")->std::string {
    const std::string filenameNoExt = filename.substr(0,filename.find_last_of(ext)-ext.size());
    std::string result = filenameNoExt+prefix+"."+ext;
    return result;
  };
  const auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(srcPath);
  for(const auto &entry: mapRunToFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepath = entry.second;
    for(const auto &srcFilepath : vecFilepath) {
      const std::string dstFilename{putPreffixInFilename(Utils::getFilename(srcFilepath),prefixDstFilename)};
      const std::string dstFilepath{dstPath+"/"+dstFilename};
      func(srcFilepath,dstFilepath,std::forward<Args>(args)...);
    }
  }
}
