#pragma once

#include "error.h"
#include "ops.h"
#include "context.h"

#include <tuple>
#include <unordered_map>


namespace pmql {


template<typename Store>
class Expression;


template<typename Store>
class Builder
{
    op::List d_ops;
    std::unordered_map<size_t, op::Id> d_added;

    size_t d_nextvar   = 0;
    std::vector<Store> d_const;

private:
    template<typename S>
    friend std::ostream &operator<<(std::ostream &, const Builder<S> &);

    template<typename Op>
    Result<op::Id> append(Op &&op);

    Result<void> visit(op::Id id, std::vector<bool> &visited) const;

public:
    template<typename T>
    Result<op::Id> constant(T &&value);

    Result<op::Id> var(std::string_view name);

    template<template<typename> typename Fn, typename... Ids>
    Result<op::Id> op(Ids ...args);

    Result<Expression<Store>> operator()() &&;
};


template<typename Store>
class Expression
{
    const op::List d_ops;
    const std::vector<Store> d_const;

private:
    template<typename S> friend class Builder;

    template<typename S>
    friend std::ostream &operator<<(std::ostream &, const Expression<S> &);

    explicit Expression(op::List &&ops, std::vector<Store> &&constants);

    std::ostream &format(std::ostream &os, op::Id current) const;

    template<typename Substitute>
    void eval(op::Id id, Context<Store, Substitute> &context) const;

public:
    template<typename Substitute>
    Context<Store, Substitute> context() const;

    template<typename Substitute>
    Result<Store> operator()(Context<Store, Substitute> &context) const;

    template<typename Substitute>
    std::ostream &log(std::ostream &os, const Context<Store, Substitute> &context) const;
};


template<typename Store>
template<typename Op>
Result<op::Id> Builder<Store>::append(Op &&op)
{
    auto [it, emplaced] = d_added.try_emplace(std::hash<Op> {}(op), d_ops.size());
    if (emplaced)
    {
        d_ops.push_back(std::move(op));
    }

    return it->second;
}

template<typename Store>
Result<void> Builder<Store>::visit(op::Id id, std::vector<bool> &visited) const
{
    if (id >= d_ops.size())
    {
        return err::error<err::Kind::BUILDER_REF_TO_UNKNOWN>(
            d_ops,
            err::format("Operation #", id),
            id,
            d_ops.size() - 1);
    }

    visited[id] = true;

    return std::visit(
        [this, id, &visited] (const auto &op) mutable
        {
            Result<void> refcheck;

            using Op = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<Op, op::Const> || std::is_same_v<Op, op::Var>)
            {
                op.refers([this, id, &op, &refcheck] (op::Id sub) mutable
                {
                    const auto next = std::is_same_v<Op, op::Const> ? d_const.size() : d_nextvar;
                    if (refcheck.has_value() && sub >= next)
                    {
                        refcheck = err::error<err::Kind::BUILDER_BAD_SUBSTITUTION>(
                            d_ops,
                            op,
                            id,
                            sub,
                            next - 1);
                    }
                });
            }
            else
            {
                op.refers([this, id, &op, &visited, &refcheck] (op::Id ref) mutable
                {
                    if (!refcheck.has_value())
                    {
                        return;
                    }

                    refcheck = ref >= id ?
                        err::error<err::Kind::BUILDER_BAD_ARGUMENT>(d_ops, op, id, ref) :
                        visit(ref, visited);
                });
            }

            return refcheck;
        },
        d_ops[id]);
}

template<typename Store>
template<typename T>
Result<op::Id> Builder<Store>::constant(T &&value)
{
    auto result = append(op::Const {d_const.size()});
    if (result.has_value() && *result == d_ops.size() - 1)
    {
        d_const.emplace_back(std::forward<T>(value));
    }

    return result;
}

template<typename Store>
Result<op::Id> Builder<Store>::var(std::string_view name)
{
    auto result = append(op::Var {d_nextvar, name});
    if (result.has_value() && *result == d_ops.size() - 1)
    {
        ++d_nextvar;
    }

    return result;
}

template<typename Store>
template<template<typename> typename Fn, typename... Ids>
Result<op::Id> Builder<Store>::op(Ids ...ids)
{
    Result<void> goodref;
    auto checkref = [this, &goodref] (op::Id ref) mutable
    {
        if (goodref.has_value() && ref >= d_ops.size())
        {
            goodref = err::error<err::Kind::BUILDER_REF_TO_UNKNOWN>(
                d_ops,
                op::Traits<Fn>::name,
                ref,
                d_ops.size() - 1);
        }
    };

    (checkref(ids), ...);

    return goodref.and_then([this, args = std::forward_as_tuple(ids...)]
    {
        return append(std::make_from_tuple<typename op::Traits<Fn>::op>(std::move(args)));
    });
}

