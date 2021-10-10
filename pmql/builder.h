#pragma once

#include "error.h"
#include "op.h"
#include "ops.h"
#include "extensions.h"

#include <string_view>
#include <ostream>
#include <vector>
#include <unordered_map>


namespace pmql {


/// Contains all data necessary to describe an expression.
/// Store contract:
/// @code{.cpp}
/// struct SampleStore
/// {
///     template<typename T>
///     static std::string_view name()
///     {
///          // returns the name of T type (if supported).
///     }
///
///     SampleStore()
///     {
///         // stores a null value.
///     }
///
///     template<typename T>
///     explicit SampleStore(T &&value)
///     {
///         // stores non-null value of T type (if supported).
///     }
///
///     template<typename Visitor>
///     auto operator()(Visitor &&visitor)const -> decltype(auto)
///     {
///         // calls visitor with stored value, returns its result.
///     }
/// };
/// @endcode
/// @tparam Store type that can store calculation results.
/// @tparam Funs pack of extension function types.
template<typename Store, typename... Funs>
struct Ingredients
{
    /// Reference to extension function pool.
    const ext::Pool<Funs...> &extensions;

    /// List of operations.
    op::List ops;

    /// List of stored constant values.
    std::vector<Store> consts;

    /// Construct empty expression ingredients container.
    /// @param ext collection of extension functions.
    Ingredients(const ext::Pool<Funs...> &ext = ext::none);

    /// Construct expression ingredients from existing operations.
    /// @tparam Ops type, convertible to operation list.
    /// @tparam Consts type, convertible to constants list.
    /// @param ops ordered operation list.
    /// @param consts constants from the operation list.
    /// @param ext collection of extension functions.
    template<typename Ops, typename Consts>
    Ingredients(Ops &&ops, Consts &&consts, const ext::Pool<Funs...> &ext = ext::none);
};



template<typename Store, typename... Funs>
class Expression;


/// Provides means of building and validating an Expression instance.
/// @tparam Store type that can store calculation results.
/// @tparam Funs pack of extension function types.
template<typename Store, typename... Funs>
class Builder
{
    Ingredients<Store, Funs...> d_data;

    /// Hash -> identifier mapping for added operation objects.
    std::unordered_map<size_t, op::Id> d_added;

    /// Variable counter for producing unique identifiers.
    size_t d_nextvar = 0;

    /// Deferred error object used to fail expression construction because of errors,
    /// detected in some Builder constructors (to avoid constructor exceptions).
    Result<void> d_deferred;

private:
    /// Streaming support.
    template<typename S, typename... Fs>
    friend std::ostream &operator<<(std::ostream &, const Builder<S, Fs...> &);

    /// Append an operation to operation list or reuse an existing one, if it already exists.
    /// @tparam Op operation type.
    /// @param op operation to add.
    /// @return operation identifier or an error.
    template<typename Op>
    Result<op::Id> append(Op &&op);

    /// Traverse operation tree, mark visited operations and look for errors.
    /// @param id operation to visit.
    /// @param visited map of visited operations to mark.
    /// @return a success or a failure, if one of the visited operations has an issue.
    Result<void> visit(op::Id id, std::vector<bool> &visited) const;

public:
    /// Construct expression builder instance.
    /// @param extensions optional extension function pool.
    explicit Builder(const ext::Pool<Funs...> &extensions = ext::none);

    /// Construct and validate expression builder instance from expression ingredients.
    /// @param consts list of constant values.
    /// @param ops list of operations.
    /// @param extensions optional extension function pool.
    Builder(std::vector<Store> &&consts, op::List &&ops, const ext::Pool<Funs...> &extensions = ext::none);

    /// Returns a success or an error, if builder contents can't be used to construct a valid expression.
    /// @return builder contents status.
    Result<void> status() const;

    /// Add a constant value.
    /// @tparam T constant value type.
    /// @param value constant value.
    /// @return operation id or an error.
    template<typename T>
    Result<op::Id> constant(T &&value);

