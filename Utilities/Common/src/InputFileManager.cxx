#include "CommonRUDA/InputFileManager.h"

using namespace utilities;

/*******************************************************************************************************************/
void InputFileManager::fillMap(unsigned int runnum, const std::string& filepath) {
  auto itRun = mMapRunnum2Index2Filepath.insert({runnum,{}});
  const int chunkIndex = mGetChunkIndex(filepath);
  int detIndex = mGetDetIndex(filepath);
  auto &mapIndex2Filepath = itRun.first->second;
  auto itChunk = mapIndex2Filepath.insert({chunkIndex,{}});
  auto &syncEntry = itChunk.first->second;
  switch(detIndex) {
    case 1:
      syncEntry.mEntryFilepathsFIT[0]=filepath;
      break;
    case 2:
      syncEntry.mEntryFilepathsFIT[1]=filepath;
      break;
    case 3:
      syncEntry.mEntryFilepathsFIT[2]=filepath;
      break;
    case 4:
      syncEntry.mEntryFilepathsFIT[3]=filepath;
      break;

    default:
      syncEntry.mEntryFilepathsFIT[1]=filepath;
      break;
  }
  syncEntry.mRunnum = runnum;
  syncEntry.mChunkIndex = chunkIndex;
}
/*******************************************************************************************************************/
void InputFileManager::addPathSrc(const std::string &pathToSrc) {
  const std::map<unsigned int,std::vector<std::string> > mapFilepaths = AnalysisUtils::makeMapRunsToFilepathsROOT(pathToSrc);
  for(const auto &entry: mapFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
    for(const auto &filepath : vecFilepaths) {
      fillMap(runnum,filepath);
    }
    mSetRunnums.insert(runnum);
  }
}
/*******************************************************************************************************************/
std::vector<InputFileManager::Parameters> InputFileManager::getFileChunks(unsigned int chunksPerRun, const std::string &pathToDst, const std::string &filenamePrefix) {

  std::vector<Parameters> vecParams{};
  std::cout<<"\nPreparing parameters..";
  std::cout<<"\nNumber of runs: "<<mMapRunnum2Index2Filepath.size();
  std::cout<<"\nChunks per runs: "<<chunksPerRun;
  std::cout<<"\n===========================================";
  for(const auto &entryRun: mMapRunnum2Index2Filepath) {
    const auto &runnum = entryRun.first;
    const auto &mapChunk2Entry = entryRun.second;
    int index{};
    int indexOutputChunk{};
    std::cout<<"\nRun "<<runnum<<" with number of files "<<mapChunk2Entry.size();
    for(const auto &entryChunk: mapChunk2Entry) {
      const auto &indexChunk = entryChunk.first;
      const auto &syncFilepaths = entryChunk.second;
      if(!(index%chunksPerRun)) {
        const std::string outputFilepath = pathToDst+"/"+filenamePrefix+"_run"+std::to_string(runnum)+Form("_%.3u",indexOutputChunk)+".root";
        std::cout<<"\n-------------------------------------------";
        std::cout<<"\nPreparing chunk: "<<indexOutputChunk;
        std::cout<<"\nOutput file path: "<<outputFilepath;
        std::cout<<"\nFDD files to process: ";
        std::cout<<"\n"<<syncFilepaths.getFDD();

        std::cout<<"\nFT0 files to process: ";
        std::cout<<"\n"<<syncFilepaths.getFT0();
        std::cout<<"\nFV0 files to process: ";
        std::cout<<"\n"<<syncFilepaths.getFV0();
        std::cout<<"\nCTP files to process: ";
        std::cout<<"\n"<<syncFilepaths.getCTP();

        vecParams.push_back({runnum,{{syncFilepaths.getFDD(),syncFilepaths.getFT0(),syncFilepaths.getFV0(),syncFilepaths.getCTP()}},outputFilepath});
        indexOutputChunk++;
      }
      else {
        auto &backEntry = vecParams.back();
        std::cout<<"\n"<<syncFilepaths.getFT0();
        backEntry.mFilepathInputFIT.push_back({syncFilepaths.getFDD(), syncFilepaths.getFT0(), syncFilepaths.getFV0(),syncFilepaths.getCTP()});
      }
      index++;
    }
    std::cout<<"\n-------------------------------------------";
  }
  std::cout<<"\n===========================================\n";
  return vecParams;
}
