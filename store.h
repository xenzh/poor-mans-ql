#pragma once

#include <variant>
#include <string_view>


namespace pmql {


template<template<typename> typename Name, typename... Vs>
class Store
{
    template<template<typename> typename N, typename... Ts>
    friend std::ostream &operator<<(std::ostream &, const Store<N, Ts...> &);

    std::variant<Vs...> d_value;

public:
    template<typename T>
    static std::string_view name();

    template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Store<Name, Vs...>>>>
    explicit Store(T &&value);

    template<typename T, typename... Args>
    explicit Store(std::in_place_type_t<T>, Args &&...args);

    template<typename V>
    Store<Name, Vs...> &operator=(V &&value);

    template<typename Visitor>
    auto operator()(Visitor &&visitor) const -> decltype(auto);
};



template<template<typename> typename Name, typename... Vs>
template<typename T>
/* static */ std::string_view Store<Name, Vs...>::name()
{
    return Name<T>::value;
}

template<template<typename> typename Name, typename... Vs>
template<typename T, typename>
Store<Name, Vs...>::Store(T &&value)
    : d_value {std::forward<T>(value)}
{
}

template<template<typename> typename Name, typename... Vs>
template<typename T, typename... Args>
Store<Name, Vs...>::Store(std::in_place_type_t<T> type, Args &&...args)
    : d_value {type, std::forward<Args>(args)...}
{
}

template<template<typename> typename Name, typename... Vs>
template<typename V>
Store<Name, Vs...> &Store<Name, Vs...>::operator=(V &&value)
{
    d_value = std::forward<V>(value);
    return *this;
}

template<template<typename> typename Name, typename... Vs>
template<typename Visitor>
auto Store<Name, Vs...>::operator()(Visitor &&visitor) const -> decltype(auto)
{
    return std::visit(
        [&visitor] (const auto &value) { return visitor(value); },
        d_value);
}

template<template<typename> typename Name, typename... Vs>
std::ostream &operator<<(std::ostream &os, const Store<Name, Vs...> &store)
{
    return std::visit(
        [&os] (const auto &value) mutable -> std::ostream &
        {
            return os << Name<std::decay_t<decltype(value)>>::value << "(" << value << ")";
        },
        store.d_value);
}


} // namespace pmql
