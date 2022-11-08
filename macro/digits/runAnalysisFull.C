#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/LookUpTable.h"
#include "DataFormatsParameters/GRPLHCIFData.h"
#include "FT0Base/Geometry.h"

#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>

#include "CommonRUDA/HistUtils.h"
#include "CommonRUDA/AnalysisUtils.h"
#include "ROOT/TProcessExecutor.hxx"

#include <vector>
#include <array>
#include <set>
#include <map>
#include <regex>
#include <utility>
#include <algorithm>


/*
HINTS:

Trigger getters
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/common/include/DataFormatsFIT/Triggers.h#L59-L71

PM bits
https://github.com/AliceO2Group/AliceO2/blob/75c6ae331dfb78622221e1c760a9a6e8af175380/DataFormats/Detectors/FIT/FT0/include/DataFormatsFT0/ChannelData.h#L37-L44

*/
const int sNchannelsFT0 = 212;
const float sNSperTimeChannel=o2::ft0::Geometry::ChannelWidth*1e-3; // nanoseconds per time channel, 0.01302
const float sNS2Cm = 29.97; // light NS to Centimetres
const int sNBC = 3564; // Number of BCs per Orbit
const int sOrbitPerTF=128;
const int sNBCperTF = sNBC*sOrbitPerTF;
const int sTFrate = 88;
const int sNBCperSec = sNBC*sOrbitPerTF*sTFrate;
const int sNchannelsA = 96;
const int sNchannelsC = 112;
const int sNchannelsAC = sNchannelsA+sNchannelsC;
using HistHelper = utilities::Hists;
using Utils = utilities::AnalysisUtils;

void writeResult(TList *listOutput, const std::string &filepathOutput);
std::bitset<sNBC> getCollBC(unsigned int runnum);
void processDigits(unsigned int runnum , const std::string &filepathInput="o2_ft0digits.root",const std::string &filepathOutput="hists.root",const std::bitset<sNBC> &argCollBC={});

void runAnalysisFull(const std::string &pathToSrc = "/data/work/run3/digits/production",const std::string &pathToOutput = "hists") {
  //Load libraries
  gSystem->Load("libboost_filesystem.so");
  gSystem->Load("$RUDA_ROOT/lib/libCommonRUDA.so");
  auto mapRunToFilepaths = Utils::makeMapRunsToFilepathsROOT(pathToSrc);
  std::size_t nParallelJobs=20;
  ROOT::TProcessExecutor pool(nParallelJobs);
  struct Parameters {
    unsigned int runnum{};
    std::string filepathInput{};
    std::string filepathOutput{};
    std::bitset<sNBC> collBC{};
  };
  std::vector<Parameters> vecParams{};
  for(const auto &entry: mapRunToFilepaths) {
    const auto &runnum = entry.first;
    const auto &vecFilepaths = entry.second;
//    if(runnum!=522236) continue;
    const std::string filepath = vecFilepaths[0];
    const std::string outputFile = pathToOutput+"/"+"hist"+std::to_string(runnum)+".root";
    std::cout<<"\nPreparing run "<<runnum<<" in file "<<filepath<<" into output file "<<outputFile<<std::endl;
    vecParams.push_back({runnum,filepath,outputFile,getCollBC(runnum)});
//    processDigits(runnum,filepath,outputFile);
  }
  const auto result = pool.Map([](const Parameters &entry) {
          processDigits(entry.runnum,entry.filepathInput,entry.filepathOutput,entry.collBC);
          return 0;}
          , vecParams);
}

