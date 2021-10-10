#pragma once

#include "error.h"
#include "context.h"
#include "builder.h"

#include <type_traits>
#include <ostream>


namespace pmql {


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
    Ingredients<Store, Funs...> d_data;

private:
    /// Allow Builder to construct Expression instances.
    template<typename S, typename... Fs> friend class Builder;

    /// Streaming support.
    template<typename S, typename... Fs>
    friend std::ostream &operator<<(std::ostream &, const Expression<S, Fs...> &);

    /// Construct expression instance.
    /// @param ingredients expression contents.
    explicit Expression(Ingredients<Store, Funs...> &&ingredients);

    /// Evaluates an operation and writes results to the context.
    /// @tparam Substitute type that can set and store variable value (see Substitute contract).
    /// @param id operation identifier to evaluate.
    /// @param context evaluation context reference.
    template<typename Substitute>
    void eval(op::Id id, Context<Store, Substitute> &context) const;

public:
    /// Return expression contents.
    /// @return expression ingredients.
    const Ingredients<Store, Funs...> &ingredients() const;

    /// Create evaluation context instance bound to this expression.
    /// @tparam Substitute type that can set and store variable value (see Substitute contract).
    /// @param cache if set to true, context will cache evaluated operation results.
    /// @return evaluation context instance.
    template<typename Substitute>
    Context<Store, Substitute> context(bool cache = true) const;

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


template<typename Store, typename... Funs>
Expression<Store, Funs...>::Expression(Ingredients<Store, Funs...> &&ingredients)
    : d_data(std::move(ingredients))
{
}

template<typename Store, typename... Funs>
template<typename Substitute>
void Expression<Store, Funs...>::eval(op::Id id, Context<Store, Substitute> &context) const
{
    if (context.d_results[id])
    {
        return;
    }

    std::visit(
        [this, id, &context] (const auto &op) mutable
        {
            using Op = std::decay_t<decltype(op)>;
            auto handle = context.d_results[id];

            if constexpr (std::is_same_v<Op, op::Const>)
            {
                handle = op.template eval<Store>([&cns = d_data.consts] (auto cn)
                {
                    const auto &store = cns[cn];
                    return Result<std::decay_t<decltype(store)>> {store};
                });
            }

            else if constexpr (std::is_same_v<Op, op::Var>)
            {
                const auto &subs = context.d_substitutions;
                handle = op.template eval<Store>([&subs] (auto sub)
                {
                    return subs[sub].eval();
                });
            }

            else if constexpr (std::is_same_v<Op, op::Extension>)
            {
                const auto &pool = d_data.extensions;
                handle = op.template eval<Store>([this, &pool, &context] (auto fun, auto begin, auto end)
                {
                    return pool.template invoke<Store>(
                        fun,
                        [this, &context] (op::Id ref) -> const Result<Store> &
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
                handle = op.template eval<Store>([this, &context] (auto ref) mutable -> const Result<Store> &
                {
                    this->eval(ref, context);
                    return context.d_results[ref];
                });
            }
        },
        d_data.ops[id]);
}

template<typename Store, typename... Funs>
const Ingredients<Store, Funs...> &Expression<Store, Funs...>::ingredients() const
{
    return d_data;
}


template<typename Store, typename... Funs>
template<typename Substitute>
Context<Store, Substitute> Expression<Store, Funs...>::context(bool cache /*= true */) const
{
    return Context<Store, Substitute> {d_data.ops, cache};
}

template<typename Store, typename... Funs>
template<typename Substitute>
Result<Store> Expression<Store, Funs...>::operator()(Context<Store, Substitute> &context) const
{
    const auto root = d_data.ops.size() - 1;

    eval(root, context);
    return context.d_results[root];
}

template<typename Store, typename... Funs>
template<typename Substitute>
std::ostream &Expression<Store, Funs...>::log(std::ostream &os, const Context<Store, Substitute> &context) const
{
    size_t id = 0;
    auto op = d_data.ops.begin();
    auto rs = context.d_results.begin();

    for (; op != d_data.ops.end() && rs != context.d_results.end(); ++op, ++rs)
    {
        os << "\t#" << id++ << ": " << *op << " = " << *rs << "\n";
    }

    return os;
}

template<typename Store, typename... Funs>
std::ostream &operator<<(std::ostream &os, const Expression<Store, Funs...> &expression)
{
    //store(expression)
    //    .map([&os] (const std::string &stored) mutable
    //    {
    //        os << stored;
    //    })
    //    .map_error([&os] (const err::Error &e) mutable
    //    {
    //        os << e;
    //    });

    return os;
}


} // namespace pmql
