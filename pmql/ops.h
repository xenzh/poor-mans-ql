#pragma once

#include "op.h"
#include "error.h"

#include <functional>
#include <string_view>
#include <variant>
#include <vector>


#define PmqlStdOpList \
    PmqlStdOp(std, plus         , + , 2) \
    PmqlStdOp(std, minus        , - , 2) \
    PmqlStdOp(std, multiplies   , * , 2) \
    PmqlStdOp(std, divides      , / , 2) \
    PmqlStdOp(std, modulus      , % , 2) \
    PmqlStdOp(std, negate       , - , 1) \
    PmqlStdOp(std, equal_to     , ==, 2) \
    PmqlStdOp(std, not_equal_to , !=, 2) \
    PmqlStdOp(std, greater      , > , 2) \
    PmqlStdOp(std, less         , < , 2) \
    PmqlStdOp(std, greater_equal, >=, 2) \
    PmqlStdOp(std, less_equal   , <=, 2) \
    PmqlStdOp(std, logical_and  , &&, 2) \
    PmqlStdOp(std, logical_or   , ||, 2) \
    PmqlStdOp(std, logical_not  , ! , 1) \


namespace pmql::op {
namespace detail {


template<typename T, typename = void> struct Ostreamable : std::false_type {};
template<typename T> struct Ostreamable<
    T,
    std::void_t<decltype(std::declval<std::ostream &>() << std::declval<const T &>())>> : std::true_type {};


template<template<typename> typename Fn, size_t MaxArity> struct ByArity;

template<template<typename> typename Fn> struct ByArity<Fn, 1> { using type = Unary<Fn> ; };
template<template<typename> typename Fn> struct ByArity<Fn, 2> { using type = Binary<Fn>; };


template<template<typename> typename Fn> struct AsOp { using type = typename Traits<Fn>::op; };


template<template<typename...> typename To, template<template<typename> typename> typename As, typename... Extra>
struct Expand
{
    template<template<typename...> typename T, template<typename> typename... Ts>
    using Expander = T<typename As<Ts>::type...>;

#define PmqlStdOp(Ns, Fn, Sign, Arity) , Ns::Fn
    using Expanded = Expander<To PmqlStdOpList>;
#undef PmqlStdOp

    template<typename T> struct Cat;
    template<typename... Args> struct Cat<To<Args...>>
    {
        template<typename... Ts> using With = To<Args..., Ts...>;
    };

    using type = typename Cat<Expanded>::template With<Extra...>;
};


template<template<typename...> typename To, template<template<typename> typename> typename As, typename... Extra>
using expand_t = typename Expand<To, As, Extra...>::type;


} // namespace detail


#define PmqlStdOp(Ns, Fn, Sign, Arity) \
template<> struct Traits<Ns::Fn> \
{ \
    static uintptr_t unique() { return reinterpret_cast<uintptr_t>(&unique); }; \
    static constexpr std::string_view name = #Fn; \
    static constexpr std::string_view sign = #Sign; \
    static constexpr size_t max_arity = Arity; \
    using fn = Ns::Fn<>; \
    using op = typename detail::ByArity<Ns::Fn, max_arity>::type; \
};
PmqlStdOpList
#undef PmqlStdOp


using Any = detail::expand_t<std::variant, detail::AsOp, Const, Var>;
using List = std::vector<Any>;


} // namespace pmql::op


#define PmqlStdOp(Ns, Fn, Sign, Arity) \
namespace Ns { \
inline std::ostream &operator<<(std::ostream &os, const Fn<void> &) \
{ \
    return os << #Fn; \
} \
}
PmqlStdOpList
#undef PmqlStdOp


namespace std {


inline std::ostream &operator<<(std::ostream &os, const pmql::op::Any &op)
{
    return std::visit(
        [&os] (const auto &item) mutable -> std::ostream & { return os << item; },
        op);
}


inline std::ostream &operator<<(std::ostream &os, const pmql::op::List &list)
{
    pmql::op::Id id = 0;
    for (const auto &op : list)
    {
        os << "\t#" << id++ << ": " << op << "\n";
    }

    return os;
}


} // namespace std


#undef PmqlStdOpList
