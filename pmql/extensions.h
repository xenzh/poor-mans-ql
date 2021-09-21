#pragma once

#include "error.h"

#include <tuple>
#include <unordered_map>
#include <utility>


namespace pmql::ext {


using FunId = size_t;


template<typename... Funs>
class Pool
{
    using List = std::tuple<Funs...>;
    using Map  = std::unordered_map<std::string_view, FunId>;

    template<size_t... Idx> struct Find
    {
        template<typename Store, typename Arg, typename It>
        static Result<Store> invoke(const List &, FunId which, Arg &&, const It &, const It &)
        {
            return error<err::Kind::EXPR_BAD_FUNCTION_ID>(which, std::tuple_size<List>::value - 1);
        }
    };

    template<size_t Idx, size_t... Rest> struct Find<Idx, Rest...>
    {
        template<typename Store, typename Arg, typename It>
        static Result<Store> invoke(const List &funs, FunId which, Arg &&arg, const It &start, const It &end)
        {
            return which == Idx ?
                std::get<Idx>(funs).template eval<Store>(std::forward<Arg>(arg), start, end) :
                Find<Rest...>::template invoke<Store>(funs, which, std::forward<Arg>(arg), start, end);
        }
    };

    template<size_t... Idx>
    static Map index(std::index_sequence<Idx...>);

    template<typename Store, typename Arg, typename It, size_t... Idx>
    Result<Store> invoke(std::index_sequence<Idx...>, FunId which, Arg &&arg, const It &start, const It &end) const;

private:
    List d_functions;
    Map  d_byname;

public:
    using const_iterator = Map::const_iterator;

    template<typename... Fs>
    explicit Pool(Fs &&...functions);

    template<typename... Others>
    Pool<Funs..., Others...> operator+(Pool<Others...> &&other);

    const_iterator begin() const;
    const_iterator end() const;

    Result<FunId> operator[](std::string_view name) const;

    template<typename Store, typename Arg, typename It>
    Result<Store> invoke(FunId fun, Arg &&arg, const It &start, const It &end) const;
};


inline const Pool<> none;


template<typename... Funs>
Pool<Funs...> pool(Funs &&...functions)
{
    return Pool<Funs...> {std::forward<Funs>(functions)...};
}


template<typename... Funs>
template<size_t... Idx>
/* static */ typename Pool<Funs...>::Map Pool<Funs...>::index(std::index_sequence<Idx...>)
{
    return Map {{Funs::name(), Idx}...};
}

template<typename... Funs>
template<typename Store, typename Arg, typename It, size_t... Idx>
Result<Store> Pool<Funs...>::invoke(
    std::index_sequence<Idx...>,
    FunId which,
    Arg &&arg,
    const It &start,
    const It &end) const
{
    return Find<Idx...>::template invoke<Store>(d_functions, which, std::forward<Arg>(arg), start, end);
}

template<typename... Funs>
template<typename... Fs>
Pool<Funs...>::Pool(Fs &&...functions)
    : d_functions {std::forward<Fs>(functions)...}
    , d_byname(index(std::index_sequence_for<Funs...> {}))
{
}

template<typename... Funs>
template<typename... Others>
Pool<Funs..., Others...> Pool<Funs...>::operator+(Pool<Others...> &&other)
{
    d_functions = std::tuple_cat(std::move(d_functions), std::move(other.d_functions));
    d_byname = index(std::index_sequence_for<Funs..., Others...> {});
}

template<typename... Funs>
typename Pool<Funs...>::const_iterator Pool<Funs...>::begin() const
{
    return d_byname.begin();
}

template<typename... Funs>
typename Pool<Funs...>::const_iterator Pool<Funs...>::end() const
{
    return d_byname.end();
}

template<typename... Funs>
Result<FunId> Pool<Funs...>::operator[](std::string_view name) const
{
    auto it = d_byname.find(name);
    if (it == d_byname.end() || it->second >= std::tuple_size<List>::value)
    {
        return error<err::Kind::EXPR_BAD_FUNCTION>(name);
    }

    return it->second;
}

template<typename... Funs>
template<typename Store, typename Arg, typename It>
Result<Store> Pool<Funs...>::invoke(FunId fun, Arg &&arg, const It &start, const It &end) const
{
    return invoke<Store>(std::index_sequence_for<Funs...> {}, fun, std::forward<Arg>(arg), start, end);
}


} // namespace pmql::ext
