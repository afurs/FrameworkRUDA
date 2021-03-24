#include "GridUtils.h"
using namespace utilities::grid;
/*******************************************************************************************************************/
TGridResult *GridUtils::makeCollectionXML(std::string pathGrid, std::string filename) {
  if(!gGrid) TGrid::Connect("alien://");
  std::string commandFind = "find " + pathGrid + " */"+ filename;
  std::cout<<"\nCreating XML collection with mask: "<<std::string{pathGrid + " */"+ filename};
  std::cout<<"\nCommand(find): "<<commandFind<<std::endl;
  TGridResult *alienResult = gGrid->Command(commandFind.c_str());
  return alienResult;
}
/*******************************************************************************************************************/
TGridResult *GridUtils::makeInputXML(std::string filepathResult, std::string filepathInputMask, bool makeXMLonGridSide) {
  if(!gGrid) TGrid::Connect("alien://");
  std::cout<<"\nDeleting old XML collection(if exists): "<<filepathResult<<std::endl;
  gGrid->Rm(filepathResult.c_str());
  std::cout<<"\nCreating XML collection with mask: "<<filepathInputMask;
  std::cout<<"\nDestionation: "<<filepathResult;
  std::string commandFind = "find";
  if(makeXMLonGridSide) {
    commandFind+=" -x ";
    commandFind+=filepathResult;
  }
  else {
    commandFind+=" -x tmp.xml";
  }
  commandFind+=" ";
  commandFind+=filepathInputMask;
  std::cout<<"\nCommand(find): "<<commandFind<<std::endl;
  TGridResult *alienResult = gGrid->Command(commandFind.c_str());
  alienResult->SetOwner(kTRUE);
  if(!makeXMLonGridSide) {
    std::string commandCp = "cp tmp.xml file://";
    commandCp+=filepathResult;
    TGridResult *alienResultCp = gGrid->Command(commandCp.c_str());
    std::cout<<"\nCommand(cp): "<<commandCp<<std::endl;
  }
  return alienResult;
}
/*******************************************************************************************************************/
void GridUtils::makeLocalInputXML(std::string filepathResult, std::string filepathInputMask) {
  std::string commandFind = "alien_find -x tmp.xml ";
  commandFind+=filepathInputMask;
  gSystem->Exec(commandFind.c_str());
  std::string commandCp = "alien_cp tmp.xml file://";
  commandCp+=filepathResult;
  gSystem->Exec(commandCp.c_str());
  std::string commandRm = "alien_rm tmp.xml";
  gSystem->Exec(commandRm.c_str());
}
/*******************************************************************************************************************/
std::map<unsigned int, std::string> GridUtils::makeMapRun2Path(std::string dirpath) {
  if(!gGrid) TGrid::Connect("alien://");
  std::string commandLs = "ls -al ";
  commandLs+=dirpath;
  std::map<unsigned int, std::string> mapRun2Path;
  TGridResult *alienResult = gGrid->Command(commandLs.c_str());
  for(const auto entryMap: (*alienResult)) {
    const TMap *mapResult = dynamic_cast<const TMap *>(entryMap);
    TObjString *objstName = dynamic_cast<TObjString *>(mapResult->GetValue("name"));
    TObjString *objstPath = dynamic_cast<TObjString *>(mapResult->GetValue("path"));
    TObjString *objstSize = dynamic_cast<TObjString *>(mapResult->GetValue("size"));
    if(std::stoi(std::string{objstSize->GetString()})==0) {
      mapRun2Path.insert({std::stoi(std::string{objstName->GetString()}),std::string{objstPath->GetString()}});
    }
  }
  return mapRun2Path;
}
/*******************************************************************************************************************/
std::map<unsigned int, std::string> GridUtils::makeMapRun2Filepath(std::string dirpath) {
  if(!gGrid) TGrid::Connect("alien://");
  std::string commandLs = "ls -al ";
  commandLs+=dirpath;
  std::map<unsigned int, std::string> mapRun2Filepath;
  TGridResult *alienResult = gGrid->Command(commandLs.c_str());
  for(const auto entryMap: (*alienResult)) {
    const TMap *mapResult = dynamic_cast<const TMap *>(entryMap);
    TObjString *objstName = dynamic_cast<TObjString *>(mapResult->GetValue("name"));
    TObjString *objstPath = dynamic_cast<TObjString *>(mapResult->GetValue("path"));
    TObjString *objstSize = dynamic_cast<TObjString *>(mapResult->GetValue("size"));
    if(std::stoi(std::string{objstSize->GetString()})>0) {
      mapRun2Filepath.insert({utilities::AnalysisUtils::getRunNum(std::string{objstName->GetString()}),std::string{objstPath->GetString()}});
    }
  }
  return mapRun2Filepath;
}
/*******************************************************************************************************************/
std::vector<std::string> GridUtils::makeListFilepath(std::string dirpath) {
  if(!gGrid) TGrid::Connect("alien://");
  std::string commandLs = "ls -al ";
  commandLs+=dirpath;
  std::vector<std::string> vecListFilepath;
  TGridResult *alienResult = gGrid->Command(commandLs.c_str());
  for(const auto entryMap: (*alienResult)) {
    const TMap *mapResult = dynamic_cast<const TMap *>(entryMap);
    TObjString *objstName = dynamic_cast<TObjString *>(mapResult->GetValue("name"));
    TObjString *objstPath = dynamic_cast<TObjString *>(mapResult->GetValue("path"));
    TObjString *objstSize = dynamic_cast<TObjString *>(mapResult->GetValue("size"));
    if(std::stoi(std::string{objstSize->GetString()})>0) {
      vecListFilepath.push_back(std::string{objstPath->GetString()});
    }
  }
  return vecListFilepath;
}
/*******************************************************************************************************************/
TList* GridUtils::getCollectionFromXML(TString filename) {
  TList *fFileGroupList = new TList;
  TXMLEngine xml;
  UInt_t parsedentries = 0;

   XMLDocPointer_t xdoc = xml.ParseFile(filename);
   if (!xdoc) {
      //Error("ParseXML", "cannot parse the xml file %s", filename.Data());
      return NULL;
   }

   XMLNodePointer_t xalien = xml.DocGetRootElement(xdoc);
   if (!xalien) {
      //Error("ParseXML", "cannot find the <alien> tag in %s",filename.Data());
      return NULL;
   }

   XMLNodePointer_t xcollection = xml.GetChild(xalien);
   if (!xcollection) {
      //Error("ParseXML", "cannot find the <collection> tag in %s",filename.Data());
      return NULL;
   }
   TString fCollectionName;
   if (xml.GetAttr(xcollection,"name")) {
      fCollectionName = TString(xml.GetAttr(xcollection,"name"));
   } else {
      fCollectionName = ("unnamed");
   }

   XMLNodePointer_t xevent = xml.GetChild(xcollection);;
   if (!xevent) {
      //Error("ParseXML", "cannot find the <event> tag in %s",filename.Data());
      return NULL;
   }
   UInt_t fNofGroups = 0;
   do {
      if (TString(xml.GetNodeName(xevent)) == "event") {
         parsedentries++;
         fNofGroups++;
         TMap *files = new TMap();

         // here is our event
         //      printf("Found event: %s\n",xml.GetAttr(xevent,"name"));

         // files
         XMLNodePointer_t xfile = xml.GetChild(xevent);
         if (!xfile)
            continue;

         Bool_t firstfile = kTRUE;
         do {
            // here we have an event file
            // get the attributes;
            xml.GetAttr(xfile, "lfn");
            xml.GetAttr(xfile, "turl");

            TMap *attributes = new TMap();
            TObjString *oname = new TObjString(xml.GetAttr(xfile, "name"));
            TObjString *oturl = new TObjString(xml.GetAttr(xfile, "turl"));
            TObjString *olfn = new TObjString(xml.GetAttr(xfile, "lfn"));
            TObjString *omd5 = new TObjString(xml.GetAttr(xfile, "md5"));
            TObjString *osize = new TObjString(xml.GetAttr(xfile, "size"));
            TObjString *oguid = new TObjString(xml.GetAttr(xfile, "guid"));
            TObjString *osurl = new TObjString(xml.GetAttr(xfile, "surl"));
            TObjString *osselect =
                new TObjString(xml.GetAttr(xfile, "select"));
            TObjString *ossexporturl =
                new TObjString(xml.GetAttr(xfile, "exporturl"));
            TObjString *osonline =
                new TObjString(xml.GetAttr(xfile, "online"));

            TObjString *oseStringlist =
                new TObjString(xml.GetAttr(xfile, "seStringlist"));
            TObjString *oevlist =
                new TObjString(xml.GetAttr(xfile, "evlist"));
            // if oevlist is defined, we parse it and fill a TEntyList
            if (oevlist && strlen(oevlist->GetName())) {
               TEntryList *xmlentrylist =
                   new TEntryList(oturl->GetName(), oguid->GetName());
               TString stringevlist = oevlist->GetName();
               TObjArray *evlist = stringevlist.Tokenize(",");
               for (Int_t n = 0; n < evlist->GetEntries(); n++) {
                  xmlentrylist->
                      Enter(atol
                            (((TObjString *) evlist->At(n))->GetName()));
               }
               attributes->Add(new TObjString("evlist"), xmlentrylist);
            }
            attributes->Add(new TObjString("name"), oname);
            attributes->Add(new TObjString("turl"), oturl);
            attributes->Add(new TObjString("lfn"), olfn);
            attributes->Add(new TObjString("md5"), omd5);
            attributes->Add(new TObjString("size"), osize);
            attributes->Add(new TObjString("guid"), oguid);
            attributes->Add(new TObjString("seStringlist"), oseStringlist);
            if (firstfile) {
               files->Add(new TObjString(""), attributes);
               firstfile = kFALSE;
            }

         } while ((xfile = xml.GetNext(xfile)));
         fFileGroupList->Add(files);
      }
   } while ((xevent = xml.GetNext(xevent)));
   return fFileGroupList;
}
