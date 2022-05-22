#pragma once

#include "op.h"
#include "error.h"

#include <functional>
#include <string_view>
#include <variant>
#include <optional>
#include <vector>
#include <unordered_set>


/// Macro list of supported operations
/// PmqlStdOp(Namespace, Class, Sign, Arity)
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
    PmqlStdOp(std, bit_and      , & , 2) \
    PmqlStdOp(std, bit_or       , | , 2) \
    PmqlStdOp(std, bit_xor      , ^ , 2) \
    PmqlStdOp(std, bit_not      , ~ , 1) \


namespace pmql::op {
namespace detail {


/// Template that checks if a type can be written to an output stream.
template<typename T, typename = void> struct Ostreamable : std::false_type {};
template<typename T> struct Ostreamable<
    T,
    std::void_t<decltype(std::declval<std::ostream &>() << std::declval<const T &>())>> : std::true_type {};


/// Template that defines a wrapper type for an operation, according to its arity.
template<template<typename> typename Fn, size_t MaxArity> struct ByArity;

template<template<typename> typename Fn> struct ByArity<Fn, 1> { using type = Unary<Fn> ; };
template<template<typename> typename Fn> struct ByArity<Fn, 2> { using type = Binary<Fn>; };


/// Adaptor template that defines a wrapper type for an operation.
template<template<typename> typename Fn> struct AsOp { using type = typename Traits<Fn>::op; };


/// Helper template used to transform and specify provided template with builtin operation types along
/// with a list of optional extra types, added as is.
template<template<typename...> typename To, template<template<typename> typename> typename As, typename... Extra>
struct Expand
{
    /// Transforms and specializes template T with types from Ts... pack.
    /// It is used to work around the leading comma propblem in template argument list when doing xmacro expansion.
    template<template<typename...> typename T, template<typename> typename... Ts>
    using Expander = T<typename As<Ts>::type...>;

    /// Transforms and specializes To template with allowed operation types.
#define PmqlStdOp(Ns, Fn, Sign, Arity) , Ns::Fn
    using Expanded = Expander<To PmqlStdOpList>;
#undef PmqlStdOp

    /// Specializes To template with the concatenation of two variadic packs.
    template<typename T> struct Cat;
    template<typename... Args> struct Cat<To<Args...>>
    {
        template<typename... Ts> using With = To<Args..., Ts...>;
    };

    /// Specializes To template with transformed operation types and a list of extra types.
    using type = typename Cat<Expanded>::template With<Extra...>;
};


/// Helper alias for operator type expansion into provided template.
/// @see Expand
template<template<typename...> typename To, template<template<typename> typename> typename As, typename... Extra>
using expand_t = typename Expand<To, As, Extra...>::type;


} // namespace detail


/// Generate Traits specialization for all supported operation types.
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


/// Collection of all builtin operation signs.
#define PmqlStdOp(Ns, Fn, Sign, Arity) Traits<Ns::Fn>::sign,
inline const std::unordered_set<std::string_view> Signs { PmqlStdOpList };
#undef PmqlStdOp


/// Invokes a callback with with default-constructed Traits object for operation type matching provided sign.
/// If the sign is not recognized, the callback is invoked with std::nullopt_t instance.
/// @tparam Callback callable, T(Traits<Fn>) or T(std::nullopt);
/// @param source operation sign.
/// @param pos position to start looking for operation sign from. Modified to past-the-end of the sign if found.
/// @param callback callback used to pass constructed operation object to.
/// @return whatever the callback returns.
template<typename Callback>
auto identify(std::string_view source, size_t &pos, Callback &&callback) -> decltype(auto)
{
    size_t signmax = 0;
#define PmqlStdOp(Ns, Name, Sign, Arity) \
    signmax = std::max(signmax, Traits<Ns::Name>::sign.size());
    PmqlStdOpList
#undef PmqlStdOp

    if (pos < source.size())
    {
        source = source.substr(pos);
        source.remove_prefix(std::min(source.find_first_not_of(" "), source.size()));
        source = source.substr(0, signmax);
    }

#define PmqlStdOp(Ns, Name, Sign, Arity) \
    if (auto start = source.find(#Sign); start != std::string_view::npos) \
    { \
        pos = start + Traits<Ns::Name>::sign.size(); \
        return callback(Traits<Ns::Name> {}); \
    }
    if (false) {} PmqlStdOpList
#undef PmqlStdOp

    return callback(std::nullopt);
}


/// Defines variant that can hold any operation descriptor.
using Any = detail::expand_t<std::variant, detail::AsOp, Const, Var, Ternary, Extension>;

/// Defines an ordered list of operations.
using List = std::vector<Any>;


} // namespace pmql::op


/// Generates stream operator implementations for supported operations.
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


/// Stream operator for operation variant type.
inline std::ostream &operator<<(std::ostream &os, const pmql::op::Any &op)
{
    return std::visit(
        [&os] (const auto &item) mutable -> std::ostream & { return os << item; },
        op);
}


/// Stream operator for operation list.
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


//#undef PmqlStdOpList