    /// Add a variable.
    /// @param name variable name, must be unique.
    /// @return operation id or an error.
    Result<op::Id> var(std::string_view name);

    /// Add an unary or binary operation.
    /// @see op::Any
    /// @tparam Fn supported std-like functional object.
    /// @tparam Ids one or two op::Id types.
    /// @param ids one or two operation identifiers, pointing to operation arguments.
    /// @return operation id or an error.
    template<template<typename> typename Fn, typename... Ids>
    Result<op::Id> op(Ids ...ids);

    /// Add an extension function.
    /// @see ext::Pool
    /// @tparam Ids zero or more op::Id types.
    /// @param name extension function name.
    /// @param ids function argument identifiers.
    /// @return operation id or an error.
    template<typename... Ids>
    Result<op::Id> fun(std::string_view name, Ids ...ids);

    /// Add an extension function.
    /// @see ext::Pool
    /// @param name extension function name.
    /// @param ids function argument identifiers.
    /// @return operation id or an error.
    Result<op::Id> fun(std::string_view name, std::vector<op::Id> &&ids);

    /// Add an `if-else` condition.
    /// @param cond operation that evaluates to bool, a condition.
    /// @param iftrue operation that will be the result if the condition evaluates to true.
    /// @param iffalse operation that will be the result if the condition evaluates to false.
    /// @return operation id or an error.
    Result<op::Id> branch(op::Id cond, op::Id iftrue, op::Id iffalse);

    /// Validate and build an Expression instance (consumes the builder).
    /// @return Expression instance or an error.
    Result<Expression<Store, Funs...>> operator()() &&;
};


/// Construct empty expression builder instance.
/// @tparam Store type that can store calculation results.
/// @tparam Funs pack of extension function types.
/// @param extensions collection of extension functions.
/// @return expression builder.
template<typename Store, typename... Funs>
Builder<Store, Funs...> builder(const ext::Pool<Funs...> &extensions = ext::none)
{
    return Builder<Store, Funs...> {extensions};
}

/// Construct expression builder instance from expression ingredients.
/// @tparam Store type that can store calculation results.
/// @tparam Funs pack of extension function types.
/// @param consts list of constants.
/// @param ops ordered list of operations.
/// @param extensions collection of extension functions.
/// @return builder instance or an error, if provided expression ingredients are invalid.
template<typename Store, typename... Funs>
Result<Builder<Store, Funs...>> builder(
    std::vector<Store> &&consts,
    op::List &&ops,
    const ext::Pool<Funs...> &extensions = ext::none)
{
    auto builder = Builder<Store, Funs...> {std::move(consts), std::move(ops), extensions};
    return builder.status().and_then([b = std::move(builder)] () mutable { return std::move(b); });
}


namespace detail {


/// Helper function object that checks if an operation arguments form a cycle.
class CheckRef
{
    const op::List &d_ops;
    const std::string_view d_op;

    Result<void> d_status;

public:
    CheckRef(const op::List &ops, std::string_view op)
        : d_ops(ops)
        , d_op(op)
    {
    }

    /// Check an operation, store the error, if any.
    void operator()(op::Id id)
    {
        if (d_status.has_value() && id >= d_ops.size())
        {
            d_status = error<err::Kind::BUILDER_REF_TO_UNKNOWN>(
                d_ops,
                d_op,
                id,
                d_ops.size() - 1);
        }
    }

