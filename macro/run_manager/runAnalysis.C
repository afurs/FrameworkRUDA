#include "AliAnalysisTaskSpectrumT0.h"
void runAnalysis(){
  // create an instance of the plugin
    AliAnalysisAlien *alienHandler = new AliAnalysisAlien();
    // if our class relies on code that has to be compiled, then we need to tell the Grid nodes
    // where the headers can be found
    alienHandler->AddIncludePath("-I. -I$ROOTSYS/include -I$ALICE_ROOT -I$ALICE_ROOT/include -I$ALICE_PHYSICS/include");
    // here we tell which source files need to be copied to the Grid nodes
    alienHandler->SetAdditionalLibs("AliAnalysisTaskSpectrumT0.cxx AliAnalysisTaskSpectrumT0.h EventStruct.h runListRCT2018_pp_tzero.root");
    alienHandler->SetAnalysisSource("AliAnalysisTaskSpectrumT0.cxx");
    // here we specify which version of aliphysics will be used on the Grid nodes
    // only specificy the AliPhysics version, other dependencies (AliRoot, ROOT, etc)
    // will be resolved automatically
    alienHandler->SetAliPhysicsVersion("vAN-20210422_ROOT6-1");
    // here we specify where the input data of our analysis is stored
    alienHandler->SetGridDataDir("/alice/data/2018/LHC18p/");
    // and here we specify a specific pattern of filenames
    alienHandler->SetDataPattern("/pass1/**AliESDs.root");
    // here we set the run prefix: use '000' for data, or nothing for MC
    alienHandler->SetRunPrefix("000");
    alienHandler->SetNtestFiles(1);
    alienHandler->SetRunMode("test");
    // specify the runnumber to be analyzed. you can call this function multiple times with 
    // different runnumbers
    alienHandler->AddRunNumber(294718);
    alienHandler->AddRunNumber(294155);
    alienHandler->AddRunNumber(294305);
    alienHandler->AddRunNumber(294524);
    alienHandler->AddRunNumber(294562);
    //alienHandler->SetRunRange(294590,294593);
    alienHandler->SetNrunsPerMaster(5);
    // number of files per subjob, this specifies how many input files
    // each Grid node will analyze. For slow analyses, you might want to 
    // lower this number
    alienHandler->SetSplitMaxInputFileNumber(40);
    // the TTL is the 'time to live', which specifies how long a job can run, in seconds
    // the longer you make this time, the lower the jobs priority will be. However, if the job doesn't
    // finish within its TTL, it will be killed, so choose a reasonable number
    alienHandler->SetTTL(10000);
    // do you want to create a subfolder in which the output of each runnumber is stored? 
    //alienHandler->SetOutputToRunNo(kTRUE);
    // do you want to keep your jobs log files ? 
    //alienHandler->SetKeepLogs(kTRUE);
    // we can specify that we want to, later on, use Grid to also merge
    // our output. to enable this, we will set 'SetMergeViaJDL' to kTRUE
    //alienHandler->SetMergeViaJDL(kTRUE);
    //alienHandler->SetMaxMergeStages(1);
    // define the output folders, and give them a sensible name
    // jobs *cannot* overwrite output, so make sure that
    // folders with names as these do not yet exist in your AliEn space
    alienHandler->SetGridWorkingDir("testWork");
    alienHandler->SetGridOutputDir("result");
    // lastly, we give names to the automatically generated analysis scripts
    alienHandler->SetJDLName("myTask.jdl");
    alienHandler->SetExecutable("myTask.sh");
TString outputFilename = "output.root";
Int_t debugLevel = 2;
	printf(" @@@@@@@@@@@@\n");
	/// Load common libraries
	gSystem->Load("libCore.so");
	gSystem->Load("libTree.so");
	gSystem->Load("libGeom.so");
	gSystem->Load("libVMC.so");
	gSystem->Load("libPhysics.so");
	gSystem->Load("libSTEERBase");
	gSystem->Load("libESD");
	gSystem->Load("libAOD");
	gSystem->Load("libANALYSIS");
	gSystem->Load("libANALYSISalice");
	gSystem->Load("libOADB.so");
	/// Use AliRoot includes to compile our task
	// gROOT->ProcessLine(".include $ALICE_ROOT/include");
  //OLD CASE
  /*
	gSystem->AddIncludePath( "-I$ALICE_ROOT/include" );
	gSystem->AddIncludePath( "-I$ALICE_PHYSICS/include" );
  */
  gInterpreter->ProcessLine(".include $ROOTSYS/include");
  gInterpreter->ProcessLine(".include $ALICE_ROOT/include");
	//gROOT->ProcessLine(".include $ROOTSYS/include");
	//gROOT->ProcessLine(".include $ALICE_ROOT/include");
	//gROOT->ProcessLine(".include $ALICE_PHYSICS/include");
	//gROOT->ProcessLine(".include /opt/alice/aliroot/v5-05-69-AN/build/include");
	//gROOT->ProcessLine(".include /opt/alice/aliroot/master/build/include");
	/// Create and configure the alien handler plugin
	//gROOT->LoadMacro("CreateAlienHandlerT0.C");
	cout<<"Done..."<<endl;
	cout<<"/////////////////////////////////////////////"<<endl;
	/// Create the analysis manager
	AliAnalysisManager *mgr = new AliAnalysisManager("Analysis");
	alienHandler->AddIncludePath( "-I$ALICE_PHYSICS/OADB/COMMON/MULTIPLICITY");
	alienHandler->AddIncludePath( "-I$ALICE_PHYSICS/OADB/macros");
  //alienHandler->SetMergeViaJDL();
	/// Connect plug-in to the analysis manager
	mgr->SetGridHandler(alienHandler);
	AliESDInputHandler* esdH = new AliESDInputHandler();
	mgr->SetInputEventHandler(esdH);
	//gROOT->ProcessLine(".L $ALICE_PHYSICS/PWGPP/CalibMacros/CPass1/AddTaskT0Analysis.C");
	///Centrality, description at https://twiki.cern.ch/twiki/bin/viewauth/ALICE/CentStudies
	//gROOT->LoadMacro("$ALICE_PHYSICS/OADB/macros/AddTaskCentrality.C");
	//AliCentralitySelectionTask *taskCentrality = AddTaskCentrality();
	//taskCentrality->SetPass(2);
	/// if you use the following line, your task only gets the selected events

	//gROOT->LoadMacro("$ALICE_PHYSICS/OADB/COMMON/MULTIPLICITY/macros/AddTaskMultSelection.C");
	//AliMultSelectionTask *taskCentrality = AddTaskMultSelection(kFALSE);
	//mgr->AddTask(taskCentrality);

	///AddTaskPhysicsSelection (const Bool_t mCAnalysisFlag = kFALSE, const Bool_t applyPileupCuts = kFALSE, const UInt_t deprecatedFlag2 = 0, const Bool_t useSpecialOutput=kFALSE)

	
  //gROOT->LoadMacro("$ALICE_PHYSICS/OADB/macros/AddTaskPhysicsSelection.C");
  TString pathAliPhysicsSelectionTask = "$ALICE_PHYSICS/OADB/macros/AddTaskPhysicsSelection.C";
  //AliPhysicsSelectionTask* taskPhysSel = AddTaskPhysicsSelection(kFALSE,kTRUE);
  AliPhysicsSelectionTask* taskPhysSel = reinterpret_cast<AliPhysicsSelectionTask*>(gInterpreter->ExecuteMacro(pathAliPhysicsSelectionTask));
	mgr->AddTask(taskPhysSel);
  gInterpreter->LoadMacro("AliAnalysisTaskSpectrumT0.cxx+g");
	TString taskName= outputFilename;
	taskName+=Form("_task");

 // AliAnalysisTaskMyTask *task = reinterpret_cast<AliAnalysisTaskMyTask*>(gInterpreter->ExecuteMacro("AddMyTask.C"));

  AliAnalysisTaskSpectrumT0 *taskMain = new AliAnalysisTaskSpectrumT0(taskName);
//  taskMain->AddEventID("101101010");
//  taskMain->AddEventID("1001101010");
//  taskMain->PrintEventIDs();
	mgr->AddTask(taskMain);

	taskMain->SetDebugLevel(debugLevel);
	TString outputName = outputFilename;
	if(!outputName.EndsWith(".root"))outputName += Form(".root");
	AliAnalysisDataContainer *cinput1 = mgr->GetCommonInputContainer();
	if (!cinput1) cinput1 = mgr->CreateContainer("cchain",TChain::Class(), AliAnalysisManager::kInputContainer);
	AliAnalysisDataContainer *coutput1 = mgr->CreateContainer("output",TList::Class(), AliAnalysisManager::kOutputContainer,outputName);
	mgr->ConnectInput(taskMain,0,cinput1);
	mgr->ConnectOutput(taskMain,1,coutput1);
	/// if you use the following line, your task only gets the selected events
	mgr->SetSkipTerminate(kFALSE);
	/// Enable debug printouts
	mgr->SetDebugLevel(debugLevel);
	if (!mgr->InitAnalysis())	return;
	mgr->PrintStatus();
	/// Start analysis in grid.
	cout<<endl<<"/////////////////////////////////////////////"<<endl;
	cout<<"Starting analysis..."<<endl;
	mgr->StartAnalysis("grid");
	cout<<"Done..."<<endl;
	cout<<endl<<"/////////////////////////////////////////////"<<endl;

};