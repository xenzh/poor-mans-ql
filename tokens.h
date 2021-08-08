#pragma once

#include <type_traits>
#include <functional>
#include <optional>
#include <variant>
#include <string>


namespace eval {


// auto vars = expression.variables();
//
// auto a = vars["a"];
// auto b = vars["b"];
//
// std::vector<std::pair<size_t, std::reference_wrapper<Variable>>> inputvars;
// for (const auto &var : vars)
// {
//     const size_t index = input->inputFieldIndex(var.name());
//     inputvars.emplace_back(index, std::ref(var));
// }
//
// typecheck?
//
// input->accept([&] (const auto &row)
// {
//     auto binder = expression.bind();
//     for (const auto &[idx, var] : inputvars)
//     {
//         binder << 
//         var << Substitute(row[idx]); // via a visitor, for any type
//
//         // or
//
//         eval::bind v1(variable, value); // but in the parent scope somehow!
//     }
//
//     expression([&] (auto &&result) { output->append(result.value_or(decltype(result) {}), result.has_value()) });
// });


class Variable;


template<typename V>
class Substitute
{
    friend class Variable;

    void bind(Variable &var);

public:
    using type = std::optional<std::reference_wrapper<const V>>;

    Substitute() = default;

    explicit Substitute(const V &value);

    Substitute(const Substitute &&) = delete;

    ~Substitute();

    operator bool() const;

    const type &operator*() const;

private:
    type d_value;

    std::optional<std::reference_wrapper<Variable>> d_var;
};


/// This is a stub to be substituted with an actual implementation with supported type list.
template<template<typename...> typename T, template<typename> typename Wrap = std::common_type>
using expand_types_t = T<typename Wrap<int>::type, typename Wrap<double>::type>;


class Variable
{
    using type = expand_types_t<std::variant, Substitute>;

    std::string d_name;
    type d_substitution;

public:
    explicit Variable(std::string_view name);

    operator bool() const;

    const type &operator*() const;

    template<typename V>
    Variable &operator<<(const Substitute<V> &substitution);

    void unset() const;
};



template<typename V>
void Substitute<V>::bind(Variable &var)
{
    d_var = std::ref(var);
}

template<typename V>
Substitute<V>::Substitute(const V &value)
    : d_value(std::ref(value))
{
}

template<typename V>
Substitute<V>::~Substitute()
{
    if (d_var)
    {
        d_var->get().unset();
    }
}

template<typename V>
Substitute<V>::operator bool() const
{
    return d_value.has_value();
}

template<typename V>
const typename Substitute<V>::type &Substitute<V>::operator*() const
{
    return d_value;
}


} // namespace eval
