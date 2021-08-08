#pragma once

#include "error.h"

#include <type_traits>
#include <iterator>
#include <functional>
#include <utility>
#include <optional>


namespace util {


template<typename... Args> struct get;

template<typename Arg> struct get<Arg>
{
    template<typename F, typename T>
    static void nth(F &&callback, size_t n, T &&arg)
    {
        if (n == 0)
        {
            callback(arg);
        }
        else
        {
            callback(Result<void>(err::error<err::Kind::OPS_NOT_FOUND>(n)));
        }
    }
};

template<typename Arg, typename... Args> struct get<Arg, Args...>
{
    template<typename F, typename T, typename... Ts>
    static void nth(F &&callback, size_t n, T &&arg, Ts &&...rest)
    {
        if (n == 0)
        {
            callback(arg);
        }
        else
        {
            get<Args...>::nth(std::forward<F>(callback), n - 1, std::forward<Ts>(rest)...);
        }
    }
};

template<typename F, typename... Args>
void nth(F &&callback, size_t n, Args &&...args)
{
    get<Args...>::nth(std::forward<F>(callback), n, std::forward<Args>(args)...);
}


struct AsIs { template<typename T> decltype(auto) operator()(T &&value) { return value; } };


template<size_t Limit, typename F, typename It, typename Transform, typename... Args>
decltype(auto) apply_iterable_impl(F &&callable, const It &iter, const It &end, Transform &&transform, Args &&...args)
{
    static_assert(std::is_invocable_v<F, Args...>);
    using return_t = std::invoke_result_t<F, Args...>;

    if (iter == end)
    {
        if constexpr (std::is_same_v<void, return_t>)
        {
            callable(std::forward<Args>(args)...);
            return;
        }
        else
        {
            return callable(std::forward<Args>(args)...);
        }
    }

    if constexpr (Limit != 0)
    {
        return apply_iterable_impl<Limit - 1>(
            std::forward<F>(callable),
            std::next(iter),
            end,
            std::forward<Transform>(transform),
            std::forward<Args>(args)...,
            transform(*iter));
    }
    else
    {
        if constexpr (std::is_same_v<void, return_t>)
        {
            return;
        }
        else
        {
            return return_t {};
        }
    }
}

template<size_t Limit = 10, typename F, typename Coll, typename Transform = AsIs>
decltype(auto) apply_iterable(F &&callable, Coll &&iterable, Transform &&transform = Transform {})
{
    return apply_iterable_impl<Limit>(
        std::forward<F>(callable),
        std::begin(iterable),
        std::end(iterable),
        std::forward<Transform>(transform));
}



} // namespace util