template<typename Store>
Result<Expression<Store>> Builder<Store>::operator()() &&
{
    if (d_ops.empty())
    {
        return err::error<err::Kind::BUILDER_EMPTY>();
    }

    std::vector<bool> visited(d_ops.size(), false);

    return visit(d_ops.size() - 1, visited)
        .and_then([this, &visited, ops = std::move(d_ops), cn = std::move(d_const)] () mutable -> Result<Expression<Store>>
        {
            auto dangling = std::find(visited.begin(), visited.end(), false);
            if (dangling != visited.end())
            {
                const size_t id = std::distance(visited.begin(), dangling);

                return std::visit(
                    [this, id] (const auto &op) -> Result<Expression<Store>>
                    {
                        return err::error<err::Kind::BUILDER_DANGLING>(d_ops, op, id);
                    },
                    ops[id]);
            }

            return Expression<Store> {std::move(ops), std::move(cn)};
        });
}

template<typename Store>
std::ostream &operator<<(std::ostream &os, const Builder<Store> &builder)
{
    os << "Operations:\n" << builder.d_ops;

    os << "\nConstants:\n";
    size_t id = 0;
    for (const auto &cn : builder.d_const)
    {
        os << "\t_" << id++ << ": " << cn << "\n";
    }

    return os;
}


template<typename Store>
Expression<Store>::Expression(op::List &&ops, std::vector<Store> &&constants)
    : d_ops(std::move(ops))
    , d_const(std::move(constants))
{
}

template<typename Store>
std::ostream &Expression<Store>::format(std::ostream &os, op::Id current) const
{
    std::visit(
        [this, &os] (const auto &op) mutable
        {
            using Op = std::decay_t<decltype(op)>;

            if constexpr (std::is_same_v<Op, op::Const>)
            {
                op::Id subst;
                op.refers([&subst] (op::Id id) mutable { subst = id; });
                os << d_const[subst];
            }

            else if constexpr (std::is_same_v<Op, op::Var>)
            {
                os << "$" << op.name();
            }

            else
            {
                using Tr = typename op::OpTraits<Op>::type;

                if constexpr (Tr::max_arity == 1)
                {
                    os << Tr::sign << "(";
                    op.refers([this, &os] (op::Id ref) mutable { format(os, ref); });
                    os << ")";
                }
                else if constexpr (Tr::max_arity == 2)
                {
                    os << "(";
                    bool first = true;
                    op.refers([this, &os, &first] (op::Id ref) mutable
                    {
                        if (first)
                        {
                            format(os, ref);
                            first = false;
                        }
                        else
                        {
                            format(os << " " << Tr::sign << " ", ref);
                        }
                    });
                    os << ")";
                }
                else
                {
                    os << Tr::name << "(";
                    bool first = true;
                    op.refers([this, &os, &first] (op::Id ref) mutable
                    {
                        if (first)
                        {
                            format(os, ref);
                            first = false;
                        }
                        else
                        {
                            format(os << ", ", ref);
                        }
                    });
                    os << ")";
                }
            }
        },
        d_ops[current]);

    return os;
}

template<typename Store>
template<typename Substitute>
void Expression<Store>::eval(op::Id id, Context<Store, Substitute> &context) const
{
    std::visit(
        [this, id, &context] (const auto &op) mutable
        {
            using Op = std::decay_t<decltype(op)>;
            auto &res = context.d_results[id];

            if constexpr (std::is_same_v<Op, op::Const>)
            {
                res = op.template eval<Store>([&cns = d_const] (auto cn)
                {
                    const auto &store = cns[cn];
                    return Result<std::decay_t<decltype(store)>> {store};
                });
            }

            else if constexpr (std::is_same_v<Op, op::Var>)
            {
                const auto &subs = context.d_substitutions;
                res = op.template eval<Store>([&subs] (auto sub)
                {
                    return subs[sub].template eval<Store>();
                });
            }

            else
            {
                res = op.template eval<Store>([this, &context] (auto ref) mutable
                {
                    eval(ref, context);
                    return context.d_results[ref];
                });
            }
        },
        d_ops[id]);
}

template<typename Store>
template<typename Substitute>
Context<Store, Substitute> Expression<Store>::context() const
{
    return Context<Store, Substitute> {d_ops};
}

template<typename Store>
template<typename Substitute>
Result<Store> Expression<Store>::operator()(Context<Store, Substitute> &context) const
{
    eval(d_ops.size() - 1, context);
    return context.d_results[d_ops.size() - 1];
}

template<typename Store>
template<typename Substitute>
std::ostream &Expression<Store>::log(std::ostream &os, const Context<Store, Substitute> &context) const
{
    os << "Constants:" << (d_const.empty() ? " <none>" : "\n");
    size_t id = 0;
    for (const auto &cn : d_const)
    {
        os << "\t_" << id++ << ": " << cn << "\n";
    }

    os << "\nVariables:" << (context.d_substitutions.empty() ? " <none>" : "\n");
    for (const auto &var : context.d_substitutions)
    {
        os << "\t$" << var.name() << ": " << var << "\n";
    }

    os << "\nEvaluations:\n";

    id = 0;
    auto op = d_ops.begin();
    auto rs = context.d_results.begin();

    for (; op != d_ops.end() && rs != context.d_results.end(); ++op, ++rs)
    {
        os << "\t#" << id++ << ": " << *op << " = " << *rs << "\n";
    }

    return os;
}

template<typename Store>
std::ostream &operator<<(std::ostream &os, const Expression<Store> &expression)
{
    return expression.format(os, expression.d_ops.size() - 1);
}


} // namespace pmql
