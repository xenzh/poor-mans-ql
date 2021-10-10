#pragma once

#include "error.h"

#include <limits>
#include <type_traits>
#include <ostream>
#include <string_view>


namespace pmql::op {


/// Operation identifier.
using Id = size_t;


/// Describes a typed constant value stored elsewhere.
class Const
{
    /// Streaming support.
    friend std::ostream &operator<<(std::ostream &, const Const &);

    /// Constant's index in an extenal storage.
    const Id d_sub;

public:
    /// Construct operation instance.
    /// @param id external storage index.
    explicit Const(Id id);

    /// Return external storage index.
    /// @return external storage index.
    Id id() const;

    /// Calls provided callback with external storage index.
    /// @tparam To callable: void(Id);
    /// @param to callback instance.
    template<typename To>
    void refers(To &&to) const;

    /// Fetches constant's value from extenal storage using provided predicate.
    /// @tparam Store type used to hold evaluation result.
    /// @tparam Arg callable: Result<Store>(Id);
    /// @param arg external storage getter.
    /// @return Storage-wrapped constant value or an error.
    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
};


/// Describes an untyped variable, that can be substituted by a typed value.
class Var : public Const
{
    /// Streaming support.
    friend std::ostream &operator<<(std::ostream &, const Var &);

    /// Unique variable name.
    const std::string_view d_name;

public:
    /// Construct operation instance.
    /// @param id external substitution storage index.
    /// @param name variable name.
    explicit Var(Id id, std::string_view name);

    /// Return variable name.
    /// @return variable name.
    std::string_view name() const;
};


/// Describes unary operation identified by provided std-like functional object.
/// @tparam Op std-like functional object (i.e. std::negate).
template<template <typename = void> typename Fn>
class Unary
{
    /// Streaming support.
    template<template <typename> typename O>
    friend std::ostream &operator<<(std::ostream &, const Unary<O> &);

    /// Operation's functional object.
    const Fn<> d_op;

    /// Reference to value to apply the operation to.
    const Id d_arg;

public:
    /// Construct operation instance.
    /// @param arg operation argument reference.
    explicit Unary(Id arg);

    /// Calls provided callback with operation argument reference.
    /// @tparam To callable: void(Id);
    /// @param to callback instance.
    template<typename To>
    void refers(To &&to) const;

    /// Fetches arguments, applies the operation and returns results.
    /// @tparam Store type used to hold evaluation result.
    /// @tparam Arg callable: Result<Store>(Id);
    /// @param arg operation argument getter.
    /// @return Storage-wrapped operation result or an error.
    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
};


/// Describes binary operation identified by provided std-like functional object.
/// @tparam Op std-like functional object (i.e. std::plus).
template<template <typename = void> typename Fn>
class Binary
{
    /// Streaming support.
    template<template <typename> typename O>
    friend std::ostream &operator<<(std::ostream &, const Binary<O> &);

    /// Operation's functional object.
    const Fn<> d_op;

    /// Reference to the first argument.
    const Id d_lhs;

    /// Reference to the second argument.
    const Id d_rhs;

public:
    /// Construct operation instance.
    /// @param lhs reference to the first argument.
    /// @param rhs reference to the second argument.
    explicit Binary(Id lhs, Id rhs);

    /// Calls provided callback with operation argument references.
    /// @tparam To callable: void(Id);
    /// @param to callback instance.
    template<typename To>
    void refers(To &&to) const;

    /// Fetches arguments, applies the operation and returns results.
    /// @tparam Store type used to hold evaluation result.
    /// @tparam Arg callable: Result<Store>(Id);
    /// @param arg operation argument getter.
    /// @return Storage-wrapped operation result or an error.
    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
};


/// Describes condition branching ("ternary operator").
/// Only condition and active branch are evaluated.
class Ternary
{
    /// Streaming support.
    friend std::ostream &operator<<(std::ostream &, const Ternary &);

    /// Reference to the condition. Value must be convertible to bool.
    const Id d_cond;

    /// Reference to the true branch value.
    const Id d_true;

    /// Reference to the false branch value.
    const Id d_false;

public:
    /// Construct operation instance.
    /// @param cond reference to the condition value. Must be convertible to bool.
    /// @param iftrue reference to the true branch value.
    /// @param iffalse reference to the false branch value.
    explicit Ternary(Id cond, Id iftrue, Id iffalse);

