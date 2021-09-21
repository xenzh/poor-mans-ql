#pragma once

#include "null.h"

#include <variant>
#include <ostream>
#include <string_view>


namespace pmql {


/// Container for storing a single nullable type that implements Store and Substitute contracts.
/// @tparam Name template that provides a name for V type (as a static std::string_view value member).
/// @tparam V value type to store.
template<template<typename> typename Name, typename V>
class Single
{
    /// Streaming support.
    template<template<typename> typename N, typename T>
    friend std::ostream &operator<<(std::ostream &, const Single<N, T> &);

    /// True if stored value is a null.
    bool d_null = true;

    /// Stored value.
    V d_value;

public:
    /// Returns a name for any supported stored type.
    /// @tparam T any supported stored type.
    /// @return type name string.
    template<typename T>
    static std::string_view name();

    /// Constructs a container instance that stored a null value.
    Single() = default;

    /// Initializes container with a non-null stored value from an lvalue reference.
    /// @param value value reference.
    explicit Single(const V &value);

    /// Initializes container with a non-null stored value from an rvalue reference.
    /// @param value value reference.
    explicit Single(V &&value);

    /// Updates stored value. Part of the Substitute contract.
    /// @tparam T type, convertible to V.
    /// @param value new value to store.
    /// @return reference to updated self.
    template<typename T>
    Single<Name, V> &operator=(T &&value);

    /// Converts to true if stored value is not null.
    operator bool() const;

    /// Calls provided visitor with stored type or null  literal.
    /// @tparam Visitor callable, T(const V &).
    /// @param visitor callback to call with stored value.
    /// @return whatever the visitor casllback returns.
    template<typename Visitor>
    auto operator()(Visitor &&visitor) const -> decltype(auto);
};


/// Container for storing any of provided types that implements Store and Substitute contracts.
/// @tparam Name template that defines names for all supported types.
/// @tparam Vs pack of supported type variants.
template<template<typename> typename Name, typename... Vs>
class Variant
{
    /// Streaming support.
    template<template<typename> typename N, typename... Ts>
    friend std::ostream &operator<<(std::ostream &, const Variant<N, Ts...> &);

    /// Underlying variant type.
    using Value = std::variant<null, Vs...>;

    /// Checks if underlying variant can be constructed from type T.
    template<typename T, typename = void> struct allowed : std::false_type {};
    template<typename T> struct allowed<
        T,
        std::void_t<decltype(Value {std::declval<T>()})>> : std::true_type {};

    /// Variable template for checking if underlying variant can be constructed from type T.
    template<typename T>
    static constexpr bool allowed_v = allowed<T>::value;

    /// Checks if type T is the same as this container type.
    template<typename T>
    static constexpr bool is_self_v = std::is_same_v<std::decay_t<T>, Variant<Name, Vs...>>;

    /// Stored value.
    Value d_value;

public:
    /// Returns a name for any supported stored type.
    /// @tparam T any supported stored type.
    /// @return type name string.
    template<typename T>
    static std::string_view name();

    /// Constructs a container instance that stored a null value.
    Variant() = default;

    /// A non-copy/move ctor that initializes the container with compatible non-null value.
    /// @tparam T type, convertible to any of allowed types from Vs.
    /// @param value value to store.
    template<typename T, typename = std::enable_if_t<!is_self_v<T> && allowed_v<T>>>
    explicit Variant(T &&value);

    /// An inplace ctor for any allowed type.
    /// @tparam T an allowed type to store.
    /// @tparam Args T constructor argument type pack.
    /// @param args T constructor arguments.
    template<typename T, typename... Args>
    explicit Variant(std::in_place_type_t<T>, Args &&...args);

    /// Updates stored value. Part of the Substitute contract.
    /// @tparam V type, convertible to any of the stored types.
    /// @param value new value to store.
    /// @return reference to updated self.
    template<typename V>
    Variant<Name, Vs...> &operator=(V &&value);

    /// Converts to true if stored value is not null.
    operator bool() const;

    /// Calls provided visitor with stored type or null  literal.
    /// @tparam Visitor callable, T(const V &).
    /// @param visitor callback to call with stored value.
    /// @return whatever the visitor casllback returns.
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
