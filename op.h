#pragma once

#include "util.h"
#include "error.h"

#include <iterator>
#include <type_traits>
#include <functional>

#include <variant>
#include <vector>
#include <string>

//removeme
#include <iostream>


namespace ops {
namespace traits {


template<typename UnaryFn, typename Arg>
using unary_result_t = std::invoke_result_t<UnaryFn, const Arg &>;


template<template <typename> typename F> struct Fn;


} // namespace traits


using Id = size_t;


class Var
{
    friend std::ostream &operator<<(std::ostream &os, const Var &item);

    const std::string d_name;

public:
    Var(std::string_view name);

    std::string_view name() const;

    template<typename Cb>
    void refers(Cb &&callback) const;
};


class Const
{
    friend std::ostream &operator<<(std::ostream &os, const Const &item);

    const Id d_arg;

public:
    Const(Id arg);

    template<typename Cb>
    void refers(Cb &&callback) const;
};


template<template<typename> typename Fn>
class Unary
{
    template<template<typename> typename F>
    friend std::ostream &operator<<(std::ostream &os, const Unary<F> &item);

    using type = typename traits::Fn<Fn>::type;

    const type d_functor;
    const Id d_arg;

public:
    Unary(Id arg);

    std::string_view name() const;

    template<typename Cb>
    void refers(Cb &&callback) const;

    template<typename Next, typename... Args>
    void operator()(Next &&next, Args &&...args) const;
};


template<template<typename> typename Fn>
class Binary
{
    template<template<typename> typename F>
    friend std::ostream &operator<<(std::ostream &os, const Binary<F> &item);

    using type = typename traits::Fn<Fn>::type;

    const type d_functor;
    const Id d_lhs;
    const Id d_rhs;

public:
    Binary(Id lhs, Id rhs);

    std::string_view name() const;

    template<typename Cb>
    void refers(Cb &&callback) const;
};


template<template<typename> typename Fn>
using func_t = std::conditional_t<traits::Fn<Fn>::arity == 1, Unary<Fn>, Binary<Fn>>;


#define OpList \
    OpItem(plus      , 2) \
    OpItem(minus     , 2) \
    OpItem(multiplies, 2) \
    OpItem(divides   , 2) \
    OpItem(modulus   , 2) \
    OpItem(negate    , 1)


namespace traits {


#define OpItem(Item, Arity) \
template<> struct Fn<std::Item> \
{ \
    using type = std::Item<void>; \
    static constexpr std::string_view name = #Item; \
    static constexpr size_t arity = Arity; \
};
OpList;
#undef OpItem


template<template<typename...> typename To, typename... Extra>
struct expand
{
    template<template<typename...> typename T, template<typename> typename... Ts>
    using macro_expander_t = T<func_t<Ts>...>;

    template<typename T> struct cat;

    template<typename... Args> struct cat<To<Args...>>
    {
        template<typename... With> using with = To<Args..., With...>;
    };

#define OpItem(Item, Arity) , std::Item
    using expanded = macro_expander_t<To OpList>;
#undef OpItem

    using type = typename cat<expanded>::template with<Extra...>;
};


template<template<typename...> typename To, typename... Extra>
using expand_t = typename expand<To, Extra...>::type;


} // namespace traits


#undef OpList


inline std::ostream &operator<<(std::ostream &os, const Var &item)
{
    return os << "var(" << item.name() << ")";
}

inline Var::Var(std::string_view name)
    : d_name(name)
{
}

inline std::string_view Var::name() const
{
    return d_name;
}

template<typename Cb>
void Var::refers(Cb &&) const
{
}


inline std::ostream &operator<<(std::ostream &os, const Const &item)
{
    return os << "const(#" << item.d_arg << ")";
}

inline Const::Const(Id arg)
    : d_arg(arg)
{
}

template<typename Cb>
void Const::refers(Cb &&callback) const
{
    callback(d_arg);
}


template<template<typename> typename Fn>
std::ostream &operator<<(std::ostream &os, const Unary<Fn> &item)
{
    return os << item.name() << "(#" << item.d_arg << ")";
}

template<template<typename> typename Fn>
Unary<Fn>::Unary(Id arg)
    : d_arg(arg)
{
}

template<template<typename> typename Fn>
std::string_view Unary<Fn>::name() const
{
    return traits::Fn<Fn>::name;
}

template<template<typename> typename Fn>
template<typename Cb>
void Unary<Fn>::refers(Cb &&callback) const
{
    callback(d_arg);
}

template<template<typename> typename Fn>
template<typename Next, typename... Args>
void Unary<Fn>::operator()(Next &&next, Args &&...args) const
{
    util::nth(
        [
            this,
            &next,
            id = d_arg,
            &fn = d_functor,
            args = std::forward_as_tuple(std::forward<Args>(args)...)
        ]
        (auto &&arg)
        {
            if (!arg)
            {
                std::apply(next, std::tuple_cat(std::move(args), std::make_tuple(arg)));
            }
            else
            {
                std::apply(next, std::tuple_cat(std::move(args), std::make_tuple(Result {fn(*arg)})));
            }
        },
        d_arg,
        std::forward<Args>(args)...
    );
}


template<template<typename> typename Fn>
std::ostream &operator<<(std::ostream &os, const Binary<Fn> &item)
{
    return os << item.name() << "(#" << item.d_lhs << ", #" << item.d_rhs << ")";
}

template<template<typename> typename Fn>
Binary<Fn>::Binary(Id lhs, Id rhs)
    : d_lhs(lhs)
    , d_rhs(rhs)
{
}

template<template<typename> typename Fn>
std::string_view Binary<Fn>::name() const
{
    return traits::Fn<Fn>::name;
}

template<template<typename> typename Fn>
template<typename Cb>
void Binary<Fn>::refers(Cb &&callback) const
{
    callback(d_lhs);
    callback(d_rhs);
}


} // namespace ops
