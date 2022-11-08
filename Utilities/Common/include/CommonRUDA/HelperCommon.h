#ifndef HelperCommon_h
#define HelperCommon_h
#include <type_traits>

namespace helpers {
namespace common {
  template <typename, template <typename> class, typename, typename = std::void_t<>,typename = void>
  struct Detect : std::false_type {};

  template <typename T, template <typename> class MethodType, typename ReturnType>
  struct Detect<T, MethodType, ReturnType, std::void_t<MethodType<T>>
      ,typename std::enable_if<std::is_same<ReturnType,MethodType<T>>::value>::type> : std::true_type {};
  /*
   * Example:
   * Checks if class Sample has "bool Sample::checkMethod(const Type_t &)"
   *
   * template <typename T>
   * using checkMethod_t = decltype(std::declval<T>().checkMethod(std::declval<const Type_t &>()));
   *
   * template <typename T>
   * using has_checkMethod = Detect<T,checkMethod_t,bool >;
   *
   * if constexpr(helpers::has_checkMethod<Sample>::value) {}
  */

  template < template <typename...> class Template, typename T >
  struct IsSpecOf : std::false_type {};

  template < template <typename...> class Template, typename... Args >
  struct IsSpecOf< Template, Template<Args...> > : std::true_type {};
  /*
   * Example:
   *
   * template<typename Hist_t
   * , typename Map_t
   * , typename = typename std::enable_if<IsSpecOf<std::tuple,typename Map_t::mapped_type>::value>::type
   * auto makeHistFromMap(const Map_t &mapValues)->Hist_t* {return new Hist_t();}
  */

} //namespace helpers
} //namespace common
#endif
