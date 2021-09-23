#pragma once

#include "error.h"
#include "op.h"
#include "ops.h"
#include "context.h"
#include "extensions.h"
#include "serial.h"

#include <tuple>
#include <unordered_map>


namespace pmql {


template<typename Store, typename... Funs>
class Expression;


/// Provides means of building and validating an Expression instance.
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
class Builder
{
    /// Reference to extension function pool.
    const ext::Pool<Funs...> &d_extensions;

    /// List of operations.
    op::List d_ops;

    /// Hash -> identifier mapping for added operation objects.
    std::unordered_map<size_t, op::Id> d_added;

    /// Variable counter for producing unique identifiers.
    size_t d_nextvar = 0;

    /// List of stored constant values.
    std::vector<Store> d_consts;

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


/// Expression that consists of mutiple steps and can be evaulated by substituting variable values.
///
/// Expression can include:
/// * Typed constants.
/// * Untyped variables.
/// * Arithmetic, logical or bitwise operations.
/// * Conditional branches.
/// * Extension functions.
///
/// Result type is defined by types of constants and variables. Operations, conditions and functions may enforce
/// typing rules on their arguments. If at some point types become incompatible, the error is returned.
///
/// Evaluation state (variable substitutions, evaluation results) are stored in Context instance, so a single
/// expression can be evaluated using different contexts and produce different results.
///
/// @tparam Store type that can store calculation results.
/// @tparam Funs pack of extension function types.
template<typename Store, typename... Funs>
class Expression
{
    /// Reference to a collection of extension functions.
    const ext::Pool<Funs...> &d_extensions;

    /// List of operations.
    const op::List d_ops;

    /// List of constants.
    const std::vector<Store> d_const;

private:
    /// Allow Builder to construct Expression instances.
    template<typename S, typename... Fs> friend class Builder;

    /// Streaming support.
    template<typename S, typename... Fs>
    friend std::ostream &operator<<(std::ostream &, const Expression<S, Fs...> &);

    /// Construct expression instance.
    /// @param exts collection of extension functions.
    /// @param ops valid ordered list of operations.
    /// @param constants list of constants.
    Expression(const ext::Pool<Funs...> &exts, op::List &&ops, std::vector<Store> &&constants);

    /// Evaluates an operation and writes results to the context.
    /// @tparam Substitute type that can set and store variable value (see Substitute contract).
    /// @param id operation identifier to evaluate.
    /// @param context evaluation context reference.
    template<typename Substitute>
    void eval(op::Id id, Context<Store, Substitute> &context) const;

public:
    /// Return ordered list of Expression's operations.
    /// @return list of operations.
    const op::List &operations() const;

    /// Return list of constants.
    /// @return list of constants.
    const std::vector<Store> &constants() const;

    /// Create evaluation context instance bound to this expression.
    /// @tparam Substitute type that can set and store variable value (see Substitute contract).
    /// @return evaluation context instance.
    template<typename Substitute>
    Context<Store, Substitute> context() const;

    /// Evaluate the expression using given context.
    /// @tparam Substitute type that can set and store variable value (see Substitute contract).
    /// @param context evaluation context.
    /// @param expression result or an error.
    template<typename Substitute>
    Result<Store> operator()(Context<Store, Substitute> &context) const;

