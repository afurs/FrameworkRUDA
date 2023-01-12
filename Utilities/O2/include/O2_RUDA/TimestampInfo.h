#ifndef TimestampInfo_H
#define TimestampInfo_H

#include <cstdio>
#include <string>

struct TimestampInfo {
//  TimestampInfo() = default;
  TimestampInfo(const std::string &filename,const std::string &filenameMask = "o2-ft0-TimeSpectraInfoObject_%llu_%llu_%llu_%llu.root",int nArgs=4):
  mFilenameMask(filenameMask),mNargs(nArgs) { init(filename);}
  ~TimestampInfo() = default;
  std::string mFilenameMask;
  int mNargs;
  uint64_t mTsCreation{0};
  uint64_t mTsStart{0};
  uint64_t mTsEnd{0};
  uint64_t mChunk{0};

  bool mIsReady{false};
  void init(const std::string &filename) {
    const int nArgs = std::sscanf(filename.c_str(),mFilenameMask.c_str(),&mTsCreation,&mTsStart,&mTsEnd,&mChunk);
    if(mNargs==nArgs) {
      mIsReady=true;
    }
    else {
      mIsReady=false;
    }
  }
};
#endif