    /// Calls provided callback with operation argument references.
    /// @tparam To callable: void(Id);
    /// @param to callback instance.
    template<typename To>
    void refers(To &&to) const;

    /// Fetches arguments, applies the operation and returns results.
    /// @tparam Store type used to hold evaluation result.
    /// @tparam Arg callable: Result<Store>(Id);
    /// @param arg operation argument getter.
    /// @return Storage-wrapped operation result or an error.
    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
};


/// Describes an external function with any number of arguments.
class Extension
{
    /// Streaming support.
    friend std::ostream &operator<<(std::ostream &, const Extension &);

    /// Function name, not used as an identifier.
    std::string_view d_name;

    /// Unique function identifier.
    Id d_fun;

    /// References to function's arguments.
    std::vector<Id> d_args;

public:
    /// Construct operation instance.
    /// @param name function name.
    /// @param funid unique function identifier.
    /// @param args references to function's arguments.
    explicit Extension(std::string_view name, Id funid, std::vector<Id> &&args);

    /// Return function identifier.
    /// @return function identifier.
    Id fun() const;

    /// Return function name.
    /// @return function name.
    std::string_view name() const;

    /// Calls provided callback with operation argument references.
    /// @tparam To callable: void(Id);
    /// @param to callback instance.
    template<typename To>
    void refers(To &&to) const;

