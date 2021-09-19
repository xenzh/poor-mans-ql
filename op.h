#pragma once

#include "error.h"

#include <type_traits>
#include <ostream>
#include <string_view>


namespace pmql::op {


using Id = size_t;


class Const
{
    friend std::ostream &operator<<(std::ostream &, const Const &);

    const Id d_sub;

public:
    explicit Const(Id id);

    Id id() const;

    template<typename To>
    void refers(To &&to) const;

    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
};


class Var : public Const
{
    friend std::ostream &operator<<(std::ostream &, const Var &);

    const std::string_view d_name;

public:
    explicit Var(Id id, std::string_view name);

    std::string_view name() const;
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

    template<typename To>
    void refers(To &&to) const;

    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
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

    template<typename To>
    void refers(To &&to) const;

    template<typename Store, typename Arg>
    Result<Store> eval(Arg &&arg) const;
};


template<template<typename = void> typename Fn> struct Traits;

template<template<typename = void> typename Fn>
constexpr std::string_view name()
{
    return Traits<Fn>::name;
}


template<typename Op> struct OpTraits;

template<template<typename> typename Fn> struct OpTraits<Unary <Fn>> { using type = Traits<Fn>; };
template<template<typename> typename Fn> struct OpTraits<Binary<Fn>> { using type = Traits<Fn>; };


namespace detail {


template<typename... Args> struct With
{
    template<typename Op>
    using result = decltype(std::declval<Op>()(std::declval<const Args &>()...));

    template<template <typename = void> typename Op, typename Store, typename = void> struct Eval
    {
        using type = void;

        static Result<Store> eval(const Op<> &op, const Args &...args)
        {
            return err::error<err::Kind::OP_INCOMPATIBLE_TYPES>(err::format(op), err::format(args...));
        }
    };

    template<template <typename = void> typename Op, typename Store> struct Eval<Op, Store, std::void_t<result<Op<>>>>
    {
        using type = result<Op<>>;

        static Result<Store> eval(const Op<> &op, const Args &...args)
        {
            return Store {op(args...)};
        }
    };
};


template<typename Store, template <typename = void> typename Op, typename... Args>
Result<Store> eval(const Op<> &op, Args &&...args)
{
    return With<Args...>::template Eval<Op, Store>::eval(op, std::forward<Args>(args)...);
}


} // namespace detail


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
        return err::error<err::Kind::OP_BAD_ARGUMENT>(*this, d_sub, var.error());
    }

    return (*var)([] (const auto &typed) -> Result<Store>
    {
        return Store {typed};
    });
}

inline std::ostream &operator<<(std::ostream &os, const Const &var)
{
    return os << "const (_" << var.d_sub << ")";
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
    return os << var.d_name << " ($" << var.id() << ")";
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
        return err::error<err::Kind::OP_BAD_ARGUMENT>(*this, d_arg, item.error());
    }

    return (*item)([&op = d_op] (const auto &typed) -> Result<Store>
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
            return detail::eval<Store>(op, ltyped, rtyped);
        });
    });
}

template<template <typename> typename O>
std::ostream &operator<<(std::ostream &os, const Binary<O> &binary)
{
    return os << name<O>() << "(#" << binary.d_lhs << ", #" << binary.d_rhs << ")";
}


template <typename... Ts>
size_t hash(size_t seed, const Ts &...values)
{
    return seed ^= ((std::hash<Ts>{}(values) + 0x9e3779b9 + (seed << 6) + (seed >> 2)) ^ ...);
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


} // namespace std
