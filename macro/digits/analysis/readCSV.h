#include <TTree.h>
#include <map>
#include <array>
#include <string>
#include <iostream>
struct Entry{
  unsigned int runnum;
  std::array<double, 208> mPeak;
  std::array<double, 208> mMean;
  std::array<double, 208> mIntegral;
  std::map<unsigned int, std::array<double, 208>> mMapChunk2Offset{}; 
  bool checkChID(int chID) const {
    if(chID>207) {
      return false;
    }
    else if(mPeak[chID]<-150||mPeak[chID]>150) {
      return false;
    }
    return true;
  }
  void print() const {
    std::cout<<"\n========================"<<runnum<<"==========================\n";
    for(int i=0;i<208;i++) std::cout<<mPeak[i]<<" ";
    std::cout<<"\n========================================================\n";
    for(int i=0;i<208;i++) std::cout<<mMean[i]<<" ";
    std::cout<<"\n========================================================\n";
    for(int i=0;i<208;i++) std::cout<<mIntegral[i]<<" ";
    std::cout<<"\n========================================================\n";

  }
};


std::map<unsigned int,Entry> getMapCSV(const std::string &filename) {
  std::map<unsigned int,Entry> mapResult{};
  unsigned int runnum;
  Entry entry{};
  TTree treeCSV;
  std::string header="runnum/i";
  for(int i=0;i<208;i++) {
    header+=std::string{":peak"+std::to_string(i)+"/D"};
  }
  for(int i=0;i<208;i++) {
    header+=std::string{":mean"+std::to_string(i)+"/D"};
  }
  for(int i=0;i<208;i++) {
    header+=std::string{":int"+std::to_string(i)+"/D"};
  }
  treeCSV.ReadFile(filename.c_str(), header.c_str(),';');
  std::cout<<"\n N branches: "<<treeCSV.GetEntries()<<std::endl;
  if(treeCSV.GetEntries()==0) return mapResult;
  treeCSV.SetBranchAddress("runnum",&entry.runnum);
  for(int iCh=0;iCh<208;iCh++) {
    treeCSV.SetBranchAddress(Form("peak%i",iCh),&entry.mPeak[iCh]);
    treeCSV.SetBranchAddress(Form("mean%i",iCh),&entry.mMean[iCh]);
    treeCSV.SetBranchAddress(Form("int%i",iCh),&entry.mIntegral[iCh]);
  }
  for(int iEntry=0;iEntry<treeCSV.GetEntries();iEntry++) {
    treeCSV.GetEntry(iEntry);
    mapResult.insert({entry.runnum,entry});
  }
  return mapResult;
}

