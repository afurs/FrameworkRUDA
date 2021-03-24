#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libGridRUDA.so)

#endif
#include <string>
#include <vector>
#include "GridUtils.h"
//#include "merge.C"
//Example:
//pathGridResultDir = TaskAgeingT0_pass1_2018_LHC18p/TaskAgeingT0_pass1_2018_LHC18p_result
//should contain dirs like "000%i" with runnums
//filenameGridMask = "output.root"
//if filenameLocalMask equals to "" then filenameLocalMask = "TaskAgeingT0_pass1_2018_LHC18p_result_run%i.root"
void downloadResult(std::string pathGridResultDir
                    ,std::string filenameGrid="output.root"
                    ,std::string filenameLocalMask=""
                    ,std::size_t nMaxFilesToMerge=500) {
  auto mapRun2Path = utilities::grid::GridUtils::makeMapRun2Path(pathGridResultDir);
  std::string localPwd = gSystem->pwd();
  std::string commandBegin;
  std::string commandEnd;
  std::string pathToFrameworkRUDA = gSystem->Getenv("RUDA_FRAMEWORK_BUILD_PATH");
  commandBegin = "aliroot -b -q .x ";
  commandEnd ="";
  //commandBegin = "nice nohup aliroot -b -q .x ";
  //commandEnd =" >> log.log &";
  std::string pathToMergerMacro = pathToFrameworkRUDA+"/macro/grid/merge.C";
  std::string outputMask = "_run%i.root";
  std::string dirNameRunMask = "000%i";
  std::string dirDownload = "Download";
  gSystem->mkdir(dirDownload.c_str());
  gSystem->cd(dirDownload.c_str());

  bool isFirst=true;

  for(const auto &entry:mapRun2Path ) {
    cout<<"\n=============================================================";
    unsigned int runnum = entry.first;
    cout<<"\nRUN: "<<runnum<<"\nPATH: "<<entry.second;
    std::string filenameResult;
    std::string dirName=pathGridResultDir.substr(pathGridResultDir.find_last_of('/')+1);
    //std::string dirNameRun=entry.second.substr(entry.second.find_last_of('/')+1);
    std::string dirNameRun=Form(dirNameRunMask.c_str(),runnum);
    if(filenameLocalMask=="") {
      filenameResult=Form(std::string{dirName+ outputMask}.c_str(),runnum);
    }
    else {
      filenameResult=Form(filenameLocalMask.c_str(),runnum);
    }
    auto gridResult = utilities::grid::GridUtils::makeCollectionXML(entry.second,filenameGrid);
    std::size_t nFilesOnGrid{};
    if(gridResult==nullptr) {
      cout<<"\nWARNING! NO FILES!\n";
      continue;
    }
    else {
      nFilesOnGrid=gridResult->GetSize();
    }
    cout<<"\nNumber of files \""<<filenameGrid<<"\" : "<<nFilesOnGrid;
    cout<<"\nLocal output: "<<filenameResult;
    bool doMerge=false;
    if(nFilesOnGrid<=nMaxFilesToMerge) doMerge = true;
    std::string localPwdTmp = gSystem->pwd();
    if(doMerge) {
      cout<<"\nOutput will be merged in file \""<<filenameResult<<"\"";
    }
    else {
      cout<<"\nOutput will not be merged! Output dir: "<<dirNameRun;
      gSystem->mkdir(dirNameRun.c_str());
      gSystem->cd(dirNameRun.c_str());
    }

    std::string command = std::string{commandBegin};
    command+=std::string{pathToMergerMacro};
    command+=std::string{"\\(\\\""};
    command+=std::string{entry.second};
    command+=std::string{"\\\"\\,\\\""};
    command+=std::string{filenameGrid};
    //BE CAREFULL
    //command+="\\\"\,";
    //command+="\\\"\\,";
    if(doMerge)	{
      //command+="kFALSE\\,\\\"";
      command+=std::string{"\\\"\\,kFALSE\\,\\\""};//\\\"\\)"};
    }
    else	{
      //command+="kTRUE\\,\\\"";
      command+=std::string{"\\\"\\,kTRUE\\,\\\""};//+{"\\\"\\)"};
    }
    if(doMerge)	command+=std::string{filenameResult};
    command+=std::string{"\\\"\\)"};
    command+=std::string{commandEnd};
    std::cout<<"\n\nCOMMAND: "<<command;
    gSystem->Exec(command.c_str());
    //gSystem->Exec(std::string{commandBegin+pathToMergerMacro}.c_str());

    cout<<"\n=============================================================\n";
    //if(!isFirst) continue;
    /*
    if(doMerge) {
      merge(entry.second.c_str(),filenameGrid.c_str(),kFALSE,filenameResult.c_str());
    }
    else {
      merge(entry.second.c_str(),filenameGrid.c_str(),kTRUE,filenameResult.c_str());
    }
    */
    gSystem->cd(localPwdTmp.c_str());
    isFirst=false;
  }
  gSystem->cd(localPwd.c_str());
  cout<<endl;
}
