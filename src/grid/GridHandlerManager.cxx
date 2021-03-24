#include "GridHandlerManager.h"

GridHandlerManager::GridHandlerManager(TString analysisName, Bool_t connectToGrid)	{
	std::cout<<"\n////////////////////////////////////////////////////////////////";
  std::cout<<"\n/Initializating object GridHandlerManager...";
	std::cout<<"\n////////////////////////////////////////////////////////////////";
	///Setting default configs for alien handlers
	this->SetDefaultVersions();
	this->SetMarkMacroName();
	this->SetMarkExecName();
	this->SetMarkJDLName();
	this->SetFilenameOutput();
	this->SetMarkFilenameOutput();
	this->SetWorkingDir();
	this->SetMarkWorkingDir();
	this->SetResultDir();
	this->SetMarkResultDir();
	this->SetInputDataPath();
	this->SetPatternDir();
	this->SetRunPrefix();
	this->SetNFilesToMerge();
	this->SetRunMode("test");
  this->SetMergerMacroPath();
	///

	fAnalysisName = analysisName;

	fListAnalysisSources = new TList();
	fListAnalysisSources->SetName("List of analysis source names");
	fListAnalysisSources->SetOwner(kTRUE);

	fListPeriods = new TList();
	fListPeriods->SetName("List of periods");
	fListPeriods->SetOwner(kTRUE);

	fMapAlienHandlers = new TMap();
	fMapAlienHandlers->SetName("mapAlienHandlers");

	fListAlienHandlers = new TList();
	fListAlienHandlers->SetName("listAlienHandlers");

	
	if(connectToGrid&&!gGrid) TGrid::Connect("alien://");

	//if(!gGrid) TGrid::Connect("alien://");
	std::cout<<"\n/Initialization complete!";
	std::cout<<"\n////////////////////////////////////////////////////////////////\n";

}
/*******************************************************************************************************************/
void GridHandlerManager::SetListOfRuns(TList *listRuns,Bool_t useListNameAsPeriod)	{
  if(!listRuns)	return;
	TObjString *objstRun;
  TString stRun;
  for(auto entry:(*listRuns)) {
    objstRun = dynamic_cast<TObjString *>(entry);
    stRun = objstRun->GetString();
		if(useListNameAsPeriod)	{
			this->AddRunNum(stRun.Atoi(),listRuns->GetName());
		}
    else	{
     // this->AddRunNum(stRun.Atoi(),fRunInfoManager->GetPeriodFromRunNum(stRun.Atoi()));
    }
	}
}
/*******************************************************************************************************************/
Int_t GridHandlerManager::GetNumberOfFilesOnGrid(TString path, TString pattern)	{
  std::cout<<"\nCHECK PATH: "<<path<<" | PATTERN: "<<pattern<<std::endl;
	if(!gGrid) TGrid::Connect("alien://");
	TGridResult *gridResult;
	TString command = "find ";
	command+=path;
  if(!path.EndsWith("/")) command+=Form("/");
	//if(!path.EndsWith("/") || !path.EndsWith("/ ")) command+=Form("/ ");
	//if(!path.EndsWith(" ")) command+=Form(" ");
	command+=pattern;
	std::cout<<"\nCommand: "<<command<<std::endl;
	gridResult = gGrid->Command(command);
	Int_t nFiles = gridResult->GetSize();
	delete gridResult;
	return nFiles;
}
/*******************************************************************************************************************/
TString GridHandlerManager::GetResultPathFromHandler(AliAnalysisAlien *alienHandler){
	TString stResult = "";
	if(!alienHandler)	return stResult;
	if(gGrid)	stResult = gGrid->Pwd();
	stResult += alienHandler->GetGridWorkingDir();
	return stResult;
}
/*******************************************************************************************************************/
Bool_t GridHandlerManager::CheckGridFile(TString path, TString pattern, Int_t minSize)	{
	Int_t nFiles = this->GetNumberOfFilesOnGrid(path,pattern);
	if(nFiles<minSize)	{
    std::cout<<"\nNO DATA AT PATH \""<<path<<"\" AND PATTERN \""<<pattern<<"\" ! Min num: "<<minSize<<"\n";
		return kFALSE;
	}
	return kTRUE;
}
/*******************************************************************************************************************/
void GridHandlerManager::ExcludeBadRuns(TList *listPeriod,TString pattern)	{
	TString periodName = listPeriod->GetName();
  TString year = Form("%i",GetYearFromPeriodName(periodName));

	TString path = fInputDataPath;
	path+=year;
	path+=Form("/");
	path+=periodName;
  TString pathFull;
	TObjString *objstRun;
	TString stRun;
  for(auto entry:(*listPeriod)) {
    objstRun = dynamic_cast<TObjString *>(entry);
    stRun = objstRun->GetString();
		pathFull=path;
		pathFull+=Form("/");
    pathFull+=fRunPrefix.c_str();
		pathFull+=stRun;
		pathFull+=Form("/");
		if(!(this->CheckGridFile(pathFull,pattern,2)))	listPeriod->Remove(objstRun);
	}
}
/*******************************************************************************************************************/
Bool_t GridHandlerManager::AddRunNum(Int_t runnum, TString period)	{
	TList *listPeriod = NULL;
	listPeriod = (TList *)fListPeriods->FindObject(period);
	if(!listPeriod)	{
		listPeriod = new TList();
		listPeriod->SetName(period);
		fListPeriods->Add(listPeriod);
	}
	if(listPeriod->FindObject(Form("%i",runnum)))	return kFALSE;
	listPeriod->Add(new TObjString(Form("%i",runnum)));
	return kTRUE;
}
/*******************************************************************************************************************/
AliAnalysisGrid *GridHandlerManager::CreateAlienHandler(TList *listPeriod, TString workDir, TString resultDirName, TString dataPattern, TString gridDataDir)	{
	/// Check if user has a valid token, otherwise make one. This has limitations.
	/// One can always follow the standard procedure of calling alien-token-init then
	///   source /tmp/gclient_env_$UID in the current shell.
	//   if (!AliAnalysisGrid::CreateToken()) return NULL;
	AliAnalysisAlien *plugin = new AliAnalysisAlien();
	TString pluginName;
	pluginName = fAnalysisName;
	if(listPeriod)	{
		pluginName += Form("_");
		pluginName += listPeriod->GetName();
	}
	plugin->SetName(pluginName);
	plugin->SetOverwriteMode();
	/// Set the run mode (can be "full", "test", "offline", "submit" or "terminate")
	plugin->SetRunMode(fRunMode);
	plugin->SetCheckCopy(kFALSE);
	plugin->SetNtestFiles(1);
	/// Set versions of used packages
	plugin->SetAPIVersion(fVersion_API);
	plugin->SetROOTVersion(fVersion_ROOT);
	plugin->SetAliROOTVersion(fVersion_AliROOT);
	plugin->SetAliPhysicsVersion(fVersion_AliPhysics);

	plugin->SetGridDataDir(gridDataDir);
	plugin->SetDataPattern(dataPattern);
  if(fRunPrefix!="")	plugin->SetRunPrefix(fRunPrefix.c_str());

	TObjString *objstRunNum;
	TString stRunNum;
	Int_t runnum;
  if(listPeriod)	{
    for(auto entry: (*listPeriod)) {
      objstRunNum = dynamic_cast<TObjString *>(entry);
			stRunNum = objstRunNum->GetString();
			runnum=stRunNum.Atoi();
			plugin->AddRunNumber(runnum);
		}
	}
	///Setting output data directories on grid
	//plugin->AddDataFile(xmlFileName);
	//TString alienDir = "/alice/cern.ch/user/a/afurs/";
	//alienDir+=gridWorkingDir;
	//gGrid->Mkdir(alienDir);
	//gROOT->LoadMacro("alien_cp_single.C+");
	//alien_cp_single(xmlFileName,alienDir);
	//gROOT->UnloadMacro("alien_cp_single.C");
	TString resultDir = resultDirName;
	plugin->SetGridWorkingDir(workDir);
	plugin->SetGridOutputDir(resultDir);
	TString stAnalysisSource = "";
	TString stAdditionalLibs = "";
  TString stBuf;
  TObjString *objstBuf;
  for(auto entry:(*fListAnalysisSources)) {
    objstBuf = dynamic_cast<TObjString *>(entry);
    stBuf = objstBuf->GetString();
		if(stBuf.EndsWith(".cxx"))	{
			stAnalysisSource += stBuf;
			stAnalysisSource += " ";
			stAdditionalLibs += stBuf;
			stAdditionalLibs += " ";
		}
		if(stBuf.EndsWith(".h"))	{
			stAdditionalLibs += stBuf;
			stAdditionalLibs += " ";
		}
		if(!stBuf.EndsWith(".cxx")&&!stBuf.EndsWith(".h"))	{
			stAnalysisSource += stBuf;
			stAnalysisSource += ".cxx ";
			stAdditionalLibs += stBuf;
			stAdditionalLibs += Form(".cxx ");
			stAdditionalLibs += stBuf;
			stAdditionalLibs += Form(".h ");
    }
	}
	plugin->SetAnalysisSource(stAnalysisSource);
	plugin->SetAdditionalLibs(stAdditionalLibs);
  //plugin->SetDefaultOutputs(kTRUE); //excluded in new version of AliROOT
  plugin->SetDefaultOutputs(kFALSE); //excluded in new version of AliROOT
	plugin->SetOutputToRunNo();
	/// Optionally define the files to be archived.
	//plugin->SetOutputArchive("log_archive.zip:stdout,stderr@ALICE::NIHAM::File root_archive.zip:*.root@ALICE::NIHAM::File");
  plugin->SetOutputArchive("log_archive:stderr,stdout,rec.log@disk=1");
  plugin->SetOutputFiles(TString{fFilenameOutput+"@disk=2"});
	//  plugin->SetOutputArchive("log_archive.zip:stdout,stderr");

	/// Optionally set a name for the generated analysis macro (default MyAnalysis.C)
	TString macroName = workDir;
	macroName+=fMarkMacroGridName;
	plugin->SetAnalysisMacro(macroName);
	/// Optionally set maximum number of input files/subjob (default 100, put 0 to ignore)
	plugin->SetSplitMaxInputFileNumber(fNSplitMaxInputFiles);
	/// Optionally modify the executable name (default analysis.sh)
	TString execName = workDir;
	execName+=fMarkExecGridName;
	plugin->SetExecutable(execName);
	/// Optionally set number of failed jobs that will trigger killing waiting sub-jobs.
	//   plugin->SetMaxInitFailed(5);
	/// Optionally resubmit threshold.
	//plugin->SetMasterResubmitThreshold(100);
	/// Optionally set time to live (default 30000 sec)
	plugin->SetTTL(30000);
	/// Optionally set input format (default xml-single)
	plugin->SetInputFormat("xml-single");
	///Optionally modify the name of the generated JDL (default analysis.jdl)
	TString jdlName = workDir;
	jdlName+=fMarkJDLGridName;
	plugin->SetJDLName(jdlName);
	/// Optionally modify job price (default 1)
	plugin->SetPrice(1);
	/// Optionally modify split mode (default 'se')
	plugin->SetSplitMode("se");
//	plugin->Print();
	return plugin;
}
/*******************************************************************************************************************/
void GridHandlerManager::AddAlienHandler(AliAnalysisGrid *alienHandler,TList *listRuns,TString filenameOutput)	{
	if(!alienHandler||filenameOutput.EqualTo(""))	return;
	TMap *mapInfo = new TMap();
	mapInfo->SetName("Alien handler info");
	mapInfo->SetOwner(kTRUE);
	if(listRuns)	mapInfo->Add(new TObjString("Run numbers"),(TList *)listRuns->Clone(listRuns->GetName()));

	mapInfo->Add(new TObjString("Output filename"),new TObjString(filenameOutput));
	fMapAlienHandlers->Add(alienHandler,mapInfo);
}
/*******************************************************************************************************************/
void GridHandlerManager::AddAlienHandler(TList *listPeriod, TString workDir, TString resultDirName, TString dataPattern, TString gridDataDir, TString filenameOutput)	{
	AliAnalysisGrid *alienHandler = this->CreateAlienHandler(listPeriod, workDir, resultDirName, dataPattern, gridDataDir);
	this->AddAlienHandler(alienHandler,listPeriod,filenameOutput);
}
/*******************************************************************************************************************/
void GridHandlerManager::InitAlienHandlers()	{
	TString pattern = "";

	if(!fPatternDir.EqualTo(""))	{
		//if(!fPatternDir.BeginsWith("*/")) pattern = "*/";
		if(!fPatternDir.BeginsWith("/")) pattern = "/";
		
		pattern += fPatternDir;
		if(!fPatternDir.EndsWith("/")||!fPatternDir.EndsWith("*")) pattern += "/";
    //OLD
    /*
    if(!fPatternDir.EndsWith("*"))pattern += "* ";
		if(fPatternDir.EndsWith("*"))pattern += " ";
    */
    if(!fPatternDir.EndsWith("*"))pattern += "*";
    //if(fPatternDir.EndsWith("*"))pattern += " ";
	}
  //OLD
  //if(!fFilenameInput.BeginsWith("*"))	pattern += " *";
  if(!fFilenameInput.BeginsWith("*"))	pattern += "*";
	pattern += fFilenameInput;
	if(!fFilenameInput.EndsWith(".root"))	pattern += ".root";


	AliAnalysisGrid *alienHandler;
	TString year;
	TString periodName;
	TString gridDataDir;
	TString workDir;
	TString resultDirName;
	TString outputFilename;
	if(!fListPeriods->GetSize())	{
		workDir = fWorkingDir;
		if(fWorkingDir.EqualTo(""))	{
			workDir = fMarkWorkingDir;
		}
		///RESULT DIR NAME
		resultDirName = fResultDir;
		if(fResultDir.EqualTo(""))	{
			resultDirName = workDir;
			resultDirName += fMarkResultDir;
		}
		///OUTPUT FILE NAME
		outputFilename = fFilenameOutput;
		///INPUT DATA PATH
		gridDataDir = fInputDataPath;
		///
		if(!this->CheckGridFile(gridDataDir,pattern)) return;
		alienHandler = this->CreateAlienHandler(NULL,workDir,resultDirName,pattern,gridDataDir);
		this->AddAlienHandler(alienHandler,NULL,outputFilename);
		return;
	}
  fListPeriods->Sort();
  for(auto entry:(*fListPeriods)) {
    TList *listPeriod = dynamic_cast<TList *>(entry);
		periodName = listPeriod->GetName();

    year = Form("%i",GetYearFromPeriodName(periodName));
		///WORK DIR NAME
		workDir = fWorkingDir;
		if(fWorkingDir.EqualTo(""))	{
			workDir = fMarkWorkingDir;
      //workDir += periodName;
		}
		///RESULT DIR NAME
		resultDirName = fResultDir;
		if(fResultDir.EqualTo(""))	{
			resultDirName = workDir;
			resultDirName += fMarkResultDir;
		}
		///OUTPUT FILE NAME
		outputFilename = fFilenameOutput;
		if(fFilenameOutput.EqualTo(""))	{
			outputFilename = periodName;
			outputFilename += fMarkFilenameOutput;
		}
		///INPUT DATA PATH
		gridDataDir = fInputDataPath;
		gridDataDir+=year;
		gridDataDir+=Form("/");
		gridDataDir+=periodName;
		///
		//this->CheckGridFile(gridDataDir,pattern);
		this->ExcludeBadRuns(listPeriod,pattern);
    if(!listPeriod->GetSize()) continue;

		alienHandler = this->CreateAlienHandler(listPeriod,workDir,resultDirName,pattern,gridDataDir);
		this->AddAlienHandler(alienHandler,listPeriod,outputFilename);
		///runAnalysis(alienHandler,outputFilename);
	}
}
/*******************************************************************************************************************/
//void GridHandlerManager::RunAnalysis(void (*runAnalysis)(AliAnalysisGrid *, TString))	{
void GridHandlerManager::RunAnalysis()	{
	if(!fMapAlienHandlers->GetSize())	{
		std::cout<<"\n#Warning! Map of alien handlers is empty, saving is abort!\n";
		return;
	}

  TList *listHandlerNames = new TList();
  for(auto entry:(*fMapAlienHandlers)) {
    auto pairEntry = dynamic_cast<TPair *>(entry);
    if(pairEntry==nullptr) continue;
    listHandlerNames->Add(new TObjString(((TNamed *)(pairEntry->Key()))->GetName()));
  }

  listHandlerNames->Sort();
	AliAnalysisGrid *alienHandler;
	TMap *mapAnalysisData;
	TObjString *objstFilenameOutput,*objstBuf;
	TString stFilenameOutput,stBuf;
	TPair *pair;
  for(auto entry:(*listHandlerNames)) {
    objstBuf = dynamic_cast<TObjString *>(entry);
		stBuf = objstBuf->GetString();
		pair = (TPair *)fMapAlienHandlers->FindObject(stBuf);
		alienHandler = (AliAnalysisGrid *)pair->Key();
		mapAnalysisData = (TMap *)	fMapAlienHandlers->GetValue(stBuf);
		objstFilenameOutput = (TObjString *) mapAnalysisData->GetValue("Output filename");
		stFilenameOutput = objstFilenameOutput->GetString();
    mAnalysisTask(alienHandler,stFilenameOutput);
//		runAnalysis(alienHandler,stFilenameOutput);
		std::cout<<"\n";
	}
}

