#ifndef GridHandlerManager_h
#define GridHandlerManager_h
#include <iostream>

#include "AnalysisUtils.h"

#include "AliAnalysisAlien.h"
#include "AliAnalysisGrid.h"
#include "TGrid.h"
#include "TGridResult.h"
#include "TSystem.h"

class GridHandlerManager
{
public:

  GridHandlerManager(TString analysisName = "analysis",Bool_t connectToGrid=kFALSE);
  ~GridHandlerManager();
//////Setter functions
	///Standard settings for alien handlers
	void SetVersionAPI(TString version)	{fVersion_API = version;}
	void SetVersionROOT(TString version)	{fVersion_ROOT = version;}
	void SetVersionAliROOT(TString version)	{fVersion_AliROOT = version;}
	void SetVersionAliPhysics(TString version)	{fVersion_AliPhysics = version;}
	void SetRunMode(TString runmode)	{fRunMode = runmode;}
	void SetMarkMacroName(TString mark = "_task.C")	{fMarkMacroGridName = mark;if(!fMarkMacroGridName.EndsWith(".C"))	fMarkMacroGridName+=Form(".C");}
	void SetMarkExecName(TString mark = "_task.sh")	{fMarkExecGridName = mark;if(!fMarkExecGridName.EndsWith(".sh"))	fMarkExecGridName+=Form(".sh");}
	void SetMarkJDLName(TString mark = "_task.jdl")	{fMarkJDLGridName = mark;if(!fMarkJDLGridName.EndsWith(".jdl"))	fMarkExecGridName+=Form(".jdl");}
	void SetSplitMaxInputFileNumber(Int_t NfilesMax=100)	{fNSplitMaxInputFiles = NfilesMax;}

	void SetDefaultVersions()	{
/*		this->SetVersionAPI("V1.1x");
		this->SetVersionROOT("v5-34-30-alice10-11");
		this->SetVersionAliROOT("v5-09-31-1");
		this->SetVersionAliPhysics("vAN-20180610-1");
*/
    this->SetVersionAPI("V1.1x");
    this->SetVersionROOT("v6-20-02-alice7-17");
    this->SetVersionAliROOT("v5-09-56_ROOT6-2");
    this->SetVersionAliPhysics("vAN-20201108_ROOT6-1");
	}
	///////////////
	void SetFilenameInput(TString filenameInput)	{fFilenameInput = filenameInput;}
	void SetPatternDir(TString patternDir = "")	{fPatternDir = patternDir;}
	void SetMarkWorkingDir(TString markWorkingDir="workDir_")	{fMarkWorkingDir = markWorkingDir;}
	void SetMarkFilenameOutput(TString markFilenameOutput = "_result")	{fMarkFilenameOutput = markFilenameOutput;if(!fMarkFilenameOutput.EndsWith(".root"))	fMarkFilenameOutput+=Form(".root");}
	void SetMarkResultDir(TString markResultDir="_result")	{fMarkResultDir = markResultDir;}
  void SetMergerMacroPath(TString mergerMacroPath = "$ALICE_PHYSICS/PWGCF/FLOW/macros/merge.C") {fMergerMacroPath = mergerMacroPath;}
	void SetFilenameOutput(TString filenameOutput="output")	{fFilenameOutput = filenameOutput;if(!fFilenameOutput.EndsWith(".root"))	fFilenameOutput+=Form(".root");}
	void SetWorkingDir(TString workingDir="")	{fWorkingDir = workingDir;}
	void SetResultDir(TString resultDir="")	{fResultDir = resultDir;}
	void SetListOfRuns(TList *listRuns,Bool_t useListNameAsPeriod=kTRUE);
	void SetInputDataPath(TString inputDataPath = "/alice/data/")	{fInputDataPath = inputDataPath;if(!fInputDataPath.EndsWith("/"))fInputDataPath+=Form("/");}
  void SetRunPrefix(std::string runPrefix = "000")	{fRunPrefix = runPrefix;}
	void SetNFilesToMerge(Int_t nFilesToMerge = 150)	{fNFilesToMerge = nFilesToMerge;}
	//////Getter functions
	///Obtaining number of files which located at path, with pattern
	Int_t GetNumberOfFilesOnGrid(TString path, TString pattern);
	TMap* GetMapOfAlienHandlers()	{return fMapAlienHandlers;}
	TString GetResultPathFromHandler(AliAnalysisAlien *alienHandler);
//////Special functions
	Bool_t CheckGridFile(TString path, TString pattern, Int_t minSize=2);
	void ExcludeBadRuns(TList *listPeriod,TString pattern);
	Bool_t AddRunNum(Int_t runnum, TString period);
	AliAnalysisGrid * CreateAlienHandler(TList *listPeriod, TString workDir,TString resultDirName, TString dataPattern,TString gridDataDir);
	void AddAlienHandler(AliAnalysisGrid *alienHandler,TList *listRuns,TString filenameOutput);
	void AddAlienHandler(TList *listPeriod, TString workDir,TString resultDirName, TString dataPattern,TString gridDataDir,TString filenameOutput);

