#ifndef AnalysisManagerBase_h
#define AnalysisManagerBase_h
//#include <bitset>

#include "AnalysisRUDA/AnalysisBase.h"
#include "AnalysisRUDA/CutObjectManager.h"
#include "AnalysisRUDA/DataModel.h"
#include <tuple>
template<typename DataType,typename EventCutIDtype,typename DataOutputType,typename... DataOutputTypes>
class AnalysisManagerBase:public AnalysisBase<AnalysisManagerBase<DataType,EventCutIDtype,DataOutputType,DataOutputTypes...>>
{
 public:
  typedef DataInput<DataType,EventCutIDtype> Data_t;
  typedef EventCutIDtype EventCutID_t;
  typedef DataOutputType DataOutput_t;
  typedef std::tuple<DataOutputManager<Data_t,DataOutputTypes>...> DataOutputTuple_t;
  //typedef DataType Data_t;
  //typedef CutObjectManagerType CutObjectManager_t;
  typedef CutObjectManager<Data_t> CutObjectManager_t;
  typedef DataOutputManager<Data_t,DataOutputType> DataOutputManager_t;
  typedef typename DataOutputManager_t::DataOutput_t DataOutputObject_t;
  typedef AnalysisBase<AnalysisManagerBase<DataType,EventCutIDtype,DataOutputType,DataOutputTypes...>> AnalysisBase_t;
//  typedef CutType Cut_t;
  AnalysisManagerBase():mCutObjectManager(CutObjectManager_t(mData)),mDataOutputManager(DataOutputManager_t(mData))
  ,mDataOutputTuple(DataOutputTuple_t(DataOutputManager<Data_t,DataOutputTypes>(mData)...)) {}
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
    std::apply([&](auto&... outputManager) {
                 ((outputManager.fillData()), ...);
                 }, mDataOutputTuple);
  }
  /*******************************************************************************************************************/
  template<typename T>
  void updateFinish(T &manager) {
    for(auto& entry:manager.mVecDataOutput ) {
      AnalysisBase_t::mListResult.Add(&(entry.mDataOutput));
//      std::cout<<"\n=================\n";
//      entry.mDataOutput.Print();
//      std::cout<<"\n=================\n";
    }
    for(auto& entryCountID:manager.mMapEventCutIDToDataOutput) {
        for(auto& entry:entryCountID.second)  {
          AnalysisBase_t::mListResult.Add(&(entry.mDataOutput));
//          std::cout<<"\n-------------\n";
//          entry.mDataOutput.Print();
//          std::cout<<"\n-------------\n";
        }
    }
  }

  void updateFinish() {
    updateFinish(mDataOutputManager);
    /*
    for(auto& entry:mDataOutputManager.mVecDataOutput ) AnalysisBase_t::mListResult.Add(&(entry.mDataOutput));
    for(auto& entryCountID:mDataOutputManager.mMapEventCutIDToDataOutput) {
        for(auto& entry:entryCountID.second)  AnalysisBase_t::mListResult.Add(&(entry.mDataOutput));
        //entryCountID.second.fillData();
    }
    */
    std::apply([&](auto&... outputManager) {
                 ((updateFinish(outputManager)), ...);
                 }, mDataOutputTuple);

  }
  Data_t mData;
  CutObjectManager_t mCutObjectManager;
  DataOutputManager_t mDataOutputManager;
  DataOutputTuple_t mDataOutputTuple;
};
template<typename DataInputType,typename DataOutputType>
using AnalysisManager = AnalysisManagerBase<typename DataInputType::Data_t,typename DataInputType::EventCutID_t,DataOutputType>;
#endif
