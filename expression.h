#pragma once

#include "op.h"
#include "ops.h"
#include "error.h"


namespace pmql {


class Expression;


class Builder
{
    op::List d_ops;

    template<typename Op>
    Result<op::Id> add(Op &&op);

public:
    template<template<typename> typename Fn>
    Result<op::Id> add(op::Id arg);

    template<template<typename> typename Fn>
    Result<op::Id> add(op::Id lhs, op::Id rhs);

    Result<op::Id> add(std::string_view name);

    Result<Expression> build() &&;
};


class Expression
{
    const op::List d_ops;
    const Result<void> d_valid;

public:
    Expression(op::List &&ops);
};


template<typename Op>
Result<op::Id> Builder::add(Op &&op)
{
    d_ops.push_back(std::move(op));
    return d_ops.size() - 1;
}

template<template<typename> typename Fn>
Result<op::Id> Builder::add(op::Id arg)
{
    if (arg >= d_ops.size())
    {
        return err::error<err::Kind::BUILDER_REF_TO_UNKNOWN>(op::Traits<Fn>::name, arg, d_ops.size() - 1);
    }

    return add(typename op::Traits<Fn>::op {arg});
}

template<template<typename> typename Fn>
Result<op::Id> Builder::add(op::Id lhs, op::Id rhs)
{
    if (lhs >= d_ops.size())
    {
        return err::error<err::Kind::BUILDER_REF_TO_UNKNOWN>(op::Traits<Fn>::name, lhs, d_ops.size() - 1);
    }

    if (rhs >= d_ops.size())
    {
        return err::error<err::Kind::BUILDER_REF_TO_UNKNOWN>(op::Traits<Fn>::name, rhs, d_ops.size() - 1);
    }

    return add(typename op::Traits<Fn>::op {lhs, rhs});
}

Result<op::Id> Builder::add(std::string_view name)
{
    return add(op::Var {d_ops.size(), name});
}

Result<Expression> Builder::build() &&
{
    // validate

    return Expression {std::move(d_ops)};
}



} // namespace pmql
