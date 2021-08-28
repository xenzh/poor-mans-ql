#pragma once

#include "error.h"
#include "ops.h"
#include <iterator>
#include <tuple>


namespace ops {


class Expression;


class Builder
{
    Ops d_ops;

    template<typename Item>
    Result<Id> add(Item &&op);

public:
    template<template<typename> typename Fn>
    Result<Id> add(Id arg);

    template<template<typename> typename Fn>
    Result<Id> add(Id lhs, Id rhs);

    Result<Id> add(std::string_view name);

    Result<Expression> build() &&;
};


class Expression
{
    static constexpr size_t OPS_LIMIT = 20u;

    Ops d_ops;

private:
    Expression(Ops &&ops);

    static Result<void> check(const Ops &ops, std::vector<bool> &visited, Id id);

    template<size_t Idx, typename Fin, typename... Substitutions, typename...Res>
    void round(Fin &&fin, const std::tuple<Substitutions &&...> &substitutions, Res &&...results);

public:
    static Result<Expression> validated(Ops &&ops);

    template<typename Fin, typename... Substitutions>
    void operator()(Fin &&fin, Substitutions &&...substitutions) const;
};



template<typename Item>
Result<Id> Builder::add(Item &&op)
{
    auto valid = std::visit(
        [this] (const auto &item) -> Result<void>
        {
            return item.refers([this, &item] (Id &ref) ->  Result<void>
            {
                if (ref >= d_ops.size())
                {
                    return err::error<err::Kind::OPS_REF_TO_UNKNOWN>(err::format(item), ref, d_ops.size());
                }
                return {};
            });
        },
        op);

    return valid.map([this] { d_ops.emplace_back(); return d_ops.size() - 1; });
}

template<template<typename> typename Fn>
Result<Id> Builder::add(Id arg)
{
    
}

template<template<typename> typename Fn>
Result<Id> Builder::add(Id lhs, Id rhs)
{
}

inline Result<Id> Builder::add(std::string_view name)
{
}

inline Result<Expression> Builder::build() &&
{
    return Expression::validated(std::move(d_ops));
}


Expression::Expression(Ops &&ops)
    : d_ops(std::move(ops))
{
}

/* static */ Result<void> Expression::check(const Ops &ops, std::vector<bool> &visited, Id id)
{
    if (visited[id])
    {
        return err::error<err::Kind::OPS_CYCLE>(err::format(ops), id);
    }

    visited[id] = true;

    return std::visit(
        [&ops, &visited] (const auto &op)
        {
            Result<void> inner;
            op.refers([&ops, &visited, &inner] (auto ref) mutable
            {
                inner = inner.and_then([&ops, &visited, &ref] { return check(ops, visited, ref); });
            });

            return inner;
        },
        ops[id]);
}

template<size_t Idx, typename Fin, typename... Substitutions, typename...Results>
void Expression::round(Fin &&fin, const std::tuple<Substitutions &&...> &substitutions, Results &&...results)
{
    // Idea:
    // * recursively iterate though ops
    // * call each op with a callback (to self) and pack of results from previous rounds.
    // * each op passes its Result(result) as the last argument in the callback to this method.
    // * on failure op yields Result(error) instead of the value; all ops should be able to bubble up failures.
    // * variable ops pick a value from the pack of substitutions and call the callback with it.

    if constexpr (Idx < OPS_LIMIT)
    {
        // Process next operation
        if (Idx < d_ops.size())
        {
            std::visit(
                [
                    this,
                    &fin,
                    &subs = substitutions,
                    args = std::forward_as_tuple(results...)] (const auto &op)
                {
                    // callback for the next calculation round
                    auto next = [this, &fin] (auto &&...res)
                    {
                        round<Idx + 1>(std::forward<Fin>(fin), std::forward<decltype(res)>(res)...);
                    };

                    // op is a variable, get value from substitutions and proceed.
                    if constexpr (std::is_same_v<Var, std::decay_t<decltype(op)>>)
                    {
                        auto wrap = [this, next = std::move(next), args = std::move(args)] (auto &&value)
                        {
                            std::apply(next, std::tuple_cat(std::move(args), std::forward_as_tuple(value)));
                        };

                        std::apply(op, std::tuple_cat(std::make_tuple(std::move(wrap)), subs));
                    }

                    // op is a functor, calculate the value and proceed.
                    else
                    {
                        std::apply(op, std::tuple_cat(std::make_tuple(std::move(next)), std::move(args)));
                    }

                },
                d_ops[Idx]);
        }

        // we're done, time to call the last callback!
        else if (Idx == d_ops.size())
        {
            util::last(fin, std::forward<Results>(results)...);
        }
    }
}

/* static */ Result<Expression> Expression::validated(Ops &&ops)
{
    if (ops.empty())
    {
        return err::error<err::Kind::OPS_EMPTY>();
    }

    if (ops.size() >= OPS_LIMIT)
    {
        return err::error<err::Kind::OPS_TOO_MANY>(err::format(ops), OPS_LIMIT, ops.size());
    }

    std::vector<bool> visited(ops.size(), false);

    return check(ops, visited, ops.size() - 1)
        .and_then([&ops, &visited] () -> Result<void>
        {
            auto it = std::find(visited.begin(), visited.end(), false);
            if (it != visited.end())
            {
                const size_t id = std::distance(visited.begin(), it);
                return err::error<err::Kind::OPS_DANGLING>(err::format(ops), id);
            }
            return {};
        })
        .map([valid = std::move(ops)] () mutable
        {
            return Expression {std::move(valid)};
        });
}

template<typename Fin, typename... Substitutions>
void Expression::operator()(Fin &&fin, Substitutions &&...substitutions) const
{
    if (d_ops.size() >= OPS_LIMIT)
    {
        fin(Result<void> {err::error<err::Kind::OPS_TOO_MANY>(err::format(d_ops), OPS_LIMIT, d_ops.size())});
        return;
    }

    round<0>(std::forward<Fin>(fin), std::forward_as_tuple(substitutions...));
}


} // namespace ops
