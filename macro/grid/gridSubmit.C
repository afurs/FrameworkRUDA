#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
R__LOAD_LIBRARY(libboost_filesystem.so)
R__LOAD_LIBRARY(libAnalysisRUDA.so)
R__LOAD_LIBRARY(libCommonRUDA.so)
R__LOAD_LIBRARY(libTriggerRUDA.so)
R__LOAD_LIBRARY(libGridRUDA.so)
#endif
#include "AnalysisUtils.h"
#include "GridManager.h"

void gridSubmit(std::string pathGridWork="/alice/cern.ch/user/a/afurs/test3"
                ,std::string filenameGridJDL="runTask.jdl"
                ,std::string dirGridXML="xml"
                ,std::string dirGridExec="exec"
                ,std::string pathExec="exec.sh"
                //,std::string pathGridResult = "TaskRatioT0_pass1_2018_LHC18o/TaskRatioT0_pass1_2018_LHC18o_result/"
                ,std::string pathGridResult = ""
                ) {
  struct ArgJDL {
    std::string argXML;
    std::string argResultDir;
    std::string argExec;
    std::string generateCommand(std::string pathGridJDL) {
      return std::string{"submit "+pathGridJDL+" "+argXML+" "+argResultDir+" "+argExec};
    }
  };
  if(pathGridResult!="")  {
    auto mapRun2Path = GridManager::makeMapRun2Path(pathGridResult);
    for(const auto &entry: mapRun2Path) {
      cout<<endl<<"Run: "<<entry.first<<" | Path: "<<entry.second;
      GridManager::makeInputXML(pathGridWork+"/"+dirGridXML+std::string{Form("/inputCollRun%i.xml",entry.first)},entry.second+" *output.root",true);
    }
  }
  auto mapRun2Filepath = GridManager::makeMapRun2Filepath(pathGridWork+"/"+dirGridXML);

  for(const auto &entry: mapRun2Filepath) {
    auto runnum = entry.first;
    auto filepath = entry.second;
    std::string execPath = pathGridWork+"/"+dirGridExec;
    std::string execFileName = Form("/exec_run%i.sh",runnum);
    gSystem->Exec(std::string{"alien_cp file://"+pathExec+" alien://"+execPath+"/"+execFileName}.c_str());
    ArgJDL argJDL{dirGridXML+"/"+filepath.substr(filepath.find_last_of('/')+1)
                  ,std::string{Form("000%i",runnum)}
                  ,std::string{dirGridExec+"/"+execFileName}};
    std::string command  = argJDL.generateCommand(pathGridWork+"/"+filenameGridJDL);
    gGrid->Command(command.c_str());
  }
  //std::string filepathInputMask = "./TaskRatioT0_pass1_2018_LHC18o/TaskRatioT0_pass1_2018_LHC18o_result/ *293392/*output.root";
  //auto res = gridManager.makeInputXML("test.xml",filepathInputMask,false);

  //std::string pathGridResult = "TaskRatioT0_pass1_2018_LHC18o/TaskRatioT0_pass1_2018_LHC18o_result/";
  //GridManager::makeLocalInputXML("xml/wn.xml",pathGridResult+" *output.root");

}
