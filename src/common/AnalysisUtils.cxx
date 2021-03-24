#include "AnalysisUtils.h"
using namespace utilities;
/*******************************************************************************************************************/
std::string AnalysisUtils::getFilename(std::string filepath) {
  const auto pos=filepath.find_last_of('/');
  return filepath.substr(pos+1);
}
/*******************************************************************************************************************/
unsigned int AnalysisUtils::getRunNum(std::string filepath) {
  std::string filename = getFilename(filepath);
  auto regRunNum = std::regex{"[2][\\d]{5}"};
  std::smatch sm;
  bool searchResult = std::regex_search(filename,sm,regRunNum);
  if(searchResult)  {
    return std::stoi(sm.str());
  }
  else {
    return 0;
  }
}
/*******************************************************************************************************************/

/*******************************************************************************************************************/

/*******************************************************************************************************************/

void AnalysisUtils::makePics(TList &listObjects,std::string path
                             ,std::function<void(TObject*)> changeHistState,Option_t *option) {
  TString filepathToSave;
  for(auto obj:listObjects) {
    filepathToSave=path;
    changeHistState(obj);
    if(!filepathToSave.EndsWith("/")) {
      filepathToSave+="/";
    }
    filepathToSave+=obj->GetName();
    filepathToSave+=".png";
    TCanvas canv("temp","temp",1920,1080);
    obj->Draw(option);
    canv.SetGridx();
    canv.SetGridy();
    canv.Print(filepathToSave);
  }
}
/*******************************************************************************************************************/
std::map<std::string,std::string> AnalysisUtils::makeMapNameTitle(std::string name,std::string title,unsigned int nEntries) {
  std::vector<int> vecEntries;
  for(int iEntry = 0;iEntry<nEntries;iEntry++) vecEntries.push_back(iEntry+1);
  return makeMapNameTitle(name,title,vecEntries);
}
/*******************************************************************************************************************/
std::map<std::string,std::string> AnalysisUtils::makeMapNameTitle(std::string name,std::string title,std::vector<int> vecEntries) {
  std::map<std::string,std::string> mapResult;
  for(const auto &entry: vecEntries)  {
    mapResult.insert({Form(name.c_str(),entry),Form(title.c_str(),entry)});
  }
  return mapResult;
}
/*******************************************************************************************************************
TList *AnalysisUtils::makeListHists1D(std::map<std::string,std::string> mapHistNameTitle,int nBins, double xMin,double xMax) {
  if(nBins<1)	return nullptr;
  if(!((xMax-xMin)>0))	return nullptr;
  if(mapHistNameTitle.size()==0)	return nullptr;
  std::string name,title;
  TList *listResult = new TList();
  listResult->SetOwner(kTRUE);
  for(const auto &histID: mapHistNameTitle) {
    name = histID.first;
    title = histID.second;
    listResult->Add(new TH1D(name.c_str(),title.c_str(),nBins,xMin,xMax));
  }
  return listResult;
}
*******************************************************************************************************************/
std::vector<std::string> AnalysisUtils::makeVecFilepaths(std::string path, std::regex regExpr)  {
  std::vector<std::string> vecFilepaths = makeVecFilepaths(path);
  //std::regex{".*\\.root"};
  auto predRegex = [&regExpr](const std::string &el)->bool  {return !std::regex_match(el,regExpr);};
  vecFilepaths.erase(std::remove_if(vecFilepaths.begin(),vecFilepaths.end(),predRegex),vecFilepaths.end());
  return vecFilepaths;
}
/*******************************************************************************************************************/
std::vector<std::string> AnalysisUtils::makeVecFilepaths(std::string path)  {
  std::vector<std::string> vecFilepaths;
  auto dirIt = boost::filesystem::recursive_directory_iterator(path);
  for (const auto& entry : dirIt) {
    //if (boost::filesystem::is_regular(entry)) vecFilepaths.push_back(entry.path().filename().native());
    if (boost::filesystem::is_regular(entry)) vecFilepaths.push_back(entry.path().native());
  }
  return vecFilepaths;
}
/*******************************************************************************************************************/
std::map<unsigned int,std::vector<std::string>> AnalysisUtils::makeMapRunsToFilepaths(std::string path, std::regex regExpr) {
  std::map<unsigned int,std::vector<std::string>> mapResult;
  std::vector<std::string> vecAllFilepaths = makeVecFilepaths(path,regExpr);
  for(const auto& filepath: vecAllFilepaths)  {
    unsigned int runNum = getRunNum(filepath);
    if(runNum!=0) {
      auto [it,isInserted] = mapResult.insert({runNum,std::vector<std::string>{}});
      it->second.push_back(filepath);
    }
  }
  return mapResult;
}
/*******************************************************************************************************************
bool AnalysisUtils::writeListResultToFile(TList *listOutput, std::string fileName) {
  return true;
}
*******************************************************************************************************************
bool AnalysisUtils::writeListResultToFile(TList *listOutput, TFile *fileOutput) {
  return true;
}
*******************************************************************************************************************
bool AnalysisUtils::writeObjToFile(TObject *objOutput, std::string fileName) {
  return true;
}

*******************************************************************************************************************/
bool AnalysisUtils::writeObjToFile(TObject *objOutput, TFile *fileOutput)	{
  if(objOutput==nullptr||fileOutput==nullptr) {
    std::cout<<"\nCannot write TObject to file!\n";
    return false;
  }
  std::cout<<"\n################################################################";
  std::cout<<"\n#Saving "<<objOutput->GetName()<<" to file: "<<fileOutput->GetName()<<std::endl;
  objOutput->Print();
  std::cout<<std::endl;
  fileOutput->WriteObject(objOutput,objOutput->GetName(),"SingleKey");
  std::cout<<"################################################################\n";
  return true;
}
