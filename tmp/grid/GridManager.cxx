#include "GridManager.h"
using namespace utilities::grid;
/*******************************************************************************************************************/
void GridManager::makeExecutables(std::string execFilename, std::string validationFilename) {
  AliAnalysisAlien *alienAnalysis = new AliAnalysisAlien();
  alienAnalysis->SetRunMode("offline");
  alienAnalysis->SetDefaultOutputs(kFALSE);
  std::string outputFiles="";
  for(const auto &entry:mVecOutputFilenames) {
    outputFiles+=entry;
    outputFiles+=" ";
  }
  alienAnalysis->SetOutputFiles(outputFiles.c_str());
  alienAnalysis->SetAnalysisMacro((mPathMacro.substr(mPathMacro.find_last_of('/')+1)).c_str());
  if(validationFilename=="") {
    alienAnalysis->SetValidationScript((mGridPathValidationExec.substr(mGridPathValidationExec.find_last_of('/')+1)).c_str());
  }
  else {
    alienAnalysis->SetValidationScript(validationFilename.c_str());
  }

  if(execFilename==""){
    alienAnalysis->SetExecutable((mGridPathExec.substr(mGridPathExec.find_last_of('/')+1)).c_str());
  }
  else {
    alienAnalysis->SetExecutable(execFilename.c_str());
  }

  alienAnalysis->WriteExecutable();
  alienAnalysis->WriteValidationScript();
}
/*******************************************************************************************************************/
TGridJDL *GridManager::makeGridJDL() {
  if(!gGrid) TGrid::Connect("alien://");
  TGridJDL *gridJDL= gGrid->GetJDLGenerator();
  //GRID PATHS
  //Setting input collection(xml file)
  gridJDL->AddToInputDataCollection(std::string{"LF:"+mGridPathInputCollXML+",nodownload"}.c_str());
  //Setting output directory
  //gridJDL->SetOutputDirectory(std::string{(mGridPathOutputDir+"/$2/#alien_counter_03i#")}.c_str());
  gridJDL->SetOutputDirectory(std::string{(mGridPathOutputDir+"/#alien_counter_03i#")}.c_str());
  //Adding packges
  for(const auto &entry:mVecPackages) {
    gridJDL->AddToPackages(std::string{entry.substr(0,entry.find_last_of("::")-1)}.c_str(),std::string{entry.substr(entry.find_last_of("::")+1)}.c_str());
  }
  //Adding input files
  for(const auto &entry:mVecPathGridInputFiles) {
    gridJDL->AddToInputSandbox(std::string{"LF:"+entry}.c_str());
  }
  //Setting grid path to executable
  gridJDL->SetExecutable(mGridPathExec.c_str());
  //Setting grid path to validation executable
  gridJDL->SetValidationCommand(mGridPathValidationExec.c_str());
  //MINOR OPTIONS
  //Setting name for single collection(wn.xml)
  gridJDL->SetInputDataList(mFilenameSingleColl.c_str());
  gridJDL->SetSplitMode("se",mNInputFilesPerMaster);
  gridJDL->SetTTL(30000);
  gridJDL->SetPrice(1);
  gridJDL->SetInputDataListFormat("xml-single");
  //Adding outputfilenames
  for(const auto &entry:mVecOutputFilenames) {
    //gridJDL->AddToOutputSandbox(entry.c_str());
    gridJDL->AddToSet("Output",entry.c_str());
  }
  gridJDL->AddToSet("Output","log_archive:stderr,stdout,rec.log@disk=1");
  gridJDL->SetValue("User",std::string{"\""+mUserName+"\""}.c_str());
  gridJDL->SetValue("JDLVariables","{\"Packages\", \"OutputDir\"}");
  gridJDL->SetValue("Workdirectorysize","{\"5000MB\"}");
  for(const auto &entry: mMapAdditionalFields) {
    gridJDL->SetValue(entry.first.c_str(),entry.second.c_str());
  }
  return gridJDL;
}
/*******************************************************************************************************************/
void GridManager::makeGridJDL(std::string filenameJDL) {
  auto gridJDL = makeGridJDL();
  TString sjdl = gridJDL->Generate();
  // Write jdl to file

  std::ofstream out;
  out.open(filenameJDL.c_str(), std::ios::out);
  if (out.bad()) {
     Error("WriteJDL", "Bad file name: %s", filenameJDL.c_str());
     return;
  }

  out << sjdl << std::endl;
  out.close();
  delete gridJDL;
}
/*******************************************************************************************************************/