    /// Calls provided function with argument references, returns invocation result.
    /// @tparam Store type used to hold evaluation result.
    /// @tparam Fun callable, Result<Store>(iterator, iterator);
    /// @param fun function implementation.
    /// @return Storage-wrapped operation result or an error.
    template<typename Store, typename Fun>
    Result<Store> eval(Fun &&fun) const;
};


/// Main template that describes properties of functional objects that can be used as operations.
/// @tparam Fn std-like functional object.
template<template<typename = void> typename Fn> struct Traits;

/// Get human-readable functional object name.
/// @tparam Fn std-like functional object.
/// @return operation name.
template<template<typename = void> typename Fn>
constexpr std::string_view name()
{
    return Traits<Fn>::name;
}


/// Main template that describes properties of operation objects.
template<typename Op> struct OpTraits;

template<template<typename> typename Fn> struct OpTraits<Unary <Fn>> { using type = Traits<Fn>; };
template<template<typename> typename Fn> struct OpTraits<Binary<Fn>> { using type = Traits<Fn>; };

template<> struct OpTraits<Ternary>
{
    struct type
    {
        static uintptr_t unique() { return reinterpret_cast<uintptr_t>(&unique); };
        static constexpr std::string_view name = "?";
        static constexpr size_t max_arity = 3;
        using op = Ternary;
    };
};

template<> struct OpTraits<Extension>
{
    struct type
    {
        static uintptr_t unique() { return reinterpret_cast<uintptr_t>(&unique); };
        static constexpr std::string_view name = "fun";
        static constexpr size_t max_arity = std::numeric_limits<size_t>::max();
        using op = Extension;
    };
};


namespace detail {


/// Type-reconciling adapter for calling functional objects.
/// @tparam Args pack of types to be used when calling the functional object.
template<typename... Args> struct With
{
    /// SFINAE shortcut for checking if an object can be called with current argument types.
    template<template<typename> typename Fn, typename Store>
    using allowed = decltype(Store {std::declval<Fn<void>>()(std::declval<const Args &>()...)});

    /// Operation evaluator template, enabled for when arguments are not compatible with the operation.
    template<template <typename = void> typename Fn, typename Store, typename = void>
    struct Eval
    {
        static Result<Store> eval(const Fn<> &op, const Args &...args)
        {
            return error<err::Kind::OP_INCOMPATIBLE_TYPES>(err::format(op), err::format(args...));
        }
    };

    /// Operation evaluator template, specialization for when arguments are compatible with the operation.
    template<template <typename = void> typename Fn, typename Store>
    struct Eval<Fn, Store, std::void_t<allowed<Fn, Store>>>
    {
        static Result<Store> eval(const Fn<> &op, const Args &...args)
        {
            return Store {op(args...)};
        }
    };
};


/// Performs type-checking and applies an operation to arguments, if it is possible.
/// @tparam Store type used to hold evaluation result.
/// @tparam Fn std-like functional object.
/// @tparam Args argument value types.
/// @param op operation to apply.
/// @param args operation arguments.
/// @return Wrapped operation result or an error.
template<typename Store, template <typename = void> typename Fn, typename... Args>
Result<Store> eval(const Fn<> &op, Args &&...args)
{
    return With<Args...>::template Eval<Fn, Store>::eval(op, std::forward<Args>(args)...);
}


} // namespace detail


/// Hash combining helper.
/// @see boost::hash_combine
/// @tparam Ts pack of types to hash and combine.
/// @param seed hashing seed.
/// @param value values to hash and combine.
/// @return combined value hash, can be used as the seed.
template <typename... Ts>
size_t hash(size_t seed, const Ts &...values)
{
    return seed ^= ((std::hash<Ts>{}(values) + 0x9e3779b9 + (seed << 6) + (seed >> 2)) ^ ...);
}


inline Const::Const(Id id)
    : d_sub(id)
{
}

inline Id Const::id() const
{
    return d_sub;
}

template<typename To>
void Const::refers(To &&to) const
{
    to(d_sub);
}

template<typename Store, typename Arg>
Result<Store> Const::eval(Arg &&arg) const
{
    const auto &var = arg(d_sub);
    if (!var.has_value())
    {
        return error<err::Kind::OP_BAD_ARGUMENT>(*this, d_sub, var.error());
    }

    return (*var)([] (const auto &typed) -> Result<Store>
    {
        return Store {typed};
    });
}

inline std::ostream &operator<<(std::ostream &os, const Const &var)
{
    return os << "const(_" << var.d_sub << ")";
}


inline Var::Var(Id id, std::string_view name)
    : Const(id)
    , d_name(name)
{
}

inline std::string_view Var::name() const
{
    return d_name;
}

inline std::ostream &operator<<(std::ostream &os, const Var &var)
{
    return os << var.d_name << "($" << var.id() << ")";
}


template<template <typename = void> typename Op>
Unary<Op>::Unary(Id arg)
    : d_op {}
    , d_arg {arg}
{
}

template<template <typename = void> typename Op>
template<typename To>
void Unary<Op>::refers(To &&to) const
{
    to(d_arg);
}

template<template <typename = void> typename Op>
template<typename Store, typename Arg>
Result<Store> Unary<Op>::eval(Arg &&arg) const
{
    const auto &item = arg(d_arg);
    if (!item.has_value())
    {
        return error<err::Kind::OP_BAD_ARGUMENT>(*this, d_arg, item.error());
    }

    const auto &value = *item;
    return value([&op = d_op] (const auto &typed) -> Result<Store>
    {
        return detail::eval<Store>(op, typed);
    });
}

template<template <typename> typename O>
std::ostream &operator<<(std::ostream &os, const Unary<O> &unary)
{
    return os << name<O>() << "(#" << unary.d_arg << ")";
}


template<template <typename = void> typename Op>
Binary<Op>::Binary(Id lhs, Id rhs)
    : d_op {}
    , d_lhs {lhs}
    , d_rhs {rhs}
{
}

template<template <typename = void> typename Op>
template<typename To>
void Binary<Op>::refers(To &&to) const
{
    to(d_lhs);
    to(d_rhs);
}

template<template <typename = void> typename Op>
template<typename Store, typename Arg>
Result<Store> Binary<Op>::eval(Arg &&arg) const
{
    const auto &lhs = arg(d_lhs);
    if (!lhs.has_value())
    {
        return error<err::Kind::OP_BAD_ARGUMENT>(*this, d_lhs, lhs.error());
    }

    const auto &rhs = arg(d_rhs);
    if (!rhs.has_value())
    {
        return error<err::Kind::OP_BAD_ARGUMENT>(*this, d_rhs, rhs.error());
    }

    return (*lhs)([&op = d_op, &rhs] (const auto &ltyped)
    {
        return (*rhs)([&op, &ltyped] (const auto &rtyped) -> Result<Store>
        {
            return detail::eval<Store>(op, ltyped, rtyped);
        });
    });
}

template<template <typename> typename O>
std::ostream &operator<<(std::ostream &os, const Binary<O> &binary)
{
    return os << name<O>() << "(#" << binary.d_lhs << ", #" << binary.d_rhs << ")";
}


inline Ternary::Ternary(Id cond, Id iftrue, Id iffalse)
    : d_cond(cond)
    , d_true(iftrue)
    , d_false(iffalse)
{
}

template<typename To>
void Ternary::refers(To &&to) const
{
    to(d_cond);
    to(d_true);
    to(d_false);
}

template<typename Store, typename Arg>
Result<Store> Ternary::eval(Arg &&arg) const
{
    const auto &cond = arg(d_cond);
    if (!cond)
    {
        return error<err::Kind::OP_BAD_ARGUMENT>(*this, d_cond, cond.error());
    }

    auto result = (*cond)([this] (const auto &value) -> Result<bool>
    {
        if constexpr (std::is_convertible_v<decltype(value), bool>)
        {
            return value;
        }
        else
        {
            return error<err::Kind::OP_TERNARY_BAD_CONDITION>(*this, value);
        }
    });

    if (!result)
    {
        return error(std::move(result).error());
    }

    return result ?
        arg(*result ? d_true : d_false) :
        error(std::move(result).error());
}

inline std::ostream &operator<<(std::ostream &os, const Ternary &ternary)
{
    return os << "if(#" << ternary.d_cond << " ? #" << ternary.d_true << " : #" << ternary.d_false << ")";
}


inline Extension::Extension(std::string_view name, Id funid, std::vector<Id> &&args)
    : d_name(name)
    , d_fun(funid)
    , d_args(std::move(args))
{
}

inline Id Extension::fun() const
{
    return d_fun;
}

inline std::string_view Extension::name() const
{
    return d_name;
}

template<typename To>
void Extension::refers(To &&to) const
{
    for (const auto id : d_args)
    {
        to(id);
    }
}

template<typename Store, typename Fun>
Result<Store> Extension::eval(Fun &&fun) const
{
    return fun(d_fun, d_args.begin(), d_args.end());
}

inline std::ostream &operator<<(std::ostream &os, const Extension &ext)
{
    os << "@" << ext.d_name << "(";
    for (auto it = ext.d_args.begin(); it != ext.d_args.end(); ++it)
    {
        os << "#" << *it << (std::next(it) == ext.d_args.end() ? "" : ", ");
    }
    return os << ")";
}


} // namespace pmql::op


