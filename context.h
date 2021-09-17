#pragma once

#include "error.h"

#include <optional>
#include <vector>

#include <ostream>
#include <string_view>


namespace pmql {


using Id = size_t;


class Variable
{
    friend std::ostream &operator<<(std::ostream &, const Variable &);

    const Id d_id;
    const std::string_view d_name;

public:
    Variable(Id id, std::string_view name);

    Id id() const;

    std::string_view name() const;
};


template<typename Substitute>
class Substitution : public Variable
{
    template<typename Sub>
    friend std::ostream &operator<<(std::ostream &, const Substitution<Sub> &);

    std::optional<Substitute> d_substitute;

public:
    Substitution(Id id, std::string_view name, std::optional<Substitute> &&sub = std::nullopt);

    template<typename Sub>
    Substitution &operator=(Sub &&substitute);
};


template<typename Storage, typename Substitute>
class Context
{
    template<typename St, typename Sub>
    friend std::ostream &operator<<(std::ostream &, const Context<St, Sub> &);

    using Var  = Substitution<Substitute>;
    using Vars = std::vector<Var>;

    Vars d_substitutions;
    std::vector<Result<Storage>> d_results;

public:
    using iterator = typename Vars::iterator;

    using const_iterator = typename Vars::const_iterator;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    Var &operator[](Id id);
    Var &operator[](std::string_view name);

    const Var &operator[](Id id) const;
    const Var &operator[](std::string_view name) const;
};


} // namespace pmql
