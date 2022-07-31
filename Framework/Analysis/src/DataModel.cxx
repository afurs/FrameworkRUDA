#include "AnalysisRUDA/DataModel.h"
/*******************************************************************************************************************/

/*******************************************************************************************************************/
//DataOutputManagerBase
/*******************************************************************************************************************/

/*******************************************************************************************************************
template<typename DataOutputType>
void DataOutputManagerBase<DataOutputType>::makeDataOutput(DataOutput_t dataOutput) {
  std::cout<<"\nMAKING DATA OUTPUT\n";
  dataOutput.print();
  if(dataOutput.mEventCutID==EventCutID_t{}) {
    mVecDataOutput.push_back(dataOutput);
  }
  else {
    auto [it, isNew] = mMapEventCutIDToDataOutput.insert({dataOutput.mEventCutID,std::vector<DataOutput_t>{}});
    it->second.push_back(dataOutput);
//      mMapEventCutIDToDataOutput.insert({dataOutput.mEventCutID,dataOutput});
  }
}
*******************************************************************************************************************/