/*******************************************************************************************************************/
void GridHandlerManager::SaveAlienHandlers(TString filenameToSave)	{
	if(!fMapAlienHandlers->GetSize())	{
		std::cout<<"\n#Warning! Map of alien handlers is empty, saving is abort!\n";
		return;
  }
  TFile fileToSave(filenameToSave,"READ");
  utilities::AnalysisUtils::writeObjToFile(fMapAlienHandlers,&fileToSave);
  fileToSave.Close();
}
/*******************************************************************************************************************/
void GridHandlerManager::LoadAlienHandlers(TString filenameToLoad)	{
	TString filename = filenameToLoad;
	if(!filename.EndsWith(".root"))	filename += Form(".root");
	TFile *fileHandler = new TFile(filename,"READ");
	if(!fileHandler)	{
		std::cout<<"\n#Warning! File "<<filename<<" doesn't exist! Alien handlers loading is aborted!\n";
		return;
	}
	TMap *mapBuf = (TMap *)fileHandler->Get("mapAlienHandlers");
	if(!mapBuf)	{
		std::cout<<"\n#Warning! Map doesn't exist in file "<<filenameToLoad<<"! Alien handlers loading is aborted!\n";
		return;
	}
	fMapAlienHandlers = (TMap *) mapBuf->Clone();
	fileHandler->Close();
	delete fileHandler;
}
/*******************************************************************************************************************/
Bool_t GridHandlerManager::DownloadFromGrid(TString inputGridDir, TString inputGridFilename, TString outputFilename, TString outputDirname)	{
  if(!gGrid) TGrid::Connect("alien://");
	TString outputDirName = outputDirname;
	TString currentWorkDir = gSystem->pwd();

	TString inputFileName = inputGridFilename;
	if(!inputFileName.EndsWith(".root"))	inputFileName+=Form(".root");
	TString pattern = " *";
	pattern+=inputFileName;
	Int_t nFilesGrid = this->GetNumberOfFilesOnGrid(inputGridDir,pattern);
	if(!nFilesGrid)	{
		std::cout<<"\n#Warning! There is no files with pattern \""<<pattern<<"\" at path: "<<inputGridDir<<"";
		return kFALSE;
	}
	TString outputFileName = outputFilename;
	if(!outputFileName.EndsWith(".root"))	outputFileName+=Form(".root");
	Bool_t doMerge=kTRUE;
	if(nFilesGrid>fNFilesToMerge)	doMerge = kFALSE;

	if(!outputDirName.EqualTo("")&&!doMerge)	{
		if(!gSystem->cd(outputDirName))	{
			gSystem->mkdir(outputDirName);
			gSystem->cd(outputDirName);
		}
	}
  TString command = "aliroot -b -q .x ";
  command+=fMergerMacroPath;
  command+="\\(\\\"";
	command+=inputGridDir;
	command+="\\\"\\,\\\"";
	command+=inputFileName;
  //BE CAREFULL
  //command+="\\\"\,";
  command+="\\\"\\,";
	if(!outputDirName.EqualTo("")&&!doMerge)	{
		command+="kTRUE\\,\\\"";
	}
	else	{
		command+="kFALSE\\,\\\"";
	}
	if(doMerge)	command+=outputFileName;
	command+="\\\"\\)";
	std::cout<<"\nCOMMAND: "<<command<<std::endl;
	gSystem->Exec(command);
	gSystem->cd(currentWorkDir);
	return kTRUE;
}
/*******************************************************************************************************************/
void GridHandlerManager::DownloadResult()	{
  if(!gGrid) TGrid::Connect("alien://");
  TList *listHandlerNames = new TList();
  for(auto entry:(*fMapAlienHandlers)) {
    auto pairEntry = dynamic_cast<TPair *>(entry);
    if(pairEntry==nullptr) continue;
    listHandlerNames->Add(new TObjString(((TNamed *)(pairEntry->Key()))->GetName()));
  }
  listHandlerNames->Sort();
	AliAnalysisAlien *alienHandler;
	TMap *mapAnalysisData;
	TList *listRuns;
  TObjString *objstFilenameOutput,*objstBuf,*objstRunNum;
	TString stFilenameOutput,stBuf,stRunNum;
	TPair *pair;
  Int_t runNum;
	TString inputGridPath;
	TString inputGridResultPath;
	TString outputDirName,outputFileName;
	TString alienPath = gGrid->Pwd();
  TString period;
  for(auto entry:(*listHandlerNames)) {
    objstBuf = dynamic_cast<TObjString *>(entry);
		stBuf = objstBuf->GetString();
		pair = (TPair *)fMapAlienHandlers->FindObject(stBuf);
		alienHandler = (AliAnalysisAlien *)pair->Key();

		inputGridPath = alienPath;
		inputGridPath += alienHandler->GetGridWorkingDir();
		inputGridPath += Form("/");
		inputGridPath += alienHandler->GetGridOutputDir();
		inputGridPath += Form("/");

		mapAnalysisData = (TMap *)	fMapAlienHandlers->GetValue(stBuf);
		objstFilenameOutput = (TObjString *) mapAnalysisData->GetValue("Output filename");
		stFilenameOutput = objstFilenameOutput->GetString();
		listRuns = (TList *)	mapAnalysisData->GetValue("Run numbers");
    period = listRuns->GetName();
    for(auto entryRunNum:(*listRuns)) {
      objstRunNum = dynamic_cast<TObjString *>(entryRunNum);
			stRunNum = objstRunNum->GetString();
			runNum = stRunNum.Atoi();
			inputGridResultPath = inputGridPath;
			inputGridResultPath += Form("000%i/",runNum);
			
			outputFileName = fMarkWorkingDir;
			outputFileName += period;
			outputFileName += Form("_run%i.root",runNum);
			
			outputDirName = fMarkWorkingDir;
			outputDirName += period;
			outputDirName += Form("_run%i",runNum);
			this->DownloadFromGrid(inputGridResultPath,stFilenameOutput,outputFileName,outputDirName);
    }
	}
}

/*******************************************************************************************************************/
GridHandlerManager::~GridHandlerManager()	{
	std::cout<<"\n////////////////////////////////////////////////////////////////";
  std::cout<<"\n/Deleting object GridHandlerManager...";
	std::cout<<"\n////////////////////////////////////////////////////////////////";
	std::cout<<"\n/Deleting completed!";
	std::cout<<"\n////////////////////////////////////////////////////////////////\n";
}

/*******************************************************************************************************************/
