#pragma once

#include "op.h"
#include "error.h"

#include <functional>
#include <string_view>
#include <variant>
#include <vector>


namespace pmql::op {


#define PmqlStdOpList \
    PmqlStdOp(plus         , + , 2) \
    PmqlStdOp(minus        , - , 2) \
    PmqlStdOp(multiplies   , * , 2) \
    PmqlStdOp(divides      , / , 2) \
    PmqlStdOp(modulus      , % , 2) \
    PmqlStdOp(negate       , - , 1) \
    PmqlStdOp(equal_to     , ==, 2) \
    PmqlStdOp(not_equal_to , !=, 2) \
    PmqlStdOp(greater      , > , 2) \
    PmqlStdOp(less         , < , 2) \
    PmqlStdOp(greater_equal, >=, 2) \
    PmqlStdOp(less_equal   , <=, 2) \
    PmqlStdOp(logical_and  , &&, 2) \
    PmqlStdOp(logical_or   , ||, 2) \
    PmqlStdOp(logical_not  , ! , 1) \


namespace detail {


template<template<typename> typename Fn, size_t MaxArity> struct ByArity;

template<template<typename> typename Fn> struct ByArity<Fn, 1> { using type = Unary<Fn> ; };
template<template<typename> typename Fn> struct ByArity<Fn, 2> { using type = Binary<Fn>; };


template<template<typename> typename Fn> struct AsOp { using type = typename Traits<Fn>::op; };


template<template<typename...> typename To, template<template<typename> typename> typename As, typename... Extra>
struct Expand
{
    template<template<typename...> typename T, template<typename> typename... Ts>
    using Expander = T<typename As<Ts>::type...>;

#define PmqlStdOp(Fn, Sign, Arity) , std::Fn
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


#define PmqlStdOp(Fn, Sign, Arity) \
template<> struct Traits<std::Fn> \
{ \
    static constexpr std::string_view name = #Fn; \
    static constexpr std::string_view sign = #Sign; \
    static constexpr size_t max_arity = Arity; \
    using fn = std::Fn<>; \
    using op = typename detail::ByArity<std::Fn, max_arity>::type; \
};
PmqlStdOpList
#undef PmqlStdOp


using Any = detail::expand_t<std::variant, detail::AsOp, Var>;
using List = std::vector<Any>;


#undef PmqlStdOpList


} // namespace pmql::op


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
        os << "#" << id++ << ": " << op << "\n";
    }

    return os;
}


} // namespace std