std::map<unsigned int,Entry> getMapCSV_2(const std::string &filename) {
  std::map<unsigned int,Entry> mapResult{};
  unsigned int chunk;
  Entry entry{};
  TTree treeCSV;
  //runnum;chunk;tsStart;tsEnd;ch0;ch1;ch2;ch3;ch4;ch5;ch6;ch7;ch8;ch9;ch10;ch11;ch12;ch13;ch14;ch15;ch16;ch17;ch18;ch19;ch20;ch21;ch22;ch23;ch24;ch25;ch26;ch27;ch28;ch29;ch30;ch31;ch32;ch33;ch34;ch35;ch36;ch37;ch38;ch39;ch40;ch41;ch42;ch43;ch44;ch45;ch46;ch47;ch48;ch49;ch50;ch51;ch52;ch53;ch54;ch55;ch56;ch57;ch58;ch59;ch60;ch61;ch62;ch63;ch64;ch65;ch66;ch67;ch68;ch69;ch70;ch71;ch72;ch73;ch74;ch75;ch76;ch77;ch78;ch79;ch80;ch81;ch82;ch83;ch84;ch85;ch86;ch87;ch88;ch89;ch90;ch91;ch92;ch93;ch94;ch95;ch96;ch97;ch98;ch99;ch100;ch101;ch102;ch103;ch104;ch105;ch106;ch107;ch108;ch109;ch110;ch111;ch112;ch113;ch114;ch115;ch116;ch117;ch118;ch119;ch120;ch121;ch122;ch123;ch124;ch125;ch126;ch127;ch128;ch129;ch130;ch131;ch132;ch133;ch134;ch135;ch136;ch137;ch138;ch139;ch140;ch141;ch142;ch143;ch144;ch145;ch146;ch147;ch148;ch149;ch150;ch151;ch152;ch153;ch154;ch155;ch156;ch157;ch158;ch159;ch160;ch161;ch162;ch163;ch164;ch165;ch166;ch167;ch168;ch169;ch170;ch171;ch172;ch173;ch174;ch175;ch176;ch177;ch178;ch179;ch180;ch181;ch182;ch183;ch184;ch185;ch186;ch187;ch188;ch189;ch190;ch191;ch192;ch193;ch194;ch195;ch196;ch197;ch198;ch199;ch200;ch201;ch202;ch203;ch204;ch205;ch206;ch207;meanTimeA;meanTimeC;collisionTime;vertexTime;LHCphase_TOF
  std::string header="runnum/i:chunk/i:tsStart/l:tsEnd/l";
  for(int i=0;i<208;i++) {
    header+=std::string{":ch"+std::to_string(i)+"/D"};
  }
  header+=std::string{":meanTimeA/D:meanTimeC/D:collisionTime/D:vertexTime/D:LHCphase_TOF/D"};
  treeCSV.ReadFile(filename.c_str(), header.c_str(),';');
  std::cout<<"\n N branches: "<<treeCSV.GetEntries()<<std::endl;
  if(treeCSV.GetEntries()==0) return mapResult;
  treeCSV.SetBranchAddress("chunk",&chunk);
  treeCSV.SetBranchAddress("runnum",&entry.runnum);

  for(int iCh=0;iCh<208;iCh++) {
    treeCSV.SetBranchAddress(Form("ch%i",iCh),&entry.mPeak[iCh]);
  }
  for(int iEntry=0;iEntry<treeCSV.GetEntries();iEntry++) {
    treeCSV.GetEntry(iEntry);
    auto it = mapResult.insert({entry.runnum,entry});
    it.first->second.mMapChunk2Offset.insert({chunk,entry.mPeak});
  }
  return mapResult;
}
std::map<unsigned int,Entry> getMapCSV_3(const std::string &filename) {
  std::map<unsigned int,Entry> mapResult{};
  unsigned int runnum;
  Entry entry{};
  TTree treeCSV;
  std::string header="runnum/i:chunk/i";
  for(int i=0;i<208;i++) {
    header+=std::string{":peak"+std::to_string(i)+"/D"};
  }
  for(int i=0;i<208;i++) {
    header+=std::string{":mean"+std::to_string(i)+"/D"};
  }
  for(int i=0;i<208;i++) {
    header+=std::string{":int"+std::to_string(i)+"/D"};
  }
  treeCSV.ReadFile(filename.c_str(), header.c_str(),';');
  std::cout<<"\n N branches: "<<treeCSV.GetEntries()<<std::endl;
  if(treeCSV.GetEntries()==0) return mapResult;
  unsigned int chunk;
  treeCSV.SetBranchAddress("chunk",&chunk);
  treeCSV.SetBranchAddress("runnum",&entry.runnum);
  for(int iCh=0;iCh<208;iCh++) {
    treeCSV.SetBranchAddress(Form("peak%i",iCh),&entry.mPeak[iCh]);
    treeCSV.SetBranchAddress(Form("mean%i",iCh),&entry.mMean[iCh]);
    treeCSV.SetBranchAddress(Form("int%i",iCh),&entry.mIntegral[iCh]);
  }
  for(int iEntry=0;iEntry<treeCSV.GetEntries();iEntry++) {
    treeCSV.GetEntry(iEntry);
    auto it = mapResult.insert({entry.runnum,entry});
    it.first->second.mMapChunk2Offset.insert({chunk,entry.mPeak});

  }
  return mapResult;
}
