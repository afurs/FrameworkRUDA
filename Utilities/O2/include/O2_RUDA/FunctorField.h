#ifndef FunctorField_H
#define FunctorField_H

#include <functional>
#include <vector>
#include <algorithm>

#include <TH1.h>
#include <TH2.h>
#include <TFitResult.h>
namespace functors
{
  std::function<unsigned int(unsigned int)>  getRunnum = [](unsigned int runnum) { return runnum; };
  std::function<double(const TH2F *, const TH2F *,int,int)> getRatioTrgs = [](const TH2F *h1,const TH2F *h2,int binPos1,int binPos2) {
    const double val1 = h1->Integral(1,3564, binPos1, binPos1);
    const double val2 = h2->Integral(1,3564, binPos2, binPos2);
    if(val2==0) {
      return val2;
    }
    return val1/val2;
  };

  std::function<std::vector<double>(const TH2 *, const TH2 *)> getVecOutCollBC = [](const TH2 *h1,const TH2 *h2) {
    std::vector<double> vecResult{};
    const std::vector<int> vecBinsToProcess {1,2,3,4,5,8};
    for(const auto &bin : vecBinsToProcess) {
      const double val1 = h1->Integral(1,3564, bin, bin);
      const double val2 = h2->Integral(1,3564, bin, bin);
      if(val2!=0) {
        vecResult.push_back(val1/val2);
      }
      else {
        vecResult.push_back(val2);
      }
    }
    return vecResult;
  };

  
  
  std::function<double(const TH2F *,int)> getCntFromBCdist = [](const TH2F *h1,int binPos1) {
    const double val1 = h1->Integral(1,3564, binPos1, binPos1);
    return val1;
  };

  std::function<std::vector<double>(const TH1 *)> getVecHistContent = [](const TH1 *hist) {
    std::vector<double> vecResult{};
    for(int iBin=1;iBin<hist->GetXaxis()->GetNbins()+1;iBin++) {
      vecResult.push_back(hist->GetBinContent(iBin));
    }
    return vecResult;
  };

  std::function<std::vector<double>(const TH1 *)> getVecTrgVals = [](const TH1 *hist) {
    std::vector<double> vecResult{};
    const std::vector<int> vecBinsToProcess {1,2,3,4,5,8};
    for(const auto &bin:vecBinsToProcess) {// OrA, OrC, Semicentral, Central, Vertex
      vecResult.push_back(hist->GetBinContent(bin));
    }
    return vecResult;
  };


  std::function<double(const TH1F *)> getMean = [](const TH1F *hist) {
    return static_cast<double>(hist->GetMean());
  };

  std::function<double(const TH1F *)> getGausPeak = [](const TH1F *hist) {
    return static_cast<double>(hist->GetMean());
  };

  std::function<double(const TH1F *)> getRMS = [](const TH1F *hist) {
    return static_cast<double>(hist->GetRMS());
  };


  std::function<double(const TH1F *)> getIntegral = [](const TH1F *hist) {
    return static_cast<double>(hist->Integral());
  };

  std::function<std::vector<double>(std::vector<TH1D *>)> getVecMean = [](std::vector<TH1D *> vecHists) {
    std::vector<double> vecOutput{};
    for (const auto & hist: vecHists) {
      vecOutput.push_back(hist->GetMean());
    }
    return vecOutput;
  };

  std::function<std::vector<double>(std::vector<TH1D *>)> getVecIntegral = [](std::vector<TH1D *> vecHists) {
    std::vector<double> vecOutput{};
    for (const auto & hist: vecHists) {
      vecOutput.push_back(hist->Integral());
    }
    return vecOutput;
  };


  std::function<std::vector<double>(std::vector<TH1D *>)> getVecGausPeak = [](std::vector<TH1D *> vecHists) {
    std::vector<double> vecOutput{};
    for (const auto & hist: vecHists) {
      if(hist->GetEntries()==0) {
        vecOutput.push_back(-3000);
        continue;
      }
      TFitResultPtr resultFit = hist->Fit("gaus", "0SQ", "", -150., 150.);
      if (((int)resultFit) == 0) {
        vecOutput.push_back(resultFit->Parameters()[1]);
      }
      else {
        std::cout<<"\nBAD FIT!\n";
        vecOutput.push_back(-4000);
      }
    }
    return vecOutput;
  };

  std::function<double(const TH2 *, const TH2 *)> getSpotFraction = [](const TH2 *histSpot, const TH2 *histVertex) {
    const double vrtTrgIntegral = histVertex->Integral();
    const int binX1 = histSpot->GetXaxis()->FindBin(-100.);
    const int binX2 = histSpot->GetXaxis()->FindBin(-60.);
    const int binY1 = histSpot->GetYaxis()->FindBin(-4.);
    const int binY2 = histSpot->GetYaxis()->FindBin(-2.);
    const double valSpot = histSpot->Integral(std::min(binX1,binX2),std::max(binX1,binX2),std::min(binY1,binY2),std::max(binY1,binY2));
    if(valSpot!=0) {
      return vrtTrgIntegral/valSpot;
    }
    else {
      return valSpot;
    }
  };
}// namespace functors


#endif