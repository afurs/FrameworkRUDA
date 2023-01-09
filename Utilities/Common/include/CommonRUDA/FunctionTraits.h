#ifndef FunctionTraits_h
#define FunctionTraits_h

#include "CommonRUDA/HelperCommon.h"

#include <type_traits>
#include <functional>
#include <tuple>
#include <vector>
#include <string>

namespace common {

  template<typename... T >
  struct FunctionTraits;

  template<typename R, typename ...Args>
  struct FunctionTraits<std::function<R(Args...)> >
  {
    typedef R Result_t;
    typedef std::tuple<Args ...> TupleArgs_t;
    typedef std::function<R(Args...)> Functor_t;
    
    typedef std::make_index_sequence<std::tuple_size_v<TupleArgs_t> > IndexesArgs_t;
    FunctionTraits(Functor_t functor):mFunctor(functor) {}
    template <std::size_t... IArgs>
    static Result_t evalBase(Functor_t functor, const TupleArgs_t &tupleArgs, std::index_sequence<IArgs...> ) {
      return functor(std::get<IArgs>(tupleArgs)...);
    }
    static Result_t eval(Functor_t functor, const TupleArgs_t &tupleArgs) {
      return evalBase(functor, tupleArgs,IndexesArgs_t {});
    }
    Result_t eval(const TupleArgs_t &tupleArgs) const {
      return eval(mFunctor, tupleArgs);
    }
    Functor_t mFunctor;
  };


  template<typename ...Functors>
  struct FunctionTraits<std::tuple<Functors ...> >
  {
    typedef std::tuple<Functors ...> TupleFunctors_t;
    typedef std::tuple<FunctionTraits<Functors> ...> TupleFunctorsWrapper_t;
    typedef std::tuple<typename FunctionTraits<Functors>::TupleArgs_t ...> TupleArgsFunc_t;

    typedef std::tuple<typename FunctionTraits<Functors>::Result_t ...> TupleResultsFunc_t;
    typedef std::tuple<std::make_index_sequence<std::tuple_size_v<typename FunctionTraits<Functors>::TupleArgs_t > > ... > TupleIndexesArgs;
    typedef std::make_index_sequence<std::tuple_size_v<TupleFunctors_t> > IndexesFunctors;
    FunctionTraits(std::tuple<Functors ...> tupleFunctors):mTupleFunctors(tupleFunctors) {}
    template <std::size_t... ITuple>
    static TupleResultsFunc_t evalBase(TupleFunctors_t tupleFunctors, const TupleArgsFunc_t &tupleArgsFuncs, std::index_sequence<ITuple...>) {
      return std::make_tuple(std::tuple_element_t<ITuple,TupleFunctorsWrapper_t>::eval(std::get<ITuple>(tupleFunctors),std::get<ITuple>(tupleArgsFuncs))...);
    }
    static TupleResultsFunc_t eval(TupleFunctors_t tupleFunctors, const TupleArgsFunc_t &tupleArgsFuncs) {
      return evalBase(tupleFunctors, tupleArgsFuncs, IndexesFunctors {});
    }
    TupleResultsFunc_t eval(const TupleArgsFunc_t &tupleArgsFuncs) const {
      return eval(mTupleFunctors, tupleArgsFuncs);
    }
    template<typename ArgType>
    static std::string argAsString(const ArgType &arg, const std::string &delimeter=";") {
      if constexpr(std::is_same<ArgType,std::string>::value) {
        return arg;
      }
      else if constexpr(helpers::common::IsSpecOf<std::vector, ArgType>::value) {
        if(arg.size()==1) {
          return std::to_string(arg[0]);
        }
        else if(arg.size()>1) {
          std::string str = std::to_string(arg[0]);
          for(int i=1;i<arg.size();i++) {
            str+=delimeter;
            str+=std::to_string(arg[i]);
          }
          return str;
        }
      }
      else {
        return std::to_string(arg);
      }
      return std::string{"nan"};
    }
    static std::vector<std::string> resultsAsVecOfStrs(const TupleResultsFunc_t &tupleResultsFunc, const std::string &delimeter=";") {
      std::vector<std::string> vecResult{};
      std::apply([&vecResult,&delimeter] (const auto &... args) {
        ((vecResult.push_back(argAsString(args,delimeter))),...);
      },
      tupleResultsFunc);
      return vecResult;
    }
    TupleFunctors_t mTupleFunctors;
  };

}// namespace common
#endif