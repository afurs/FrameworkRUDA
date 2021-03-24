///Change AliMyTaskTemplate to class you need
#include "AliTaskAgeingT0.h"
void runGridTask(AliAnalysisGrid *alienHandler,TString outputFilename){
	if (!alienHandler) return;Int_t debugLevel = 2;
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
  alienHandler->SetMergeViaJDL();
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
  gInterpreter->LoadMacro("AliTaskAgeingT0.cxx+g");
	TString taskName= outputFilename;
	taskName+=Form("_task");

 // AliAnalysisTaskMyTask *task = reinterpret_cast<AliAnalysisTaskMyTask*>(gInterpreter->ExecuteMacro("AddMyTask.C"));

  AliTaskAgeingT0 *taskMain = new AliTaskAgeingT0(taskName);
	mgr->AddTask(taskMain);

	taskMain->SetDebugLevel(debugLevel);
	TString outputName = outputFilename;
	if(!outputName.EndsWith(".root"))outputName += Form(".root");
	AliAnalysisDataContainer *cinput1 = mgr->GetCommonInputContainer();
	if (!cinput1) cinput1 = mgr->CreateContainer("cchain",TChain::Class(), AliAnalysisManager::kInputContainer);
	AliAnalysisDataContainer *coutput1 = mgr->CreateContainer("outputList",TList::Class(), AliAnalysisManager::kOutputContainer,outputName);
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
}
