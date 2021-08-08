#pragma once

#include "error.h"
#include "ops.h"
#include <iterator>


namespace ops {


class Expression;


class Builder
{
    Ops d_ops;

public:
    template<typename O>
    Result<Id> add(O &&op);

    Result<Expression> build() &&;
};


class Expression
{
    static constexpr size_t OPS_LIMIT = 20u;

    Ops d_ops;

private:
    Expression(Ops &&ops);

    static Result<void> check(const Ops &ops, std::vector<bool> &visited, Id id);

    template<typename Fin, size_t Idx, typename...Res>
    void round(Fin &&fin, const Ops &ops, Res &&...results);

public:
    static Result<Expression> validated(Ops &&ops);
};



template<typename O>
Result<Id> Builder::add(O &&op)
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

template<typename Fin, size_t Idx, typename...Res>
void Expression::round(Fin &&fin, const Ops &ops, Res &&...results)
{
    // Idea:
    // * recursively iterate though ops
    // * call each op with a callback (to self) and pack of values, calculated by prev callbacks
    // * each op will pass its Result(result) the last argument in the callback call.
    // * on failure op will pass Result(error) instead of the value; all ops should be able to bubble up failures.

    if constexpr (Idx < OPS_LIMIT)
    {
        if (Idx < ops.size())
        {
            std::visit(
                [this, &fin, &ops, args = std::forward_as_tuple(results...)] (const auto &op)
                {
                    auto next = [this, &fin, &ops] (auto &&...res)
                    {
                        round(std::forward<Fin>(fin), ops, std::forward<decltype(res)>(res)...);
                    };

                    std::apply(op, std::tuple_cat(std::make_tuple(std::move(next)), std::move(args)));
                },
                ops[Idx]);
        }
        else
        {
            // we finished, time to call the last callback!
            //fin();
        }
    }
    else
    {
        if (Idx < ops.size())
        {
            // we didn't finish!
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





} // namespace ops
