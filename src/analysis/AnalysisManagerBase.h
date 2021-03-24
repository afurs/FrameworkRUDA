#ifndef AnalysisManagerBase_h
#define AnalysisManagerBase_h
//#include <bitset>

#include "AnalysisBase.h"
#include "CutObjectManager.h"
#include "DataModel.h"

template<typename DataType,typename EventCutIDtype,typename DataOutputType>
class AnalysisManagerBase:public AnalysisBase<AnalysisManagerBase<DataType,EventCutIDtype,DataOutputType>>
{
 public:
  typedef DataInput<DataType,EventCutIDtype> Data_t;
  typedef EventCutIDtype EventCutID_t;
  typedef DataOutputType DataOutput_t;
  //typedef DataType Data_t;
  //typedef CutObjectManagerType CutObjectManager_t;
  typedef CutObjectManager<Data_t> CutObjectManager_t;
  typedef DataOutputManager<Data_t,DataOutputType> DataOutputManager_t;
  typedef typename DataOutputManager_t::DataOutput_t DataOutputObject_t;
  typedef AnalysisBase<AnalysisManagerBase<DataType,EventCutIDtype,DataOutputType>> AnalysisBase_t;
//  typedef CutType Cut_t;
  AnalysisManagerBase():mCutObjectManager(CutObjectManager_t(mData)),mDataOutputManager(DataOutputManager_t(mData)) {}
  ~AnalysisManagerBase() = default;
  /*******************************************************************************************************************/

  void initTree(TTree *treeInput) {
    mData.connectTree(treeInput);
  }
  /*******************************************************************************************************************/

  void processEvent()   {
    if constexpr(AnalysisHelper::has_initState<Data_t>::value)  mData.initState();
    mCutObjectManager.fillEventCutID();
    mDataOutputManager.fillData();
  }
  /*******************************************************************************************************************/

  void updateFinish() {
    for(auto& entry:mDataOutputManager.mVecDataOutput ) AnalysisBase_t::mListResult.Add(&(entry.mDataOutput));
    for(auto& entryCountID:mDataOutputManager.mMapEventCutIDToDataOutput) {

        for(auto& entry:entryCountID.second)  AnalysisBase_t::mListResult.Add(&(entry.mDataOutput));
        //entryCountID.second.fillData();

    }
  }
  Data_t mData;
  CutObjectManager_t mCutObjectManager;
  DataOutputManager_t mDataOutputManager;
};
template<typename DataInputType,typename DataOutputType>
using AnalysisManager = AnalysisManagerBase<typename DataInputType::Data_t,typename DataInputType::EventCutID_t,DataOutputType>;
#endif
