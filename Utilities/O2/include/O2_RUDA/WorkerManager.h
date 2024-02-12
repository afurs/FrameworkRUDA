#include "ROOT/TProcessExecutor.hxx"

#include "CommonRUDA/AnalysisUtils.h"
#include "CommonRUDA/InputFileManager.h"

#include "O2_RUDA/CCDB.h"
#include "O2_RUDA/DetectorFIT.h"
#include "O2_RUDA/EventChDataParams.h"

#include <vector>
#include <utility>
#include <cstddef>
#include <type_traits>
#include <iostream>

namespace worker_manager {

using EntryCCDB = utilities::ccdb::EntryCCDB;

template<typename Detector, typename Functor, typename ExtraParams = std::nullptr_t>
void runWorkers(const Functor &functor,
                const std::vector<std::string> &vecPathToSrc,
                const std::string &pathToDst,
                std::size_t nParallelJobs,
                std::size_t nChunksPerRun,
                const std::set<unsigned int> &setRunnum,
                const ExtraParams &extraParams
                ) {
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  /////////////////////////////////////////////////////
//  const SlewingParameters slewing_params("slewing_coefs_pp.root");

  /////////////////////////////////////////////////////
  const std::string filenamePrefix = "hist";
  /////////////////////////////////////////////////////
  utilities::InputFileManager inputFileManager;
  for(const auto & pathToSrc:vecPathToSrc) {
    inputFileManager.addPathSrc(pathToSrc);
  }
  const auto vecParams = inputFileManager.getFileChunks(nChunksPerRun,pathToDst,filenamePrefix);

  const auto &setRunnumsToProcces = setRunnum.size()>0 ? setRunnum : inputFileManager.mSetRunnums;
  const auto mapRun2EntryCCDB = utilities::ccdb::EntryCCDB::getMapRun2EntryCCDB(setRunnumsToProcces,true,true,true);
  ROOT::TProcessExecutor pool(nParallelJobs);
  const auto result = pool.Map([&mapRun2EntryCCDB, &setRunnumsToProcces, &functor, &extraParams](const utilities::InputFileManager::Parameters &entry) {
            if(setRunnumsToProcces.size()>0) {
              if(setRunnumsToProcces.find(entry.mRunnum)==setRunnumsToProcces.end()) {
                return 0;
              }
            }
            std::vector<std::string> vecFilepathInput{};
            for(const auto &entryFilepath: entry.mFilepathInputFIT) {
              if(Detector::sDetFIT_ID==detectorFIT::EDetectorFIT::kFT0) {
                vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFT0);
              }
              else if(Detector::sDetFIT_ID==detectorFIT::EDetectorFIT::kFV0) {
                vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFV0);
              }
              else if(Detector::sDetFIT_ID==detectorFIT::EDetectorFIT::kFDD) {
                vecFilepathInput.emplace_back(entryFilepath.mFilepathInputFDD);
              }
            }
            const auto itEntryCCDB = mapRun2EntryCCDB.find(entry.mRunnum);
            if(itEntryCCDB != mapRun2EntryCCDB.end()) {
              if constexpr(!std::is_same_v<std::decay_t<ExtraParams>, std::nullptr_t>) {
                functor(entry.mRunnum,
                                  vecFilepathInput,
                                  entry.mFilepathOutput,
                                  itEntryCCDB->second,
                                  extraParams);
              }
              else {
                functor(entry.mRunnum,
                                  vecFilepathInput,
                                  entry.mFilepathOutput,
                                  itEntryCCDB->second);
              }
            }
            else {
              std::cout<<"ERROR! CANNOT FIND GRPLHCIFData for run "<<entry.mRunnum<<std::endl;
            }
            return 0;
          }
          , vecParams);
}
}