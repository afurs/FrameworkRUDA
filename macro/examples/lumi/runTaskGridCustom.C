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
#include "GridUtils.h"
void runTaskGridCustom() {
  if (!TGrid::Connect("alien://")) return;

  GridManager gridManager;
  gridManager.mUserName="afurs";
  gridManager.mVecPackages= {"JAliEn-ROOT::0.6.5-7",
                             "ROOT::v6-20-08-alice1-63",
                             "APISCONFIG::V1.1x"};
  gridManager.mVecOutputFilenames={"output.root"};
  gridManager.mNInputFilesPerMaster=1000;
  gridManager.mMapAdditionalFields.insert({"Requirements","member(other.GridPartitions, \"cc7\")"});
  std::string pathToFrameworkRUDA = gSystem->Getenv("RUDA_FRAMEWORK_BUILD_PATH");
  std::string gridPathToFrameworkRUDA = gSystem->Getenv("RUDA_FRAMEWORK_BUILD_GRIDPATH");
  std::string gridPathToLogbook = gSystem->Getenv("ALICE_LOGBOOK_GRIDPATH");
  std::string stFilenameJDL = "runTask.jdl";
  std::string execFilename = "exec.sh";
  std::string dirNameWork = "test3";
  std::string dirNameXML = "xml";
  std::string dirNameExec = "exec";
  std::string dirNameResult = "result";
  std::string pathToEntryStruct = "../TrgRatioRawStruct.h";
  std::string pathToExecMacro = "taskGrid.C";
  //auto vecFilepathsFrameworkRUDA = utilities::AnalysisUtils::makeVecFilepaths(pathToFrameworkRUDA);
  std::vector<std::string> vecPathToFiles = {"taskAnalysisFull.C",pathToExecMacro,pathToEntryStruct};
  /*
  std::vector<std::string> vecPathToTrgFiles = {
    "/home/deliner/work/logbook_alice/trees/TrgClasses/TrgClasses2016.root",
    "/home/deliner/work/logbook_alice/trees/TrgClasses/TrgClasses2017.root",
    "/home/deliner/work/logbook_alice/trees/TrgClasses/TrgClasses2018.root"
  };
  */
  auto vecGridPathToRUDA = utilities::grid::GridUtils::makeListFilepath(gridPathToFrameworkRUDA);
  auto vecGridPathToLogBook = utilities::grid::GridUtils::makeListFilepath(gridPathToLogbook);
  std::copy(vecGridPathToRUDA.begin(),vecGridPathToRUDA.end(),std::back_inserter(gridManager.mVecPathGridInputFiles));
  std::copy(vecGridPathToLogBook.begin(),vecGridPathToLogBook.end(),std::back_inserter(gridManager.mVecPathGridInputFiles));
  //Adding FrameworkRUDA
  //std::copy(vecFilepathsFrameworkRUDA.begin(),vecFilepathsFrameworkRUDA.end(),std::back_inserter(vecPathToFiles));
  //for(const auto &entry: vecPathToFiles) std::cout<<std::endl<<entry<<std::endl;
  //Adding Trg files
  //std::copy(vecPathToTrgFiles.begin(),vecPathToTrgFiles.end(),std::back_inserter(vecPathToFiles));
  //for(const auto &entry: vecPathToFiles) std::cout<<std::endl<<entry<<std::endl;
  //
  std::string gridWorkingDir = std::string{gGrid->Pwd()}+dirNameWork;
  std::string gridResultDir=gridWorkingDir+"/"+dirNameResult;
  //std::string gridResultDir=gridWorkingDir+"/result/$2";
  std::string gridDirColl = gridWorkingDir+"/"+dirNameXML;
  std::string gridDirExec =  gridWorkingDir+"/"+dirNameExec;
  //std::string gridDirJDL = gridWorkingDir+"/jdl";

  gGrid->Mkdir(gridWorkingDir.c_str());
  gGrid->Mkdir(gridResultDir.c_str());
  gGrid->Mkdir(gridDirColl.c_str());
  //gGrid->Mkdir(gridDirJDL.c_str());
  gridManager.mPathMacro=pathToExecMacro;
  gridManager.mGridPathOutputDir=gridResultDir+"/$2";
  //gridManager.mGridPathInputCollXML=gridDirColl+"/inputCollRun293582.xml";
  //gridManager.mGridPathInputCollXML=gridWorkingDir+"/inputCollRun293582.xml";
  gridManager.mGridPathInputCollXML=gridDirColl+"/$1";

  gridManager.mGridPathValidationExec=gridWorkingDir+"/validation.sh";
  //gridManager.mGridPathExec=gridWorkingDir+"/"+execFilename;
  gridManager.mGridPathExec=gridDirExec+"/$3";
  //gridManager.mVecOutputFilenames={"log_archive:stderr,stdout,rec.log@disk=1","outputRun293582.root"};

  //Add grid paths for files
  std::map<std::string,std::string> mapLocal2Grid;
  for(const auto &entry: vecPathToFiles) {
    gridManager.mVecPathGridInputFiles.push_back(gridWorkingDir+"/"+(entry.substr(entry.find_last_of('/')+1)));
    mapLocal2Grid.insert({entry,gridManager.mVecPathGridInputFiles.back()});
    //gSystem->Exec(std::string{"alien_cp file://"+entry+" alien://"+gridManager.mVecPathGridInputFiles.back()}.c_str());
    //cout<<endl<<gridManager.mVecPathGridInputFiles.back()<<endl;
  }
  mapLocal2Grid.insert({execFilename,gridWorkingDir+"/"+execFilename});

  mapLocal2Grid.insert({gridManager.mGridPathValidationExec.substr(gridManager.mGridPathValidationExec.find_last_of('/')+1),
                       gridManager.mGridPathValidationExec});
  mapLocal2Grid.insert({"xml/inputCollRun293582.xml",gridDirColl+"/inputCollRun293582.xml"});


  //Set packages
  gridManager.makeExecutables();
  gridManager.makeGridJDL(stFilenameJDL);

  mapLocal2Grid.insert({stFilenameJDL,gridWorkingDir+"/"+stFilenameJDL});
  for(const auto &entry: mapLocal2Grid) {
    std::cout<<"\n====================================================";
    std::cout<<"\nSrc: "<<entry.first;
    std::cout<<"\nDest: "<<entry.second;
    std::cout<<"\n====================================================";
    gSystem->Exec(std::string{"alien_cp file://"+entry.first+" alien://"+entry.second}.c_str());
  }
  std::cout<<std::endl;
  //
  //std::string filepathInputMask = "./TaskRatioT0_pass1_2018_LHC18o/TaskRatioT0_pass1_2018_LHC18o_result/ *293392/*output.root";
  //auto res = gridManager.makeInputXML("test.xml",filepathInputMask,false);
  //res->Print();
  //std::string pathGridResult = "TaskRatioT0_pass1_2018_LHC18o/TaskRatioT0_pass1_2018_LHC18o_result/";
  //GridManager::makeLocalInputXML("xml/wn.xml",pathGridResult+" *output.root");
  /*
  auto mapRun2Path = GridManager::makeMapRun2Path(pathGridResult);
  for(const auto &entry: mapRun2Path) { 
    cout<<endl<<"Run: "<<entry.first<<" | Path: "<<entry.second;
    GridManager::makeLocalInputXML(Form("xml/inputCollRun%i.xml",entry.first),entry.second+" *output.root");
  }
  */
  
}
