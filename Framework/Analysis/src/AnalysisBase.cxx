#include "AnalysisRUDA/AnalysisBase.h"



/*******************************************************************************************************************/

/*******************************************************************************************************************/

   // template<typename... U>
   // using Detect = helpers::common::Detect<U...>;
/*  template <typename, template <typename> class, typename, typename = void_t<>,typename = void>
  struct Detect : std::false_type {};

  template <typename T, template <typename> class MethodType, typename ReturnType>
  struct Detect<T, MethodType, ReturnType, void_t<MethodType<T>>
      ,typename std::enable_if<std::is_same<ReturnType,MethodType<T>>::value>::type> : std::true_type {};
*/    //Checks if analysis class has init() method
/*
    template <typename T>
    using AnalysisHelper::init_t = decltype(std::declval<T>().init());
    template <typename T>
    using AnalysisHelper::has_init = helpers::common::Detect<T, init_t, void>;
    //Checks if analysis class has initFile() method
    template <typename T>
    using AnalysisHelper::initFile_t = decltype(std::declval<T>().initFile());
    template <typename T>
    using AnalysisHelper::has_initFile = helpers::common::Detect<T, initFile_t, void>;
    //Checks if analysis class has checkTree(const TTree &)
    template <typename T>
    using AnalysisHelper::checkTree_t = decltype(std::declval<T>().checkTree(std::declval<const TTree &>()));
    template <typename T>
    using AnalysisHelper::has_checkTree = helpers::common::Detect<T,checkTree_t,bool >;
    //Checks if analysis class has updateFinish() method
    template <typename T>
    using AnalysisHelper::updateFinish_t = decltype(std::declval<T>().updateFinish());
    template <typename T>
    using AnalysisHelper::has_updateFinish = helpers::common::Detect<T, updateFinish_t, void>;
    //Checks if analysis class has updatePerTree() method
    template <typename T>
    using AnalysisHelper::updatePerTree_t = decltype(std::declval<T>().updatePerTree());
    template <typename T>
    using AnalysisHelper::has_updatePerTree = helpers::common::Detect<T, updatePerTree_t, void>;
    //Checks if there are initState method
    template <typename T>
    using AnalysisHelper::initState_t = decltype(std::declval<T>().initState());
    template <typename T>
    using AnalysisHelper::has_initState = helpers::common::Detect<T, initState_t, void>;
*/
/*
    template <typename T>
    using toString_t = decltype(std::declval<T>().toString(std::declval<int>()));

    template <typename T>
    using has_toString = detect<T, toString_t>;
*/
