#pragma once

#include <variant>
#include <string_view>


namespace pmql {


template<template<typename> typename Name, typename V>
class Single
{
    template<template<typename> typename N, typename T>
    friend std::ostream &operator<<(std::ostream &, const Single<N, T> &);

    V d_value;

public:
    template<typename T>
    static std::string_view name();

    explicit Single(const V &value);
    explicit Single(V &&value);

    template<typename T>
    Single<Name, V> &operator=(T &&value);

    template<typename Visitor>
    auto operator()(Visitor &&visitor) const -> decltype(auto);
};


template<template<typename> typename Name, typename... Vs>
class Variant
{
    template<template<typename> typename N, typename... Ts>
    friend std::ostream &operator<<(std::ostream &, const Variant<N, Ts...> &);

    template<typename T, typename = void> struct allowed : std::false_type {};
    template<typename T> struct allowed<
        T,
        std::void_t<decltype(std::variant<Vs...> {std::declval<T>()})>> : std::true_type {};

    template<typename T>
    static constexpr bool allowed_v = allowed<T>::value;

    template<typename T>
    static constexpr bool is_self_v = std::is_same_v<std::decay_t<T>, Variant<Name, Vs...>>;

    std::variant<Vs...> d_value;

public:
    template<typename T>
    static std::string_view name();

    template<typename T, typename = std::enable_if_t<!is_self_v<T> && allowed_v<T>>>
    explicit Variant(T &&value);

    template<typename T, typename... Args>
    explicit Variant(std::in_place_type_t<T>, Args &&...args);

    template<typename V>
    Variant<Name, Vs...> &operator=(V &&value);

    template<typename Visitor>
    auto operator()(Visitor &&visitor) const -> decltype(auto);
};



template<template<typename> typename Name, typename V>
template<typename T>
/* static */ std::string_view Single<Name, V>::name()
{
    return Name<T>::value;
}

template<template<typename> typename Name, typename V>
Single<Name, V>::Single(const V &value)
    : d_value(value)
{
}

template<template<typename> typename Name, typename V>
Single<Name, V>::Single(V &&value)
    : d_value(std::move(value))
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
template<typename Visitor>
auto Single<Name, V>::operator()(Visitor &&visitor) const -> decltype(auto)
{
    return visitor(d_value);
}

template<template<typename> typename Name, typename V>
std::ostream &operator<<(std::ostream &os, const Single<Name, V> &single)
{
    return os << Name<V>::value << "(" << single.d_value << ")";
}


template<template<typename> typename Name, typename... Vs>
template<typename T>
/* static */ std::string_view Variant<Name, Vs...>::name()
{
    return Name<T>::value;
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
            return os << Name<std::decay_t<decltype(value)>>::value << "(" << value << ")";
        },
        store.d_value);
}


} // namespace pmql
