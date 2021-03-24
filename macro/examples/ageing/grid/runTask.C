#ifdef __CLING__
R__ADD_INCLUDE_PATH($RUDA_FRAMEWORK_BUILD_PATH/include)
R__ADD_LIBRARY_PATH($RUDA_FRAMEWORK_BUILD_PATH/lib)
//R__LOAD_LIBRARY(libboost_filesystem.so)
//R__LOAD_LIBRARY(libAnalysisRUDA.so)
//R__LOAD_LIBRARY(libCommonRUDA.so)
R__LOAD_LIBRARY(libRunManagerRUDA.so)
R__LOAD_LIBRARY(libGridRUDA.so)
#endif
//#include
#include "runGridTask.h"
#include "GridHandlerManager.h"
#include "RunManager.h"
void runTask()	{
	TString sourceNames = "AliTaskAgeingT0";
	Bool_t useLoadHanlers = 0;
	//Bool_t useLoadHanlers = kTRUE;
  Bool_t downloadResult = false;
  Bool_t useListsFromRunManager=false;
  GridHandlerManager *fGridManager;

	////////////////
	TList *listPeriods = new TList();
	listPeriods->SetOwner(kTRUE);
	TString stPeriod;


  //std::vector<Int_t> vecRunAllowed = {};
  std::set<int> setRunNums = {294241,294242,294721,294722};
  auto runFilter = [](const utilities::run_manager::RunConditionTable &entry)->bool {
    if(entry.mStatusT00==1&&(entry.mStatusV00==1||entry.mStatusV00==11)&&entry.mIntBunches>0 /*&& entry.mFill<4947&&entry.mRunNum==254422*/) {
      return true;
    }
    return false;
  };


  std::string filepathTreesRCT = "/home/deliner/work/logbook_alice/trees/RCT/data/";
//  std::vector<std::string> vecPeriods = {/*"LHC18g","LHC18h","LHC18j","LHC18i","LHC18k"*/"LHC18m"};
  //std::vector<std::string> vecPeriods = {"LHC17j"};
  std::vector<std::string> vecPeriods = {"LHC18p"};
  
  std::string year = "2018";
  //std::string dataType = "muon_calo1";
  std::string dataType = "pass1";
  std::string inputFilename = "AliESDs.root";
  std::string currentWorkDir = gSystem->pwd();
  std::string dirDownload = "download";
  std::string filepathToStruct = "EventStruct.h";
  std::string taskName = "TaskAgeingT0_pass1_tmp_";//will be used as workdir also
  taskName+=year;
  //taskName+="_";
  std::string runMode = "full";
  //std::string runMode = "test";
  for(const auto& entry:vecPeriods) {
    TList *listRuns = new TList();
    listRuns->SetName(entry.c_str());
    std::string filepath{filepathTreesRCT+year+"/"+entry+"/"+dataType+"/RCT.root"};
    auto vectorRuns = utilities::run_manager::RunConditionTable::getVecRCT(filepath);
    for(const auto& entryRun:vectorRuns) {
      if(runFilter(entryRun)&&setRunNums.find(entryRun.mRunNum)!=setRunNums.end()) {
        listRuns->Add(new TObjString(Form("%i",entryRun.mRunNum)));
        cout<<"\n RUN: "<<entryRun.mRunNum;
      }
    }
    listPeriods->Add(listRuns);
  }
  //dataType = "pass1_withTRDtracking";
  //dataType = "muon_calo1";
  std::string dirTaskFiles = taskName;
  cout<<endl;
  if(listPeriods->GetSize()==0) {
    cout<<"\nWarning! Empty run list!\n";
  }
  for(const auto &entry:(*listPeriods))  {
    gSystem->cd(currentWorkDir.c_str());
    TList *listBufRuns = dynamic_cast<TList *>(entry);
    if(!listBufRuns->GetSize())	continue;
    /*
    if(!gSystem->cd(dirTaskFiles.c_str()))	{
      gSystem->mkdir(dirTaskFiles.c_str());
      gSystem->cd(dirTaskFiles.c_str());
    }
    */
    listBufRuns->Print();
    std::string stResultDirMark="_";
    std::string stPeriodName = listBufRuns->GetName();
    stResultDirMark+=stPeriodName;

    std::string stTask = taskName+"_"+stPeriodName;
    std::string stTaskName = stTask;
    //stTaskName+="_task.sh";

    fGridManager = new GridHandlerManager(stTask.c_str(),0);
    fGridManager->SetListOfRuns(listBufRuns);
    fGridManager->AddAnalysisSource(sourceNames);
    fGridManager->AddAnalysisSource(filepathToStruct);
    fGridManager->SetMarkWorkingDir(stTask.c_str());
    //fGridManager->SetMarkResultDir(stResultDirMark);
    fGridManager->SetPatternDir(dataType.c_str());
    fGridManager->SetFilenameInput(inputFilename.c_str());
    //fGridManager->SetMarkExecName(stTaskName);
    fGridManager->SetSplitMaxInputFileNumber(80);
    fGridManager->SetNFilesToMerge(400);
    fGridManager->SetRunMode(runMode.c_str());
    //fGridManager->SetRunMode("test");
    TString handlerHame = "handlerTaskAmpT0_";
    handlerHame += stPeriod;

    if(!useLoadHanlers) {
      fGridManager->InitAlienHandlers();
      fGridManager->SaveAlienHandlers(handlerHame);
    }
    else {
      fGridManager->LoadAlienHandlers(handlerHame);
    }

    if(!downloadResult) {
      fGridManager->mAnalysisTask = runGridTask;
      fGridManager->RunAnalysis();
    }
    else {
      if(!gSystem->cd(dirDownload.c_str()))	{
        gSystem->mkdir(dirDownload.c_str());
        gSystem->cd(dirDownload.c_str());
      }
      fGridManager->DownloadResult();
    }

    delete fGridManager;
  }
  gSystem->cd(currentWorkDir.c_str());
}