void processDigits(unsigned int runnum, const std::string &filepathInput, const std::string &filepathOutput,const std::bitset<sNBC> &argCollBC)
{
  //Constants
  const std::string treeName="o2sim";
  std::map<int,float> mapChID2Amp;
  const std::map<unsigned int, std::string> mMapTrgNames = {
    {o2::fit::Triggers::bitA+1, "OrA"},
    {o2::fit::Triggers::bitC+1, "OrC"},
    {o2::fit::Triggers::bitCen+1, "Cen"},
    {o2::fit::Triggers::bitSCen+1, "SCen"},
    {o2::fit::Triggers::bitVertex+1, "Vertex"},
    {o2::fit::Triggers::bitLaser+1, "Laser"},
    {o2::fit::Triggers::bitOutputsAreBlocked+1,"OutputAreBlocked"},
    {o2::fit::Triggers::bitDataIsValid+1, "DataIsValid"}};
  const std::map<unsigned int, std::string> mMapPMbits = {
    {o2::ft0::ChannelData::kNumberADC+1, "NumberADC" },
    {o2::ft0::ChannelData::kIsDoubleEvent+1, "IsDoubleEvent" },
    {o2::ft0::ChannelData::kIsTimeInfoNOTvalid+1, "IsTimeInfoNOTvalid" },
    {o2::ft0::ChannelData::kIsCFDinADCgate+1, "IsCFDinADCgate" },
    {o2::ft0::ChannelData::kIsTimeInfoLate+1, "IsTimeInfoLate" },
    {o2::ft0::ChannelData::kIsAmpHigh+1, "IsAmpHigh" },
    {o2::ft0::ChannelData::kIsEventInTVDC+1, "IsEventInTVDC" },
    {o2::ft0::ChannelData::kIsTimeInfoLost+1, "IsTimeInfoLost" }};
  uint8_t pmBitsGood = (1<<o2::ft0::ChannelData::kIsCFDinADCgate) | (1<<o2::ft0::ChannelData::kIsEventInTVDC);
  uint8_t pmBitsBad = (1<<o2::ft0::ChannelData::kIsDoubleEvent)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoNOTvalid)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLate)
                    | (1<<o2::ft0::ChannelData::kIsAmpHigh)
                    | (1<<o2::ft0::ChannelData::kIsTimeInfoLost);
  uint8_t pmBitsToCheck = pmBitsGood | pmBitsBad; //All except kNumberADC
  std::cout<<"\nGOOD PM BITS: "<<static_cast<int>(pmBitsGood);
  std::cout<<"\nBAD PM BITS: "<<static_cast<int>(pmBitsBad);
  std::cout<<"\nCHECK PM BITS: "<<static_cast<int>(pmBitsToCheck);

  //
  std::array<int,sNchannelsFT0> arrAmp{};
  std::array<int,sNchannelsFT0> arrTime{};

  std::bitset<sNBC> collBC_tmp=argCollBC;
  if(collBC_tmp.count()==0) {
    collBC_tmp = getCollBC(runnum);
  }
  const auto collBC = collBC_tmp;

  TList *listOutput = new TList();
  listOutput->SetOwner(true);
  listOutput->SetName("output");

  TList *listAmpPerChIDPerBC = new TList();
  listAmpPerChIDPerBC->SetName("listAmpPerChIDPerBC");
  listAmpPerChIDPerBC->SetOwner(true);
  listOutput->Add(listAmpPerChIDPerBC);


  TList *listTriggers = new TList();
  listTriggers->SetName("listTriggers");
  listTriggers->SetOwner(true);
  listOutput->Add(listTriggers);


  /*
  *PREPARE HISTOGRAMS AND ADD THEM INTO LIST WITH OUTPUTS
  */
  // Amp and time
  TH2F *hAmpPerChID = new TH2F("hAmpPerChannel","Amplitude per chID;chID;Amp",sNchannelsFT0,0,sNchannelsFT0,4100,-100,4000);
  listOutput->Add(hAmpPerChID);

  TH2F *hTimePerChID = new TH2F("hTimePerChannel","Time per chID;chID;Time",sNchannelsFT0,0,sNchannelsFT0,4000,-2000,2000);
  listOutput->Add(hTimePerChID);

  TH2F *hTimePerChID_goodPMbits = new TH2F("hTimePerChannel_goodPMbits","Time per chID(IsCFDinADCgate && IsEventInTVDC, no bad PM bits);chID;Time",sNchannelsFT0,0,sNchannelsFT0,4000,-2000,2000);
  listOutput->Add(hTimePerChID_goodPMbits);

  TH2F *hTimePerChIDoffsets = new TH2F("hTimePerChannelOffsets","Time per chID(amp>4 && abs(time)<153 && only good PM bits && vertexTrg);chID;Time",sNchannelsFT0,0,sNchannelsFT0,4000,-2000,2000);
  listOutput->Add(hTimePerChIDoffsets);


  TH2F *hAmpPerBC = new TH2F("hAmpPerBC","Amplitude per BC;BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listOutput->Add(hAmpPerBC);

  TH2F *hTimePerBC = new TH2F("hTimePerBC","Time per BC;BC;Time",sNBC,0,sNBC,4000,-2000,2000);
  listOutput->Add(hTimePerBC);
  //Collision time and vertex
  TH1F *hCollisionTime = new TH1F("hCollisionTime","Collision time;Collision time [ns]",400,-5,5);
  listOutput->Add(hCollisionTime);
  TH1F *hCollisionTime_vrtTrg = new TH1F("hCollisionTime_vrtTrg","Collision time(vertex trigger);Collision time [ns]",400,-5,5);
  listOutput->Add(hCollisionTime_vrtTrg);
  TH1F *hCollisionTime_noVrtTrg = new TH1F("hCollisionTime_noVrtTrg","Collision time(w/o vertex trigger);Collision time [ns]",400,-5,5);
  listOutput->Add(hCollisionTime_noVrtTrg);

  TH1F *hVertexNoCut = new TH1F("hVertexNoCut","Vertex position(w/o cuts);Vertex [cm]",2000,-200,200);
  listOutput->Add(hVertexNoCut);

  TH1F *hVertex = new TH1F("hVertex","Vertex position;Vertex [cm]",600,-30,30);
  listOutput->Add(hVertex);
  TH1F *hVertex_vrtTrg = new TH1F("hVertex_vrtTrg","Vertex position (vertex trigger);Vertex [cm]",600,-30,30);
  listOutput->Add(hVertex_vrtTrg);
  TH1F *hVertex_noVrtTrg = new TH1F("hVertex_noVrtTrg","Vertex position (w/o vertex trigger);Vertex [cm]",600,-30,30);
  listOutput->Add(hVertex_noVrtTrg);

//  TH2F *hCollisionTimeVsVertex = new TH2F("hCollisionTimeVsVertex","Collision time vs Vertex position;Vertex [cm];Collision time [ns]",600,-30,30,400,-5,5);


  TH2F *hCollisionTimeVsVertex_noCut = new TH2F("hCollisionTimeVsVertex_noCut","Collision time vs Vertex position(w/o cuts);Vertex [cm];Collision time [ns]",2000,-100,100,1000,-50,50);
  listOutput->Add(hCollisionTimeVsVertex_noCut);


  TH2F *hCollisionTimeVsVertex = new TH2F("hCollisionTimeVsVertex","Collision time vs Vertex position;Vertex [cm];Collision time [ns]",1000,-50,50,1000,-50,50);
  listOutput->Add(hCollisionTimeVsVertex);
  TH2F *hCollisionTimeVsVertex_vrtTrg = new TH2F("hCollisionTimeVsVertex_vrtTrg","Collision time vs Vertex position (vertex trigger);Vertex [cm];Collision time [ns]",600,-30,30,400,-5,5);
  listOutput->Add(hCollisionTimeVsVertex_vrtTrg);
  TH2F *hCollisionTimeVsVertex_noVrtTrg = new TH2F("hCollisionTimeVsVertex_noVrtTrg","Collision time vs Vertex position (w/o vertex trigger);Vertex [cm];Collision time [ns]",600,-30,30,400,-5,5);
  listOutput->Add(hCollisionTimeVsVertex_noVrtTrg);

  TH2F *hVertexVsBC_vrtTrg = new TH2F("hVertexVsBC_vrtTrg","Vertex position vs BC (vertex trigger);BC;Vertex [cm]",sNBC,0,sNBC,600,-30,30);
  listOutput->Add(hVertexVsBC_vrtTrg);

  TH2F *hVertexVsBC_noVrtTrg = new TH2F("hVertexVsBC_noVrt","Vertex position vs BC (w/o vertex trigger);BC;Vertex [cm]",sNBC,0,sNBC,600,-30,30);
  listOutput->Add(hVertexVsBC_noVrtTrg);


  //
  TH1F *hTriggers = new TH1F("hTriggers","Triggers",mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggers,mMapTrgNames,0);
  listTriggers->Add(hTriggers);
  TH2F *hTriggersCorr = new TH2F("hTriggersCorr","Trigger correlations",mMapTrgNames.size(),0,mMapTrgNames.size(),mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersCorr,mMapTrgNames,0);
  HistHelper::makeHistBinNamed(hTriggersCorr,mMapTrgNames,1);
  listTriggers->Add(hTriggersCorr);


  TH2F *hTriggersVsBC = new TH2F("hTriggersVsBC","Triggers vs BC;BC;Triggers",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hTriggersVsBC,mMapTrgNames,1);
  listTriggers->Add(hTriggersVsBC);

  TH1F *hBC_trgVrt = new TH1F("hBC_trgVrt","BC (Vertex trigger);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_trgVrt);

  TH1F *hNChanA = new TH1F("hNChanA","Number of channels side A;nChannels",sNchannelsA,0,sNchannelsA);
  listOutput->Add(hNChanA);

  TH1F *hNChanC = new TH1F("hNChanC","Number of channels side C;nChannels",sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanC);

  TH2F *hNChanAvsC = new TH2F("hNChanAvsC","Number of channels side A vs C;nChannelsA;nChannelsC",sNchannelsC,0,sNchannelsC,sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanAvsC);

  TH2F *hNChanAvsC_vrtTrg = new TH2F("hNChanAvsC_vrtTrg","Number of channels side A vs C(vertex trigger);nChannelsA;nChannelsC",sNchannelsC,0,sNchannelsC,sNchannelsC,0,sNchannelsC);
  listOutput->Add(hNChanAvsC_vrtTrg);

  TH1F *hNChannels = new TH1F("hNChannels","Number of channels;nChannels",sNchannelsFT0,0,sNchannelsFT0);
  listOutput->Add(hNChannels);

  TH2F *hAmpAfterCollision = new TH2F("hAmpAfterCollision","Amplitudes after collision BC;BC;Amp",sNBC,0,sNBC,4100,-100,4000);
  listOutput->Add(hAmpAfterCollision);

  TH2F *hAmpCollBC_afterCollision = new TH2F("hAmpCollBC_afterCollision","Amplitudes in collision BC vs next BC(closest to collision BC in given channel);Next BC;Amp(Collision BC)",sNBC,0,sNBC,4100,-100,4000);
  listOutput->Add(hAmpCollBC_afterCollision);

  TH2F *hChID_afterCollision = new TH2F("hChID_afterCollision","ChannelID after collision BC;BC;Amp",sNBC,0,sNBC,sNchannelsFT0,0,sNchannelsFT0);
  listOutput->Add(hChID_afterCollision);

  TH2F *hBCvsTrg_OutOfColl = new TH2F("hBCvsTrg_OutOfColl","BC vs Trg, out of collision;BC",sNBC,0,sNBC,mMapTrgNames.size(),0,mMapTrgNames.size());
  HistHelper::makeHistBinNamed(hBCvsTrg_OutOfColl,mMapTrgNames,1);
  listTriggers->Add(hBCvsTrg_OutOfColl);

  TH1F *hBC_VrtTrg_OutOfColl = new TH1F("hBC_VrtTrg_OutOfColl","BC(Vertex Trigger, out of collision)",sNBC,0,sNBC);
  listOutput->Add(hBC_VrtTrg_OutOfColl);

  TH2F *hAmpVsPMbits = new TH2F("hAmpVsPMbits","Amplitude vs PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4100,-100,4000);
  listOutput->Add(hAmpVsPMbits);
  HistHelper::makeHistBinNamed(hAmpVsPMbits,mMapPMbits,0);

  TH2F *hAmpVsNotPMbits = new TH2F("hAmpVsNotPMbits","Amplitude vs negative PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4100,-100,4000);
  listOutput->Add(hAmpVsNotPMbits);
  HistHelper::makeHistBinNamed(hAmpVsNotPMbits,mMapPMbits,0);

  TH2F *hTimeVsPMbits = new TH2F("hTimeVsPMbits","Time vs PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4000,-2000,4000);
  listOutput->Add(hTimeVsPMbits);
  HistHelper::makeHistBinNamed(hTimeVsPMbits,mMapPMbits,0);

  TH2F *hTimeVsNotPMbits = new TH2F("hTimeVsNotPMbits","Time vs negative PM bits;PM bits;Amp",mMapPMbits.size(),0,mMapPMbits.size(),4000,-2000,4000);
  listOutput->Add(hTimeVsNotPMbits);
  HistHelper::makeHistBinNamed(hTimeVsNotPMbits,mMapPMbits,0);



  std::array<TH2F*,sNchannelsAC> arrHistAmpPerChIDPerBC_afterCollission{};
  std::array<TH2F*,sNchannelsAC> arrHistAmpPerChIDPerCollBC_afterCollission{};

  std::set setChID{1,26};
  for(const auto &chID : setChID) {
    TH2F *hist = new TH2F(Form("hAmpBC_afterCollision%i",chID),Form("Amplitude ch%i after collision BC;BC;Amp",chID),sNBC,0,sNBC,4100,-100,4000);
    arrHistAmpPerChIDPerBC_afterCollission[chID]=hist;
    listAmpPerChIDPerBC->Add(hist);

    hist = new TH2F(Form("hAmpCollBC_afterCollision%i",chID),Form("Amplitude ch%i in collision BC vs next BC(closest to collision BC in given channel);Next BC;Amp(Collision BC)",chID),sNBC,0,sNBC,4100,-100,4000);
    arrHistAmpPerChIDPerCollBC_afterCollission[chID]=hist;
    listAmpPerChIDPerBC->Add(hist);
  }
/*
  for(int iCh=0;iCh<1;iCh++) {
    TH2F *hist = new TH2F(Form("hAmpPerBC_ch%i",iCh),Form("Amplitude ch%i per BC;BC;Amp",iCh),sNBC,0,sNBC,4100,-100,4000);
    arrHistAmpPerChIDPerBC[iCh]=hist;
    listAmpPerChIDPerBC->Add(hist);
  }
*/
  TH1F *hBC_OrA_notVrt_collBC = new TH1F("hBC_OrA_notVrt_collBC","BC (only colliding BCs, OrA && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrA_notVrt_collBC);

  TH1F *hBC_OrC_notVrt_collBC = new TH1F("hBC_OrC_notVrt_collBC","BC (only colliding BCs, OrC && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrC_notVrt_collBC);

  TH1F *hBC_OrA_notVrt_nonCollBC = new TH1F("hBC_OrA_notVrt_nonCollBC","BC (only non-colliding BCs, OrA && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrA_notVrt_nonCollBC);

  TH1F *hBC_OrC_notVrt_nonCollBC = new TH1F("hBC_OrC_notVrt_nonCollBC","BC (only non-colliding BCs, OrC && !Vrt);BC",sNBC,0,sNBC);
  listTriggers->Add(hBC_OrC_notVrt_nonCollBC);

  ////
  typedef typename o2::ft0::Digit Digit;
  typedef typename o2::ft0::ChannelData ChannelData;
  std::vector<Digit> vecDigits;
  std::vector<Digit> *ptrVecDigits = &vecDigits;
  std::vector<ChannelData> vecChannelData;
  std::vector<ChannelData> *ptrVecChannelData = &vecChannelData;
  std::size_t mCntEvents{};
  std::vector<std::string> vecFilenames{};
  vecFilenames.push_back(filepathInput);// One can parse later line with filelist
  for(const auto &filepathInput:vecFilenames) {
    std::cout<<"\nProcessing file: "<<filepathInput<<std::endl;
    TFile fileInput(filepathInput.c_str(),"READ");
    if(!fileInput.IsOpen()) {
      std::cout<<"\nWARNING! CANNOT OPEN FILE: "<<filepathInput<<std::endl;
      return;
    }
    TTree* treeInput = dynamic_cast<TTree*>(fileInput.Get(treeName.c_str()));
    if(treeInput==nullptr) {
      std::cout<<"\nWARNING! CANNOT FIND TREE: "<<treeName<<std::endl;
      return;
    }
    treeInput->SetBranchAddress("FT0DIGITSBC", &ptrVecDigits);
    treeInput->SetBranchAddress("FT0DIGITSCH", &ptrVecChannelData);
    std::size_t nTotalTFs = treeInput->GetEntries();
    std::size_t nPercents = 10;
    std::size_t stepTF = nPercents*nTotalTFs/100; //step for 10%
    if(stepTF==0) stepTF=nTotalTFs;
    std::cout<<"\nTotal number of TFs: "<<nTotalTFs<<std::endl;
    //Accumulating events into vector
    for (int iEvent = 0; iEvent < treeInput->GetEntries(); iEvent++) {
      //Iterating TFs in tree
      treeInput->GetEntry(iEvent);
      mCntEvents++;
      if(mCntEvents%stepTF==0) std::cout<<nPercents*mCntEvents/stepTF<<"% processed"<<std::endl;

      uint32_t orbitOld{};
      std::array<uint16_t, sNchannelsAC> arrChID_lastBC{};
      arrChID_lastBC.fill(0xffff);
      std::array<std::bitset<sNBC>, sNchannelsAC> arrChID_BC{};

      for(const auto &digit : vecDigits) {
        //Iterating events(Digits)
        const auto& channels = digit.getBunchChannelData(vecChannelData);
        //VARIABLES TO USE
        const auto &ir = digit.mIntRecord;
        const auto &bc = ir.bc;
        const auto &orbit = ir.orbit;
        const auto &trg = digit.mTriggers;
        const auto &trgBits = digit.mTriggers.triggersignals;
        const auto &nChA = digit.mTriggers.nChanA;
        const auto &nChC = digit.mTriggers.nChanC;
        const auto &sumAmpA = digit.mTriggers.amplA;
        const auto &sumAmpC = digit.mTriggers.amplC;
        const auto &averageTimeA = digit.mTriggers.timeA;
        const auto &averageTimeC = digit.mTriggers.timeC;
        /*
        **PUT HERE CODE FOR PROCESSING DIGITS
        */
//        if(!collBC.test(bc)) continue;
        if(orbit!=orbitOld) {
          //new orbit
          orbitOld=orbit;
          arrChID_lastBC.fill(0xffff); // reset last BC per channel
          std::for_each(arrChID_BC.begin(),arrChID_BC.end(),[](auto &entry) {entry.reset();});
        }
        const double secSinceSOR = 1.*(orbit*sNBC+bc)/sNBCperSec;
        const bool isEventVertex = (trg.getVertex() && trg.getDataIsValid() && !trg.getOutputsAreBlocked());
/*        if(nChA>0 && nChC>0 && trg.getVertex()) {
          const float collTime = (averageTimeA + averageTimeC)/2 * sNSperTimeChannel;
          const float vrtPos = (averageTimeA - averageTimeC)/2 * sNSperTimeChannel * sNS2Cm;
          hCollisionTimeVsVertex->Fill(collTime,vrtPos);
        }
*/
        std::vector<uint8_t> vecPMbits{};
        const bool isCollision = collBC.test(bc);

        for(int i=0;i<8;i++)  {
          if(trgBits & (1<<i)) {
            hTriggers->Fill(i);
            hTriggersVsBC->Fill(bc,i);
            if(!isCollision) {
              hBCvsTrg_OutOfColl->Fill(bc,i);
            }
            for(int j=i+1;j<8;j++) {
              if(trgBits & (1<<j)) hTriggersCorr->Fill(i,j);
            }
          }
        }
        std::bitset<sNchannelsFT0> bsChID{};
        int nChanA{};
        int nChanC{};
        double meanTimeA{};
        double meanTimeC{};

        int nChanA_noCut{};
        int nChanC_noCut{};
        double meanTimeA_noCut{};
        double meanTimeC_noCut{};

        //
        //
        for(const auto &channelData: channels) {
          //Iterating over ChannelData(PM data) per given Event(Digit)
          //VARIABLEES TO USE
          const auto &amp = channelData.QTCAmpl;
          const auto &time = channelData.CFDTime;
          const auto &chID = channelData.ChId;
          const auto &pmBits = channelData.ChainQTC;
          if(chID>=sNchannelsAC) continue;
          /*
          **PUT HERE ANALYSIS CODE
          */
          //vecAmpsVsBC[chID]->Fill(bc,amp);
          //vecAmpsTrend[chID]->Fill(secSinceSOR,amp);
          /*
          if(chID==0) {
            arrHistAmpPerChIDPerBC[chID]->Fill(bc,amp);
          }
          */
          if(chID<sNchannelsA) {
            nChanA_noCut++;
            meanTimeA_noCut+=time;
          }
          else if(chID<sNchannelsAC){
            nChanC_noCut++;
            meanTimeC_noCut+=time;
          }

          hTimePerChID->Fill(chID,time);
          if(amp>5 && std::abs(time)<153 && ((pmBits & pmBitsToCheck) == pmBitsGood)) {
            if(isEventVertex) hTimePerChIDoffsets->Fill(chID,time);
            vecPMbits.push_back(pmBits);
            if(chID<sNchannelsA) {
              nChanA++;
              meanTimeA+=time;
            }
            else if(chID<sNchannelsAC){
              nChanC++;
              meanTimeC+=time;
            }
          }
          bsChID.set(chID);
          if((pmBits & pmBitsToCheck) == pmBitsGood) {
            hTimePerChID_goodPMbits->Fill(chID,time);
          }
          hTimePerBC->Fill(bc,time);
          if(pmBits & (1 << ChannelData::EEventDataBit::kIsCFDinADCgate)) {
            hAmpPerChID->Fill(chID,amp);
            hAmpPerBC->Fill(bc,amp);
          }
          if(isCollision) {
            arrChID_BC[chID].set(bc);
          }
          else if(arrChID_lastBC[chID]!=0xffff) {
            if(collBC.test(arrChID_lastBC[chID])) {
              hAmpAfterCollision->Fill(bc,amp);
              hAmpCollBC_afterCollision->Fill(bc,arrAmp[chID]);
              hChID_afterCollision->Fill(bc,chID);
              if(setChID.find(chID)!=setChID.end()) {
                arrHistAmpPerChIDPerBC_afterCollission[chID]->Fill(bc,amp);
                arrHistAmpPerChIDPerCollBC_afterCollission[chID]->Fill(bc,arrAmp[chID]);
              }
            }
          }
          for(int i=0;i<8;i++) {
            if(pmBits & (1<<i)) {
              hAmpVsPMbits->Fill(i,amp);
              hTimeVsPMbits->Fill(i,time);
            }
            else {
              hAmpVsNotPMbits->Fill(i,amp);
              hTimeVsNotPMbits->Fill(i,time);
            }
          }
          arrChID_lastBC[chID]=bc;
          arrAmp[chID] = amp;
          arrTime[chID] = time;
          //
        }
        if(nChanA>0) meanTimeA = meanTimeA/nChanA;
        if(nChanC>0) meanTimeC = meanTimeC/nChanC;
        const double collTime = (meanTimeA + meanTimeC)/2 * sNSperTimeChannel;
        const double vrtPos = (meanTimeC - meanTimeA)/2 * sNSperTimeChannel * sNS2Cm;
        if(nChanA_noCut>0) meanTimeA_noCut = meanTimeA_noCut/nChanA_noCut;
        if(nChanC_noCut>0) meanTimeC_noCut = meanTimeC_noCut/nChanC_noCut;
        const double collTime_noCut = (meanTimeA_noCut + meanTimeC_noCut)/2 * sNSperTimeChannel;
        const double vrtPos_noCut = (meanTimeC_noCut - meanTimeA_noCut)/2 * sNSperTimeChannel * sNS2Cm;

        if(nChanA_noCut>0 && nChanC_noCut>0) {
          hVertexNoCut->Fill(vrtPos_noCut);
          hCollisionTimeVsVertex_noCut->Fill(vrtPos_noCut,collTime_noCut);
        }
        if(nChanA>0 && nChanC>0) {
          hCollisionTimeVsVertex->Fill(vrtPos,collTime);
          hCollisionTime->Fill(collTime);
          hVertex->Fill(vrtPos);

          if(trg.getVertex() && trg.getDataIsValid() && !trg.getOutputsAreBlocked()) {
            hCollisionTimeVsVertex_vrtTrg->Fill(vrtPos,collTime);
            hCollisionTime_vrtTrg->Fill(collTime);
            hVertex_vrtTrg->Fill(vrtPos);
            hVertexVsBC_vrtTrg->Fill(bc,vrtPos);
          }
          else {
            hCollisionTimeVsVertex_noVrtTrg->Fill(vrtPos,collTime);
            hCollisionTime_noVrtTrg->Fill(collTime);
            hVertex_noVrtTrg->Fill(vrtPos);
            hVertexVsBC_noVrtTrg->Fill(bc,vrtPos);
/*
            for(const auto &entry: vecPMbits) {
              for(int i=0;i<8;i++) {
                if(entry & (1<<i)) {
                  hVertexVsPMbits_noVrtTrg->Fill(vrtPos,i);
                }
              }
            }
*/
          }
        }
        if(trg.getVertex()) {
          hBC_trgVrt->Fill(bc);
          hNChanAvsC_vrtTrg->Fill(nChanA,nChanC);
          if(!isCollision) {
            hBC_VrtTrg_OutOfColl->Fill(bc);
          }
        }
        else{
          if(trg.getOrA()) {
            if(isCollision) {
              hBC_OrA_notVrt_collBC->Fill(bc);
            }
            else {
              hBC_OrA_notVrt_nonCollBC->Fill(bc);
            }
          }
          if(trg.getOrC()) {
            if(isCollision) {
              hBC_OrC_notVrt_collBC->Fill(bc);
            }
            else {
              hBC_OrC_notVrt_nonCollBC->Fill(bc);
            }
          }
        }
        hNChanA->Fill(nChanA);
        hNChanC->Fill(nChanC);
        hNChanAvsC->Fill(nChanA,nChanC);
        hNChannels->Fill(nChanA+nChanC);
      }
      /*
      **PUT HERE CODE FOR PROCESSING TF
      */

      //

    }
    delete treeInput;
    fileInput.Close();
  }
  const auto isDataWritten = utilities::AnalysisUtils::writeObjToFile(listOutput,filepathOutput);
  delete listOutput;
  std::cout<<std::endl;
}

std::bitset<sNBC> getCollBC(unsigned int runnum) {
  std::bitset<sNBC> collBC{};
  const std::string tsKey = "SOR";
  o2::ccdb::CcdbApi ccdb_api;
  ccdb_api.init(o2::base::NameConf::getCCDBServer());
  const auto headers = ccdb_api.retrieveHeaders("RCT/Info/RunInformation", std::map<std::string, std::string>(),runnum);
  uint64_t ts{};
  const auto &itTimestamp = headers.find(tsKey);
  if(itTimestamp!=headers.end()) {
    ts = std::stoll(itTimestamp->second);
  }
  else {
    return collBC;
  }
  std::map<std::string, std::string> mapMetadata;
  std::map<std::string, std::string> mapHeader;
  const auto *ptrGRPLHCIFData = ccdb_api.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF",mapMetadata,ts,&mapHeader);
  const auto &bunchFilling = ptrGRPLHCIFData->getBunchFilling();
  ptrGRPLHCIFData->print();
  collBC=bunchFilling.getBCPattern();
  return collBC;
}