#pragma once

#include "error.h"
#include "ops.h"

#include <optional>
#include <vector>
#include <unordered_map>

#include <ostream>
#include <string_view>

#include <numeric>


namespace pmql {


class Variable
{
    friend std::ostream &operator<<(std::ostream &, const Variable &);

    const op::Id d_id;
    const std::string_view d_name;

public:
    Variable(op::Id id, std::string_view name);

    op::Id id() const;

    std::string_view name() const;
};


template<typename Substitute>
class Substitution : public Variable
{
    template<typename Sub>
    friend std::ostream &operator<<(std::ostream &, const Substitution<Sub> &);

    std::optional<Substitute> d_substitute;

public:
    Substitution(op::Id id, std::string_view name, std::optional<Substitute> &&sub = std::nullopt);

    operator bool() const;

    template<typename Sub>
    Substitution &operator=(Sub &&substitute);

    template<typename Store>
    Result<Store> eval() const;
};


template<typename Store, typename Substitute>
class Context
{
    template<typename St> friend class Expression;

    template<typename St, typename Sub>
    friend std::ostream &operator<<(std::ostream &, const Context<St, Sub> &);

    using Subst  = Substitution<Substitute>;
    using Substs = std::vector<Subst>;

    Substs d_substitutions;
    std::unordered_map<std::string_view, op::Id> d_byname;

    std::vector<Result<Store>> d_results;

public:
    using iterator = typename Substs::iterator;

    using const_iterator = typename Substs::const_iterator;

    using ref = std::reference_wrapper<Subst>;

    using cref = std::reference_wrapper<const Subst>;

    explicit Context(const op::List &ops);

    operator bool() const;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    iterator find(std::string_view name);
    const_iterator find(std::string_view name) const;

    Subst &operator[](op::Id id);
    const Subst &operator[](op::Id id) const;

    Result<ref> operator()(std::string_view name);
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


template<typename Substitute>
Substitution<Substitute>::Substitution(
    op::Id id,
    std::string_view name,
    std::optional<Substitute> &&sub /*= std::nullopt*/)
    : Variable(id, name)
    , d_substitute(std::move(sub))
{
}

template<typename Substitute>
Substitution<Substitute>::operator bool() const
{
    return bool(d_substitute);
}

template<typename Substitute>
template<typename Sub>
Substitution<Substitute> &Substitution<Substitute>::operator=(Sub &&substitute)
{
    d_substitute = std::forward<Sub>(substitute);
    return *this;
}

template<typename Substitute>
template<typename Store>
Result<Store> Substitution<Substitute>::eval() const
{
    if (!d_substitute)
    {
        return err::error<err::Kind::EXPR_BAD_SUBST>(name());
    }

    return (*d_substitute)([] (const auto &value) { return Store {value}; });
}

template<typename Substitute>
std::ostream &operator<<(std::ostream &os, const Substitution<Substitute> &subst)
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
Context<Store, Substitute>::Context(const op::List &ops)
{
    d_results.reserve(ops.size());
    op::Id current = 0;

    for (const auto &op : ops)
    {
        d_results.emplace_back(err::error<err::Kind::EXPR_NOT_READY>());

        std::visit(
            [this, &current] (const auto &op) mutable
            {
                using Op = std::decay_t<decltype(op)>;

                if constexpr (std::is_same_v<Op, op::Var>)
                {
                    d_byname[op.name()] = d_substitutions.size();
                    d_substitutions.emplace_back(current, op.name());
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
        return err::error<err::Kind::CONTEXT_BAD_VARIABLE>(name);
    }

    return std::ref(*it);
}


template<typename Store, typename Substitute>
Result<typename Context<Store, Substitute>::cref> Context<Store, Substitute>::operator()(std::string_view name) const
{
    auto it = find(name);
    if (it == d_substitutions.end())
    {
        return err::error<err::Kind::CONTEXT_BAD_VARIABLE>(name);
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