    /// Get stored error, if any.
    const Result<void> &operator()() const
    {
        return d_status;
    }
};


} // namespace detail


template<typename Store, typename... Funs>
Ingredients<Store, Funs...>::Ingredients(const ext::Pool<Funs...> &ext /*= ext::none*/)
    : extensions(ext)
{
}

template<typename Store, typename... Funs>
template<typename Ops, typename Consts>
Ingredients<Store, Funs...>::Ingredients(Ops &&ops, Consts &&consts, const ext::Pool<Funs...> &ext /*= ext::none*/)
    : extensions(ext)
    , ops(std::forward<Ops>(ops))
    , consts(std::forward<Consts>(consts))
{
}

template<typename Store, typename... Funs>
std::ostream &operator<<(std::ostream &os, const Ingredients<Store, Funs...> &data)
{
    os << "Operations:\n" << data.ops;

    os << "\nConstants:\n";
    size_t id = 0;
    for (const auto &cn : data.consts)
    {
        os << "\t_" << id++ << ": " << cn << "\n";
    }

    os << "\nExtension functions:\n";
    for (const auto &[name, funid] : data.extensions)
    {
        os << "\t@" << funid << ": " << name << "\n";
    }

    return os;
}

template<typename Store, typename... Funs>
template<typename Op>
Result<op::Id> Builder<Store, Funs...>::append(Op &&op)
{
    auto [it, emplaced] = d_added.try_emplace(std::hash<Op> {}(op), d_data.ops.size());
    if (emplaced)
    {
        d_data.ops.push_back(std::move(op));
    }

    return it->second;
}

template<typename Store, typename... Funs>
Result<void> Builder<Store, Funs...>::visit(op::Id id, std::vector<bool> &visited) const
{
    if (id >= d_data.ops.size())
    {
        return error<err::Kind::BUILDER_REF_TO_UNKNOWN>(
            d_data.ops,
            err::format("Operation #", id),
            id,
            d_data.ops.size() - 1);
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
                    const auto next = std::is_same_v<Op, op::Const> ? d_data.consts.size() : d_nextvar;
                    if (refcheck.has_value() && sub >= next)
                    {
                        refcheck = error<err::Kind::BUILDER_BAD_SUBSTITUTION>(
                            d_data.ops,
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
                        error<err::Kind::BUILDER_BAD_ARGUMENT>(d_data.ops, op, id, ref) :
                        visit(ref, visited);
                });
            }

            return refcheck;
        },
        d_data.ops[id]);
}

template<typename Store, typename... Funs>
Builder<Store, Funs...>::Builder(const ext::Pool<Funs...> &extensions /*= ext::none*/)
    : d_data(extensions)
{
}

template<typename Store, typename... Funs>
Builder<Store, Funs...>::Builder(
    std::vector<Store> &&consts,
    op::List &&ops,
    const ext::Pool<Funs...> &extensions /*= ext::none*/)
    : d_data(std::move(ops), std::move(consts), extensions)
    , d_nextvar(0)
{
    op::Id current = 0;
    for (const auto &op : d_data.ops)
    {
        std::visit(
            [this, current] (const auto &op) mutable
            {
                using Op = std::decay_t<decltype(op)>;

                if constexpr (std::is_same_v<Op, op::Const>)
                {
                    op::Id cn = 0;
                    op.refers([&cn] (op::Id ref) mutable { cn = ref; });
                    if (cn >= d_data.consts.size())
                    {
                        d_deferred = error<err::Kind::BUILDER_BAD_SUBSTITUTION>(
                            d_data.ops,
                            op,
                            current,
                            cn,
                            d_data.consts.size() - 1);
                    }
                }
                else if constexpr (std::is_same_v<Op, op::Var>)
                {
                    ++d_nextvar;
                }
                else // ops
                {
                    op.refers([this, current, &op] (op::Id ref) mutable
                    {
                        if (d_deferred.has_value() && ref >= current)
                        {
                            d_deferred = error<err::Kind::BUILDER_BAD_ARGUMENT>(
                                d_data.ops,
                                op,
                                current,
                                ref);
                        }
                    });
                }
            },
            op);

        if (!d_deferred.has_value())
        {
            break;
        }

        ++current;
    }
}

template<typename Store, typename... Funs>
Result<void> Builder<Store, Funs...>::status() const
{
    return d_deferred;
}

template<typename Store, typename... Funs>
template<typename T>
Result<op::Id> Builder<Store, Funs...>::constant(T &&value)
{
    auto result = append(op::Const {d_data.consts.size()});
    if (result.has_value() && *result == d_data.ops.size() - 1)
    {
        d_data.consts.emplace_back(std::forward<T>(value));
    }

    return result;
}

