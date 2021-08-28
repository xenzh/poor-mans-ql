#pragma once

#include "error.h"

#include <tuple>
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

    template<typename F, typename T>
    static void last(F &&callback, T &&arg)
    {
        callback(std::forward<T>(arg));
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

    template<typename F, typename T, typename... Ts>
    static void last(F &&callback, T &&, Ts &&...rest)
    {
        get<Args...>::last(std::forward<F>(callback), std::forward<Ts>(rest)...);
    }
};


template<size_t... Pos> struct nargs
{
    template<typename F, typename... Ts, typename... Idx, typename... Args>
    static void collect(F &&callback, std::tuple<Ts...> to, const std::tuple<Idx...> &indices, Args &&...args)
    {
        std::apply(std::forward<F>(callback), std::move(to));
    }
};

//template<size_t Pos> struct nargs<Pos>
//{
//    template<typename F, typename... Ts, typename... Idx, typename... Args>
//    static void collect(F &&callback, std::tuple<Ts...> to, const std::tuple<Idx...> &indices, Args &&...args)
//    {
//        get<Args...>::nth(
//            [to = std::move(to), cb = std::forward<F>(callback)] (auto &&arg)
//            {
//                std::apply(cb, std::tuple_cat(std::move(to), std::forward_as_tuple(arg)));
//            },
//            std::get<Pos>(indices),
//            std::forward<Args>(args)...
//        );
//    }
//};

template<size_t Cur, size_t... Rest> struct nargs<Cur, Rest...>
{
    template<typename F, typename... Ts, typename... Idx, typename... Args>
    static void collect(F &&callback, std::tuple<Ts...> to, const std::tuple<Idx...> &indices, Args &&...args)
    {
        get<Args...>::nth(
            [
                to = std::move(to),
                cb = std::forward<F>(callback),
                &indices,
                args = std::forward_as_tuple(args...)
            ]
            (auto &&arg) mutable
            {
                std::apply(
                    [&] (auto &&...args)
                    {
                        nargs<Rest...>::collect(
                            std::forward<F>(cb),
                            std::tuple_cat(std::move(to), std::forward_as_tuple(arg)),
                            indices,
                            std::forward<decltype(args)>(args)...
                        );
                    },
                    std::move(args));
            },
            std::get<Cur>(indices),
            std::forward<Args>(args)...);
    };
};


struct nargs_collector
{
    template<typename F, size_t... Pos, typename... Idx, typename... Args>
    auto operator()(F &&callback, std::index_sequence<Pos...>, const std::tuple<Idx...> &indices, Args &&...args) const
    {
        nargs<Pos...>::collect(std::forward<F>(callback), std::make_tuple(), indices, std::forward<Args>(args)...);
    }
};


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
            transform(*iter)
        );
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


/// Takes an iterable and calls provided callable with each element as an argument.
template<size_t Limit = 10, typename F, typename Coll, typename Transform = AsIs>
decltype(auto) apply_iterable(F &&callable, Coll &&iterable, Transform &&transform = Transform {})
{
    return apply_iterable_impl<Limit>(
        std::forward<F>(callable),
        std::begin(iterable),
        std::end(iterable),
        std::forward<Transform>(transform)
    );
}


/// Calls provided callable with n-th argument from a pack.
template<typename F, typename... Args>
void nth(F &&callback, size_t n, Args &&...args)
{
    get<Args...>::nth(std::forward<F>(callback), n, std::forward<Args>(args)...);
}


/// Calls provided callable with arguments from pack, taken from collection of indices.
template<typename F, typename Iterable, typename... Args>
void collect_apply_iterable(F &&callback, const Iterable &indices, Args &&...args)
{
    apply_iterable(
        [&callback, args = std::forward_as_tuple(args...)] (auto &&...indices)
        {
            std::apply(nargs_collector {}, std::tuple_cat(
                std::forward_as_tuple(callback),
                std::make_tuple(std::index_sequence_for<decltype(indices)...> {}),
                std::make_tuple(std::forward_as_tuple(indices...)),
                std::move(args)
            ));
        },
        indices);
}


/// Calls provided callback with the last member of a pack.
template<typename F, typename... Args>
void last(F &&callback, Args &&...args)
{
    get<Args...>::last(std::forward<F>(callback), std::forward<Args>(args)...);
}


} // namespace util
