#pragma once

#include "error.h"
#include "ops.h"
#include "results.h"

#include <optional>
#include <vector>
#include <unordered_map>

#include <ostream>
#include <string_view>

#include <numeric>


namespace pmql {


/// Client-facing interface that describes an expression variable.
class Variable
{
    /// Streaming support.
    friend std::ostream &operator<<(std::ostream &, const Variable &);

    /// Variable identifier inside Expression's operator list.
    const op::Id d_id;

    /// Variable name.
    const std::string_view d_name;

public:
    /// Construct variable interface.
    /// @param id Expression's operation identifier.
    /// @param name variable name.
    Variable(op::Id id, std::string_view name);

    /// Return Expression's operation identifier.
    /// @return operation identifier.
    op::Id id() const;

    /// Return function name.
    /// @return function name.
    std::string_view name() const;
};


/// Client-facing proxy for variable substitution.
/// Substitute contract:
/// @code{.cpp}
/// struct SampleSubstitute
/// {
///     template<typename T>
///     SampleSubstitute &operator=(T &&value)
///     {
///          // sets variable to value.
///     }
///
///     template<typename Visitor>
///     auto operator()(Visitor &&visitor) const -> decltype(auto)
///     {
///         // calls visitor with current variable value or null [}, returns callback's result.
///     }
/// };
/// @endcode
/// @tparam Store type that can store a calculation result (see Store contract).
/// @tparam Substitute object that can set and store variable value.
template<typename Store, typename Substitute>
class Substitution : public Variable
{
    /// Streaming support.
    template<typename St, typename Sub>
    friend std::ostream &operator<<(std::ostream &, const Substitution<St, Sub> &);

    /// Variable index in parent context.
    const size_t d_index;

    /// Reference to parent context's evaluation results storage (for invalidation).
    op::Results<Store> &d_results;

    /// Optional substitution object.
    std::optional<Substitute> d_substitute;

public:
    /// Construct substitution proxy instance.
    /// @param id host Expression's operation identifier.
    /// @param name variable name.
    /// @param index associated variant index from parent context.
    /// @param results reference to expression results cache.
    /// @param sub substitution object.
    Substitution(
        op::Id id,
        std::string_view name,
        size_t index,
        op::Results<Store> &results,
        std::optional<Substitute> &&sub = std::nullopt);

    /// Converts to true if substitution object has been set.
    operator bool() const;

    /// Sets a substitution value.
    /// @tparam Sub substitution value type compatible with Substitute.
    /// @param substitute new substitution object.
    /// @return reference to updated self.
    template<typename Sub>
    Substitution &operator=(Sub &&substitute);

    /// Reads variable substitution value and stores it in provided Store type.
    /// @return stored variable value or an error (if the value has not been set).
    Result<Store> eval() const;
};


/// Expression evaluation context.
/// Contains all "moving parts" of the evaluation process,
/// including variables and calculation results for each step.
/// @param Store type that can store a calculation result (see Store contract).
/// @tparam Substitute type that can set and store variable value (see Substitute contract).
template<typename Store, typename Substitute>
class Context
{
    /// Allows host Expression object to access context state.
    template<typename St, typename... Fs> friend class Expression;

    /// Streaming support.
    template<typename St, typename Sub>
    friend std::ostream &operator<<(std::ostream &, const Context<St, Sub> &);

    /// Client-facing variable substitution proxy type.
    using Subst  = Substitution<Store, Substitute>;

    /// List of variable substitutions.
    using Substs = std::vector<Subst>;

    /// Variable substitution.
    Substs d_substitutions;

    /// Variable substitution index.
    std::unordered_map<std::string_view, op::Id> d_byname;

    /// Expression's step-by-step operation evaluation results.
    op::Results<Store> d_results;

public:
    /// Defines modifiable substitutions iterator type.
    using iterator = typename Substs::iterator;

    /// Defines substitutions iterator type.
    using const_iterator = typename Substs::const_iterator;

    /// Defines a modifiable substitution proxy reference.
    using ref = std::reference_wrapper<Subst>;

    /// Defines a constant substitution proxy reference.
    using cref = std::reference_wrapper<const Subst>;

    /// Construct context instance from operation list.
    /// @param ops valid list of operations.
    /// @param cache if set to true, operation result caching is enabled.
    explicit Context(const op::List &ops, bool cache = true);

    /// Converts to true if all variable substitutions are set.
    operator bool() const;

    /// Returns the start of variable substitution range.
    /// @eturn modifiable substitution range iterator.
    iterator begin();

    /// Returns the end of variable substitution range.
    /// @eturn modifiable substitution range iterator.
    iterator end();

    /// Returns the start of variable substitution range.
    /// @eturn const variable range iterator.
    const_iterator begin() const;

    /// Returns the end of variable substitution range.
    /// @eturn const variable range iterator.
    const_iterator end() const;

    /// Looks a variable substitution up by variable name.
    /// @param name variable name.
    /// @return modifiable substitution iterator (or end, of not found).
    iterator find(std::string_view name);

    /// Looks a variable substitution up by variable name.
    /// @param name variable name.
    /// @return const variable iterator (or end, of not found).
    const_iterator find(std::string_view name) const;

    /// Gets substitution reference by operation identifier.
    /// Proving an out-of-range identifier is considered undefined behaviour.
    /// @param id valid operation id.
    /// @return modifiable substitution reference.
    Subst &operator[](op::Id id);

    /// Gets substitution reference by operation identifier.
    /// Proving an out-of-range identifier is considered undefined behaviour.
    /// @param id valid operation id.
    /// @return const variable reference.
    const Subst &operator[](op::Id id) const;