template<typename Store, typename... Funs>
Result<op::Id> Builder<Store, Funs...>::var(std::string_view name)
{
    auto result = append(op::Var {d_nextvar, name});
    if (result.has_value() && *result == d_data.ops.size() - 1)
    {
        ++d_nextvar;
    }

    return result;
}

template<typename Store, typename... Funs>
template<template<typename> typename Fn, typename... Ids>
Result<op::Id> Builder<Store, Funs...>::op(Ids ...ids)
{
    detail::CheckRef checkref {d_data.ops, op::Traits<Fn>::name};
    (checkref(ids), ...);

    return checkref()
        .and_then([this, args = std::forward_as_tuple(ids...)]
        {
            return append(std::make_from_tuple<typename op::Traits<Fn>::op>(std::move(args)));
        });
}

template<typename Store, typename... Funs>
Result<op::Id> Builder<Store, Funs...>::branch(op::Id cond, op::Id iftrue, op::Id iffalse)
{
    detail::CheckRef checkref {d_data.ops, "if"};

    checkref(cond);
    checkref(iftrue);
    checkref(iffalse);

    return checkref()
        .and_then([this, cond, iftrue, iffalse]
        {
            return append(op::Ternary {cond, iftrue, iffalse});
        });
}

template<typename Store, typename... Funs>
template<typename... Ids>
Result<op::Id> Builder<Store, Funs...>::fun(std::string_view name, Ids ...ids)
{
    detail::CheckRef checkref {d_data.ops, name};
    (checkref(ids), ...);

    return checkref()
        .and_then([this, name]
        {
            return d_data.extensions[name];
        })
        .and_then([this, name, ids = std::vector<op::Id> {ids...}] (ext::FunId fun) mutable
        {
            return append(op::Extension {name, fun, std::move(ids)});
        });
}

template<typename Store, typename... Funs>
Result<op::Id> Builder<Store, Funs...>::fun(std::string_view name, std::vector<op::Id> &&ids)
{
    detail::CheckRef checkref {d_data.ops, name};
    for (auto arg : ids)
    {
        checkref(arg);
    }

    return checkref()
        .and_then([this, name]
        {
            return d_data.extensions[name];
        })
        .and_then([this, name, args = std::move(ids)] (ext::FunId fun) mutable
        {
            return append(op::Extension {name, fun, std::move(args)});
        });
}

template<typename Store, typename... Funs>
Result<Expression<Store, Funs...>> Builder<Store, Funs...>::operator()() &&
{
    if (d_data.ops.empty())
    {
        return error<err::Kind::BUILDER_EMPTY>();
    }

    if (!d_deferred)
    {
        return error(std::move(d_deferred).error());
    }

    std::vector<bool> visited(d_data.ops.size(), false);

    return visit(d_data.ops.size() - 1, visited)
        .and_then([&visited, data = std::move(d_data)] () mutable -> Result<Expression<Store, Funs...>>
        {
            auto dangling = std::find(visited.begin(), visited.end(), false);
            if (dangling != visited.end())
            {
                const size_t id = std::distance(visited.begin(), dangling);

                return std::visit(
                    [&data, id] (const auto &op) -> Result<Expression<Store, Funs...>>
                    {
                        return error<err::Kind::BUILDER_DANGLING>(data.ops, op, id);
                    },
                    data.ops[id]);
            }

            return Expression<Store, Funs...> {std::move(data)};
        });
}

template<typename Store, typename... Funs>
std::ostream &operator<<(std::ostream &os, const Builder<Store, Funs...> &builder)
{
    os << "Operations:\n" << builder.d_data.ops;

    os << "\nConstants:\n";
    size_t id = 0;
    for (const auto &cn : builder.d_data.consts)
    {
        os << "\t_" << id++ << ": " << cn << "\n";
    }

    os << "\nExtension functions:\n";
    for (const auto &[name, funid] : builder.d_data.extensions)
    {
        os << "\t@" << funid << ": " << name << "\n";
    }

    return os;
}


} // namespace pmql