    /// Write step-by-step expression evaluation log to an output stream.
    /// @tparam Substitute type that can set and store variable value (see Substitute contract).
    /// @param os output stream.
    /// @param context evaluation context.
    /// @return modified output stream.
    template<typename Substitute>
    std::ostream &log(std::ostream &os, const Context<Store, Substitute> &context) const;
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
/// @param ordered list of operations.
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


/// Serializes expression into a string.
template<typename Store, typename... Funs>
Result<std::string> store(const Expression<Store, Funs...> &expression)
{
    return serial::store(expression.operations(), expression.constants());
}


/// Loads expression from a string.
template<typename Store, typename... Funs>
Result<Expression<Store, Funs...>> load(std::string_view stored, const ext::Pool<Funs...> &extensions = ext::none)
{
    return serial::load<Store>(stored)
        .and_then([] (serial::Ingredients<Store> &&ingredients)
        {
            auto [ops, consts] = std::move(ingredients);
            return builder<Store>(std::move(ops), std::move(consts))();
        });
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
template<typename Op>
Result<op::Id> Builder<Store, Funs...>::append(Op &&op)
{
    auto [it, emplaced] = d_added.try_emplace(std::hash<Op> {}(op), d_ops.size());
    if (emplaced)
    {
        d_ops.push_back(std::move(op));
    }

    return it->second;
}

template<typename Store, typename... Funs>
Result<void> Builder<Store, Funs...>::visit(op::Id id, std::vector<bool> &visited) const
{
    if (id >= d_ops.size())
    {
        return error<err::Kind::BUILDER_REF_TO_UNKNOWN>(
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
                    const auto next = std::is_same_v<Op, op::Const> ? d_consts.size() : d_nextvar;
                    if (refcheck.has_value() && sub >= next)
                    {
                        refcheck = error<err::Kind::BUILDER_BAD_SUBSTITUTION>(
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
                        error<err::Kind::BUILDER_BAD_ARGUMENT>(d_ops, op, id, ref) :
                        visit(ref, visited);
                });
            }

            return refcheck;
        },
        d_ops[id]);
}

template<typename Store, typename... Funs>
Builder<Store, Funs...>::Builder(const ext::Pool<Funs...> &extensions /*= ext::none*/)
    : d_extensions(extensions)
{
}

template<typename Store, typename... Funs>
Builder<Store, Funs...>::Builder(
    std::vector<Store> &&consts,
    op::List &&ops,
    const ext::Pool<Funs...> &extensions /*= ext::none*/)
    : d_extensions(extensions)
    , d_consts(std::move(consts))
    , d_ops(std::move(ops))
    , d_nextvar(0)
{
    op::Id current = 0;
    for (const auto &op : d_ops)
    {
        std::visit(
            [this, current] (const auto &op) mutable
            {
                using Op = std::decay_t<decltype(op)>;

                if constexpr (std::is_same_v<Op, op::Const>)
                {
                    op::Id cn = 0;
                    op.refers([&cn] (op::Id ref) mutable { cn = ref; });
                    if (cn >= d_consts.size())
                    {
                        d_deferred = error<err::Kind::BUILDER_BAD_SUBSTITUTION>(
                            d_ops,
                            op,
                            current,
                            cn,
                            d_consts.size() - 1);
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
                                d_ops,
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
    auto result = append(op::Const {d_consts.size()});
    if (result.has_value() && *result == d_ops.size() - 1)
    {
        d_consts.emplace_back(std::forward<T>(value));
    }

    return result;
}

template<typename Store, typename... Funs>
Result<op::Id> Builder<Store, Funs...>::var(std::string_view name)
{
    auto result = append(op::Var {d_nextvar, name});
    if (result.has_value() && *result == d_ops.size() - 1)
    {
        ++d_nextvar;
    }

    return result;
}

template<typename Store, typename... Funs>
template<template<typename> typename Fn, typename... Ids>
Result<op::Id> Builder<Store, Funs...>::op(Ids ...ids)
{
    detail::CheckRef checkref {d_ops, op::Traits<Fn>::name};
    (checkref(ids), ...);

    return checkref()
        .and_then([this, args = std::forward_as_tuple(ids...)]
        {
            return append(std::make_from_tuple<typename op::Traits<Fn>::op>(std::move(args)));
        });
}

template<typename Store, typename... Funs>
template<typename... Ids>
Result<op::Id> Builder<Store, Funs...>::fun(std::string_view name, Ids ...ids)
{
    detail::CheckRef checkref {d_ops, name};
    (checkref(ids), ...);

    return checkref()
        .and_then([this, name]
        {
            return d_extensions[name];
        })
        .and_then([this, name, ids = std::vector<op::Id> {ids...}] (ext::FunId fun) mutable
        {
            return append(op::Extension {name, fun, std::move(ids)});
        });
}

template<typename Store, typename... Funs>
Result<op::Id> Builder<Store, Funs...>::branch(op::Id cond, op::Id iftrue, op::Id iffalse)
{
    detail::CheckRef checkref {d_ops, "if"};

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
Result<Expression<Store, Funs...>> Builder<Store, Funs...>::operator()() &&
{
    if (d_ops.empty())
    {
        return error<err::Kind::BUILDER_EMPTY>();
    }

    if (!d_deferred)
    {
        return error(std::move(d_deferred).error());
    }

    std::vector<bool> visited(d_ops.size(), false);

    return visit(d_ops.size() - 1, visited).and_then(
        [
            this,
            &visited,
            ops = std::move(d_ops),
            cn = std::move(d_consts)
        ]
        () mutable -> Result<Expression<Store, Funs...>>
        {
            auto dangling = std::find(visited.begin(), visited.end(), false);
            if (dangling != visited.end())
            {
                const size_t id = std::distance(visited.begin(), dangling);

                return std::visit(
                    [this, id] (const auto &op) -> Result<Expression<Store, Funs...>>
                    {
                        return error<err::Kind::BUILDER_DANGLING>(d_ops, op, id);
                    },
                    ops[id]);
            }

            return Expression<Store, Funs...> {d_extensions, std::move(ops), std::move(cn)};
        });
}

template<typename Store, typename... Funs>
std::ostream &operator<<(std::ostream &os, const Builder<Store, Funs...> &builder)
{
    os << "Operations:\n" << builder.d_ops;

    os << "\nConstants:\n";
    size_t id = 0;
    for (const auto &cn : builder.d_consts)
    {
        os << "\t_" << id++ << ": " << cn << "\n";
    }

    os << "\nExtension functions:\n";
    for (const auto &[name, funid] : builder.d_extensions)
    {
        os << "\t@" << funid << ": " << name << "\n";
    }

    return os;
}


template<typename Store, typename... Funs>
Expression<Store, Funs...>::Expression(const ext::Pool<Funs...> &exts, op::List &&ops, std::vector<Store> &&constants)
    : d_extensions(exts)
    , d_ops(std::move(ops))
    , d_const(std::move(constants))
{
}

template<typename Store, typename... Funs>
template<typename Substitute>
void Expression<Store, Funs...>::eval(op::Id id, Context<Store, Substitute> &context) const
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

            else if constexpr (std::is_same_v<Op, op::Extension>)
            {
                const auto &pool = d_extensions;
                res = op.template eval<Store>([this, &pool, &context] (auto fun, auto begin, auto end)
                {
                    return pool.template invoke<Store>(
                        fun,
                        [this, &context] (op::Id ref)
                        {
                            this->eval(ref, context);
                            return context.d_results[ref];
                        },
                        begin,
                        end);
                });
            }

            else // unary, binary, ternary
            {
                res = op.template eval<Store>([this, &context] (auto ref) mutable
                {
                    this->eval(ref, context);
                    return context.d_results[ref];
                });
            }
        },
        d_ops[id]);
}

template<typename Store, typename... Funs>
const op::List &Expression<Store, Funs...>::operations() const
{
    return d_ops;
}

template<typename Store, typename... Funs>
const std::vector<Store> &Expression<Store, Funs...>::constants() const
{
    return d_const;
}

template<typename Store, typename... Funs>
template<typename Substitute>
Context<Store, Substitute> Expression<Store, Funs...>::context() const
{
    return Context<Store, Substitute> {d_ops};
}

template<typename Store, typename... Funs>
template<typename Substitute>
Result<Store> Expression<Store, Funs...>::operator()(Context<Store, Substitute> &context) const
{
    eval(d_ops.size() - 1, context);
    return context.d_results[d_ops.size() - 1];
}

template<typename Store, typename... Funs>
template<typename Substitute>
std::ostream &Expression<Store, Funs...>::log(std::ostream &os, const Context<Store, Substitute> &context) const
{
    size_t id = 0;
    auto op = d_ops.begin();
    auto rs = context.d_results.begin();

    for (; op != d_ops.end() && rs != context.d_results.end(); ++op, ++rs)
    {
        os << "\t#" << id++ << ": " << *op << " = " << *rs << "\n";
    }

    return os;
}

template<typename Store, typename... Funs>
std::ostream &operator<<(std::ostream &os, const Expression<Store, Funs...> &expression)
{
    store(expression)
        .map([&os] (const std::string &stored) mutable
        {
            os << stored;
        })
        .map_error([&os] (const err::Error &e) mutable
        {
            os << e;
        });

    return os;
}


} // namespace pmql