namespace std {


template<> struct hash<pmql::op::Const>
{
    size_t operator()(const pmql::op::Const &var) const noexcept
    {
        return var.id();
    }
};

template<> struct hash<pmql::op::Var>
{
    size_t operator()(const pmql::op::Var &var) const noexcept
    {
        return pmql::op::hash(0, var.id(), var.name());
    }
};


template<template <typename = void> typename Op> struct hash<pmql::op::Unary<Op>>
{
    size_t operator()(const pmql::op::Unary<Op> &unary) const noexcept
    {
        size_t seed = pmql::op::hash(0, pmql::op::Traits<Op>::unique());
        unary.refers([&seed] (auto ref) mutable { seed = pmql::op::hash(seed, ref); });
        return seed;
    }
};

template<template <typename = void> typename Op> struct hash<pmql::op::Binary<Op>>
{
    size_t operator()(const pmql::op::Binary<Op> &binary) const noexcept
    {
        size_t seed = pmql::op::hash(0, pmql::op::Traits<Op>::unique());
        binary.refers([&seed] (auto ref) mutable { seed = pmql::op::hash(seed, ref); });
        return seed;
    }
};

template<> struct hash<pmql::op::Ternary>
{
    size_t operator()(const pmql::op::Ternary &ternary) const noexcept
    {
        size_t seed = pmql::op::OpTraits<pmql::op::Ternary>::type::unique();
        ternary.refers([&seed] (auto ref) mutable { seed = pmql::op::hash(seed, ref); });
        return seed;
    }
};

template<> struct hash<pmql::op::Extension>
{
    size_t operator()(const pmql::op::Extension &ext) const noexcept
    {
        size_t seed = pmql::op::OpTraits<pmql::op::Extension>::type::unique();
        seed = pmql::op::hash(seed, ext.fun());
        ext.refers([&seed] (auto ref) mutable { seed = pmql::op::hash(seed, ref); });
        return seed;
    }
};


} // namespace std