	void AddAnalysisSource(TString analysisSource)	{fListAnalysisSources->Add(new TObjString(analysisSource));}
	void CreateAlienHandlers();
	void InitAlienHandlers();
  //void RunAnalysis(void (*runAnalysis)(AliAnalysisGrid*,TString));
//  void RunAnalysis(std::function<void(AliAnalysisGrid *, TString)> runAnalysis);
  void RunAnalysis();
	void SaveAlienHandlers(TString filenameToSave = "alienHandlers.root");
	void LoadAlienHandlers(TString filenameToLoad = "alienHandlers.root");

	Bool_t DownloadFromGrid(TString inputGridDir,TString inputGridFilename,TString outputFilename, TString outputDirname = "");
	void DownloadResult();


  Int_t GetYearFromPeriodName(TString periodName)	{
    TString year;
    TString stBufYear;
    year = "20";
    for(int iChar=0;iChar<periodName.Length();iChar++)	{
      stBufYear = periodName(iChar,2);
      if(stBufYear.IsDec())	break;
    }
    year += stBufYear;
    return year.Atoi();
  }
  Int_t GetYearFromRunNum(Int_t runnum)	{
    if(208365<=runnum&&runnum<=247170)	return 2015;
    if(247171<=runnum&&runnum<=267254)	return 2016;
    if(267258<=runnum&&runnum<=282900)	return 2017;
    if(282901<=runnum)	return 2018;
    return 0;
  }
  std::function<void(AliAnalysisGrid *, TString)> mAnalysisTask;
protected:

private:
  TString fVersion_API;
  TString fVersion_ROOT;
  TString fVersion_AliROOT;
  TString fVersion_AliPhysics;

  TString fRunMode;

  TString fMarkMacroGridName;
  TString fMarkExecGridName;
  TString fMarkJDLGridName;

  TString fAnalysisSources;
	TList *fListAnalysisSources;
  TString fAnalysisLibs;
  unsigned int fNSplitMaxInputFiles;

  TString fFilenameOutput;
  TString fMarkFilenameOutput;
  TString fWorkingDir;
  TString fMarkWorkingDir;
  TString fResultDir;
  TString fMarkResultDir;
  TString fMergerMacroPath;

  TString fFilenameInput;
  TString fPatternDir;
  TString fInputDataPath;
  std::string fRunPrefix;
  TString fAnalysisName;
  std::vector<unsigned long int> mVecRunNums;
  std::vector<TString> mVecPeriods;
	TList *fListFullRuns;
  unsigned int fNFilesToMerge;
	TList *fListPeriods;
	///Map with alien handlers as key and TMap as value
 /// In value-TMap will be pairs fieldInfo-Info
 /// 1 field: "Run numbers" - TList with runs
 /// 2 filed: "Output filename" - filename
	TMap *fMapAlienHandlers;
	TList *fListAlienHandlers;
  const TString fNameMapAlienHandlers;
 // AliSudaRunInfo *fRunInfoManager;
	/////////////////////////////////////////////////
//  ClassDef(GridHandlerManager,1);
};
#endif
