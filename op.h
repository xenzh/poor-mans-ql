#pragma once

#include "error.h"

#include <type_traits>
#include <ostream>
#include <string_view>


// auto context = expression.context<Store, Get>();
//
// for (auto &var : context)
// {
//     var = getGetter(var.name());
// }
//
// auto result = expression(context);
//
// * context stores vector with calculation results.
// * Get interface:
//      * accept      : operator()(callback &&) const;
// * Store interface:
//      * construct   : explicit Store(T &&);
//      * move-assign : Stire &Store=(T &&);
//      * accept      : operator()(callback &&) const;
//      * name        : template<typename T> static name() -> std::string_view
//
// * Op describes a calculation step
// * Arg interface:
//      * get         : operator()(Id) -> Result<Get>;

namespace pmql::op {


using Id = size_t;


class Var
{
    friend std::ostream &operator<<(std::ostream &, const Var &);

    const Id d_id;
    const std::string_view d_name;

public:
    explicit Var(Id id, std::string_view name);

    Id id() const;

    std::string_view name() const;

    template<typename Arg, typename Store>
    Result<Store> operator()(Arg &&arg) const;
};


template<template <typename = void> typename Op>
class Unary
{
    template<template <typename> typename O>
    friend std::ostream &operator<<(std::ostream &, const Unary<O> &);

    const Op<> d_op;
    const Id d_arg;

public:
    explicit Unary(Id arg);

    template<typename Arg, typename Store>
    Result<Store> operator()(Arg &&arg) const;
};


template<template <typename = void> typename Op>
class Binary
{
    template<template <typename> typename O>
    friend std::ostream &operator<<(std::ostream &, const Binary<O> &);

    const Op<> d_op;
    const Id d_lhs;
    const Id d_rhs;

public:
    explicit Binary(Id lhs, Id rhs);

    template<typename Arg, typename Store>
    Result<Store> operator()(Arg &&arg) const;
};



template<template<typename = void> typename Op> struct Traits;

template<template<typename = void> typename Op>
constexpr std::string_view name()
{
    return Traits<Op>::name;
}


namespace detail {


template<typename... Args> struct With
{
    template<typename Op>
    using result = decltype(std::declval<Op>()(std::declval<const Args &>()...));

    template<template <typename = void> typename Op, typename Store, typename = void> struct Eval
    {
        using type = void;

        static Result<type> eval(const Op<> &op, const Args &...args)
        {
            return err::error<err::Kind::OP_INCOMPATIBLE_TYPES>(err::format(op), err::format(args...));
        }
    };

    template<template <typename = void> typename Op, typename Store> struct Eval<Op, Store, std::void_t<result<Op<>>>>
    {
        using type = result<Op<>>;

        static Result<type> eval(const Op<> &op, const Args &...args)
        {
            return op(args...);
        }
    };
};


template<typename Store, template <typename = void> typename Op, typename... Args>
Result<typename detail::With<Args...>::template result<>> eval(const Op<> &op, Args &&...args)
{
    return With<Args...>::template Eval<Op, Store>::eval(op, std::forward<Args>(args)...);
}


} // namespace detail


inline Var::Var(Id id, std::string_view name)
    : d_id(id)
    , d_name(name)
{
}

inline Id Var::id() const
{
    return d_id;
}

inline std::string_view Var::name() const
{
    return d_name;
}

inline std::ostream &operator<<(std::ostream &os, const Var &var)
{
    return os << var.d_name << "(#" << var.d_id << ")";
}


template<typename Arg, typename Store>
Result<Store> Var::operator()(Arg &&arg) const
{
    const auto &var = arg(d_id);
    if (!var.has_value())
    {
        return err::error<err::Kind::OP_BAD_ARGUMENT>(*this, d_id, var.error());
    }

    return (*var)([] (const auto &typed) -> Result<Store>
    {
        return typed;
    });
}


template<template <typename = void> typename Op>
Unary<Op>::Unary(Id arg)
    : d_arg(arg)
{
}

template<template <typename = void> typename Op>
template<typename Arg, typename Store>
Result<Store> Unary<Op>::operator()(Arg &&arg) const
{
    const auto &item = arg(d_arg);
    if (!item.has_value())
    {
        return err::error<err::Kind::OP_BAD_ARGUMENT>(*this, d_arg, item.error());
    }

    return (*item)([&op = d_op] (const auto &typed) -> Result<Store>
    {
        return detail::eval(op, typed);
    });
}

template<template <typename> typename O>
std::ostream &operator<<(std::ostream &os, const Unary<O> &unary)
{
    return os << name<O>() << "(#" << unary.d_arg << ")";
}


template<template <typename = void> typename Op>
Binary<Op>::Binary(Id lhs, Id rhs)
    : d_lhs(lhs)
    , d_rhs(rhs)
{
}

template<template <typename = void> typename Op>
template<typename Arg, typename Store>
Result<Store> Binary<Op>::operator()(Arg &&arg) const
{
    const auto &lhs = arg(d_lhs);
    if (!lhs.has_value())
    {
        return err::error<err::Kind::OP_BAD_ARGUMENT>(*this, d_lhs, lhs.error());
    }

    const auto &rhs = arg(d_rhs);
    if (!rhs.has_value())
    {
        return err::error<err::Kind::OP_BAD_ARGUMENT>(*this, d_rhs, rhs.error());
    }

    return (*lhs)([&op = d_op, &rhs] (const auto &ltyped)
    {
        return (*rhs)([&op, &ltyped] (const auto &rtyped) -> Result<Store>
        {
            return op(ltyped, rtyped);
        });
    });
}

template<template <typename> typename O>
std::ostream &operator<<(std::ostream &os, const Binary<O> &binary)
{
    return os << name<O>() << "(#" << binary.d_lhs << ", #" << binary.d_rhs << ")";
}


} // namespace pmql::op
