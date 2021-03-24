#include "HistUtils.h"
using namespace utilities;


/*******************************************************************************************************************/
TList * Hists::makeListHists1D(std::map<std::string,std::string> mapHistNameTitle,int nBins, double xMin,double xMax) {
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
