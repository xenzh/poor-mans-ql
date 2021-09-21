#pragma once

#include "null.h"

#include <variant>
#include <ostream>
#include <string_view>


namespace pmql {


template<template<typename> typename Name, typename V>
class Single
{
    template<template<typename> typename N, typename T>
    friend std::ostream &operator<<(std::ostream &, const Single<N, T> &);

    bool d_null = true;
    V d_value;

public:
    template<typename T>
    static std::string_view name();

    Single() = default;

    explicit Single(const V &value);
    explicit Single(V &&value);

    template<typename T>
    Single<Name, V> &operator=(T &&value);

    operator bool() const;

    template<typename Visitor>
    auto operator()(Visitor &&visitor) const -> decltype(auto);
};


template<template<typename> typename Name, typename... Vs>
class Variant
{
    template<template<typename> typename N, typename... Ts>
    friend std::ostream &operator<<(std::ostream &, const Variant<N, Ts...> &);

    using Value = std::variant<null, Vs...>;

    template<typename T, typename = void> struct allowed : std::false_type {};
    template<typename T> struct allowed<
        T,
        std::void_t<decltype(Value {std::declval<T>()})>> : std::true_type {};

    template<typename T>
    static constexpr bool allowed_v = allowed<T>::value;

    template<typename T>
    static constexpr bool is_self_v = std::is_same_v<std::decay_t<T>, Variant<Name, Vs...>>;

    Value d_value;

public:
    template<typename T>
    static std::string_view name();

    Variant() = default;

    template<typename T, typename = std::enable_if_t<!is_self_v<T> && allowed_v<T>>>
    explicit Variant(T &&value);

    template<typename T, typename... Args>
    explicit Variant(std::in_place_type_t<T>, Args &&...args);

    template<typename V>
    Variant<Name, Vs...> &operator=(V &&value);

    operator bool() const;

    template<typename Visitor>
    auto operator()(Visitor &&visitor) const -> decltype(auto);
};



template<template<typename> typename Name, typename V>
template<typename T>
/* static */ std::string_view Single<Name, V>::name()
{
    if constexpr (std::is_same_v<T, null>)
    {
        return "<null>";
    }
    else
    {
        return Name<T>::value;
    }
}

template<template<typename> typename Name, typename V>
Single<Name, V>::Single(const V &value)
    : d_null(false)
    , d_value(value)
{
}

template<template<typename> typename Name, typename V>
Single<Name, V>::Single(V &&value)
    : d_null(false)
    , d_value(std::move(value))
{
}

template<template<typename> typename Name, typename V>
template<typename T>
Single<Name, V> &Single<Name, V>::operator=(T &&value)
{
    d_value = std::forward<T>(value);
    return *this;
}

template<template<typename> typename Name, typename V>
Single<Name, V>::operator bool() const
{
    return !d_null;
}

template<template<typename> typename Name, typename V>
template<typename Visitor>
auto Single<Name, V>::operator()(Visitor &&visitor) const -> decltype(auto)
{
    if (d_null)
    {
        return visitor(null {});
    }

    return visitor(d_value);
}

template<template<typename> typename Name, typename V>
std::ostream &operator<<(std::ostream &os, const Single<Name, V> &single)
{
    if (!single)
    {
        return os << null {};
    }

    return os << Name<V>::value << "(" << single.d_value << ")";
}


template<template<typename> typename Name, typename... Vs>
template<typename T>
/* static */ std::string_view Variant<Name, Vs...>::name()
{
    if constexpr (std::is_same_v<T, null>)
    {
        return "null";
    }
    else
    {
        return Name<T>::value;
    }
}

template<template<typename> typename Name, typename... Vs>
template<typename T, typename>
Variant<Name, Vs...>::Variant(T &&value)
    : d_value {std::forward<T>(value)}
{
}

template<template<typename> typename Name, typename... Vs>
template<typename T, typename... Args>
Variant<Name, Vs...>::Variant(std::in_place_type_t<T> type, Args &&...args)
    : d_value {type, std::forward<Args>(args)...}
{
}

template<template<typename> typename Name, typename... Vs>
template<typename V>
Variant<Name, Vs...> &Variant<Name, Vs...>::operator=(V &&value)
{
    d_value = std::forward<V>(value);
    return *this;
}

template<template<typename> typename Name, typename... Vs>
Variant<Name, Vs...>::operator bool() const
{
    return !std::holds_alternative<null>(d_value);
}

template<template<typename> typename Name, typename... Vs>
template<typename Visitor>
auto Variant<Name, Vs...>::operator()(Visitor &&visitor) const -> decltype(auto)
{
    return std::visit(
        [&visitor] (const auto &value) { return visitor(value); },
        d_value);
}

template<template<typename> typename Name, typename... Vs>
std::ostream &operator<<(std::ostream &os, const Variant<Name, Vs...> &store)
{
    return std::visit(
        [&os] (const auto &value) mutable -> std::ostream &
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, null>)
            {
                return os << value;
            }
            else
            {
                return os << Name<std::decay_t<decltype(value)>>::value << "(" << value << ")";
            }
        },
        store.d_value);
}


} // namespace pmql
