#ifndef GridManager_h
#define GridManager_h
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#include "AliAnalysisAlien.h"
#include "AliAnalysisGrid.h"
#include "TGrid.h"
#include "TGridResult.h"
#include "TGridJDL.h"
#include "TSystem.h"
#include "TObjString.h"
#include "TXMLEngine.h"
#include "TMap.h"
#include "TList.h"
#include "TEntryList.h"

//#include "AnalysisUtils.h"
#include "GridUtils.h"
class GridManager
{
public:

  GridManager()=default;
  ~GridManager()=default;
  //Input data mask
  std::string mPathInput;
  //Working directory path
  std::string mPathWorkingDir;
  //JDl directory path
  std::string mPathDirJDL;
  //Executables(main+validation) directory path
  std::string mPathDirExec;
  //ROOT macro path or filename
  std::string mPathMacro;
  //Additional fields in JDL
  std::map<std::string,std::string> mMapAdditionalFields;
  ///////////////////////////////
  //FIELDS FOR JDL GENERATOR
  ///////////////////////////////
  //Grid path to executable
  std::string mGridPathExec;
  //Grid path to validation executable
  std::string mGridPathValidationExec;
  //Output grid path
  std::string mGridPathOutputDir;
  //Grid input collection(XML file)
  std::string mGridPathInputCollXML;
  //List of grid paths for files to transfer
  std::vector<std::string> mVecPathGridInputFiles;
  ///////////////////////////////
  //username in alien
  std::string mUserName;
  //List of packages to add
  std::vector<std::string> mVecPackages;
  //List of output filenames
  std::vector<std::string> mVecOutputFilenames;
  //Name of file collection on nodem for example wn.xml
  std::string mFilenameSingleColl="wn.xml";
  //
  unsigned int mNInputFilesPerMaster=100;
  //Make main executable and validation executable
  void makeExecutables(std::string execFilename="",std::string validationFilename="");
  //Generate JDL
  TGridJDL* makeGridJDL();
  void makeGridJDL(std::string filenameJDL);

};

#endif