    /// Looks up variable substitution by name.
    /// @param name variable name.
    /// @return modifiable variable substitution reference or an error.
    Result<ref> operator()(std::string_view name);

    /// Looks up variable substitution by name.
    /// @param name variable name.
    /// @return const variable reference or an error.
    Result<cref> operator()(std::string_view name) const;
};



inline Variable::Variable(op::Id id, std::string_view name)
    : d_id(id)
    , d_name(name)
{
}

inline op::Id Variable::id() const
{
    return d_id;
}

inline std::string_view Variable::name() const
{
    return d_name;
}

inline std::ostream &operator<<(std::ostream &os, const Variable &var)
{
    return os << "$" << var.d_name;
}


template<typename Store, typename Substitute>
Substitution<Store, Substitute>::Substitution(
    op::Id id,
    std::string_view name,
    size_t index,
    op::Results<Store> &results,
    std::optional<Substitute> &&sub /*= std::nullopt*/)
    : Variable(id, name)
    , d_index(index)
    , d_results(results)
    , d_substitute(std::move(sub))
{
}

template<typename Store, typename Substitute>
Substitution<Store, Substitute>::operator bool() const
{
    return bool(d_substitute);
}

template<typename Store, typename Substitute>
template<typename Sub>
Substitution<Store, Substitute> &Substitution<Store, Substitute>::operator=(Sub &&substitute)
{
    d_substitute = std::forward<Sub>(substitute);
    d_results.invalidate(d_index);

    return *this;
}

template<typename Store, typename Substitute>
Result<Store> Substitution<Store, Substitute>::eval() const
{
    if (!d_substitute)
    {
        return error<err::Kind::EXPR_BAD_SUBST>(name());
    }

    return (*d_substitute)([] (const auto &value) { return Store {value}; });
}

template<typename Store, typename Substitute>
std::ostream &operator<<(std::ostream &os, const Substitution<Store, Substitute> &subst)
{
    if (!subst.d_substitute)
    {
        return os << "<empty>";
    }

    if constexpr (op::detail::Ostreamable<Substitute>::value)
    {
        os << *subst.d_substitute;
    }
    else
    {
        os << "<ready>";
    }

    return os;
}


template<typename Store, typename Substitute>
Context<Store, Substitute>::Context(const op::List &ops, bool cache /*= true */)
    : d_results(ops, cache)
{
    op::Id current = 0;
    size_t var = 0;

    for (const auto &op : ops)
    {
        std::visit(
            [this, &current, &var] (const auto &op) mutable
            {
                using Op = std::decay_t<decltype(op)>;

                if constexpr (std::is_same_v<Op, op::Var>)
                {
                    d_byname[op.name()] = d_substitutions.size();
                    d_substitutions.emplace_back(current, op.name(), var++, d_results);
                }
            },
            op);

        ++current;
    }
}

template<typename Store, typename Substitute>
Context<Store, Substitute>::operator bool() const
{
    return std::accumulate(
        d_substitutions.begin(),
        d_substitutions.end(),
        true,
        [] (auto valid, const auto &subst) { return valid && bool(subst); });
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::iterator Context<Store, Substitute>::begin()
{
    return d_substitutions.begin();
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::iterator Context<Store, Substitute>::end()
{
    return d_substitutions.end();
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::const_iterator Context<Store, Substitute>::begin() const
{
    return d_substitutions.begin();
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::const_iterator Context<Store, Substitute>::end() const
{
    return d_substitutions.end();
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::iterator Context<Store, Substitute>::find(std::string_view name)
{
    auto it = d_byname.find(name);
    return it == d_byname.end() ? d_substitutions.end() : (d_substitutions.begin() + it->second);
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::const_iterator Context<Store, Substitute>::find(std::string_view name) const
{
    auto it = d_byname.find(name);
    return it == d_byname.end() ? d_substitutions.end() : (d_substitutions.begin() + it->second);
}

template<typename Store, typename Substitute>
typename Context<Store, Substitute>::Subst &Context<Store, Substitute>::operator[](op::Id id)
{
    return d_substitutions[id];
}

template<typename Store, typename Substitute>
const typename Context<Store, Substitute>::Subst &Context<Store, Substitute>::operator[](op::Id id) const
{
    return d_substitutions[id];
}

template<typename Store, typename Substitute>
Result<typename Context<Store, Substitute>::ref> Context<Store, Substitute>::operator()(std::string_view name)
{
    auto it = find(name);
    if (it == d_substitutions.end())
    {
        return error<err::Kind::CONTEXT_BAD_VARIABLE>(name);
    }

    return std::ref(*it);
}


template<typename Store, typename Substitute>
Result<typename Context<Store, Substitute>::cref> Context<Store, Substitute>::operator()(std::string_view name) const
{
    auto it = find(name);
    if (it == d_substitutions.end())
    {
        return error<err::Kind::CONTEXT_BAD_VARIABLE>(name);
    }

    return std::cref(*it);
}

template<typename Store, typename Substitute>
std::ostream &operator<<(std::ostream &os, const Context<Store, Substitute> &context)
{
    os << "Substitutions:\n";
    for (const auto &sub : context.d_substitutions)
    {
        os << "\t$" << sub.name() << ": " << sub << "\n";
    }

    os << "\nEvaluations:\n";
    size_t id = 0;
    for (const auto &result : context.d_results)
    {
        os << "\t#" << id++ << ": " << result << "\n";
    }

    return os;
}


} // namespace pmql
