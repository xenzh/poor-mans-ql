#pragma once

#include "error.h"

#include <tuple>
#include <unordered_map>
#include <utility>


namespace pmql::ext {


/// Defines unique extension function identifier type.
using FunId = size_t;


/// Collection of extension functions.
/// Function object contract:
/// @code{.cpp}
/// struct SampleFunction
/// {
///     static std::string_view name() { /* returns unique function name *.}
///
/// template<typename Store, typename Arg, typename It>
/// Result<Store> eval(Arg &&arg, const It &begin, const It &end) const
/// {
///     // Store: an entity that can store calculation result (see Store contract).
///     // Arg is callable: Result<Store>(op::Id) (used to access arguments by identifier).
///     // It is an iterator over op::Id (function argument identifiers).
/// }
/// };
/// @endcode
/// @tparam Funs pack of included function types.
template<typename... Funs>
class Pool
{
    /// Defines a type that holds included extension functions.
    using List = std::tuple<Funs...>;

    /// Defines a name -> id mapping for included extension functions.
    using Map  = std::unordered_map<std::string_view, FunId>;

    /// Template that looks up and and invokes a stored function by identifier.
    /// Main template, for the case when the function was not found.
    template<size_t... Idx> struct Find
    {
        /// Returns a "function not found" error.
        template<typename Store, typename Arg, typename It>
        static Result<Store> invoke(const List &, FunId which, Arg &&, const It &, const It &)
        {
            return error<err::Kind::EXPR_BAD_FUNCTION_ID>(which, std::tuple_size<List>::value - 1);
        }
    };

    /// Template that looks up and and invokes a stored function by identifier.
    /// Spcialization that either invokes current function or attemts to invoke the next one.
    template<size_t Idx, size_t... Rest> struct Find<Idx, Rest...>
    {
        /// Invokes current function if it's the requested one or attemps the next one.
        /// @tparam Store type that can store calculation results (see Store contract).
        /// @tparam Arg callable, Result<Store>(op::Id).
        /// @tparam It forward iterator type over op::Id values.
        /// @param funs a tuple of extension functions.
        /// @param which identifier of the function to invoke.
        /// @param arg argument value getter.
        /// @param start argument identifier range start.
        /// @param end argument identifier range end.
        /// @return function's calculation result or an error.
        template<typename Store, typename Arg, typename It>
        static Result<Store> invoke(const List &funs, FunId which, Arg &&arg, const It &start, const It &end)
        {
            return which == Idx ?
                std::get<Idx>(funs).template eval<Store>(std::forward<Arg>(arg), start, end) :
                Find<Rest...>::template invoke<Store>(funs, which, std::forward<Arg>(arg), start, end);
        }
    };

    /// Build a name -> identifier mapping from a sequence of function ids.
    /// @tparam Idx pack of function infices built from Funs pack.
    /// @return name -> identifier mapping.
    template<size_t... Idx>
    static Map index(std::index_sequence<Idx...>);

    /// Invokes a stored function by identifier.
    /// @tparam Store type that can store calculation results (see Store contract).
    /// @tparam Arg callable, Result<Store>(op::Id).
    /// @tparam It forward iterator type over op::Id values.
    /// @tparam Idx function identifier pack generated from Funs pack.
    /// @param which identifier of the function to invoke.
    /// @param arg argument value getter.
    /// @param start argument identifier range start.
    /// @param end argument identifier range end.
    /// @return function's calculation result or an error.
    template<typename Store, typename Arg, typename It, size_t... Idx>
    Result<Store> invoke(std::index_sequence<Idx...>, FunId which, Arg &&arg, const It &start, const It &end) const;

private:
    /// Extension functions.
    List d_functions;

    /// Name -> identifier function mapping.
    Map  d_byname;

public:
    /// Defines iterator type for available extension functions, as (name, identifier) pairs.
    using const_iterator = Map::const_iterator;

    /// Construct function collection from a list of function objects.
    /// @tparam Fs pack of function types.
    /// @param functions function objects to store.
    template<typename... Fs>
    explicit Pool(Fs &&...functions);

    /// Concatenate this function collection with another.
    /// @tparam Others function type pack to add to the collection.
    /// @param other other collection of functions to add to this one.
    /// @return a new collection that contains functions from both this and other collections.
    template<typename... Others>
    Pool<Funs..., Others...> operator+(const Pool<Others...> &other) const;

    /// Returns the start of supported functions range.
    const_iterator begin() const;

    /// Returns the end of supported functions range.
    const_iterator end() const;

    /// Gets a unique function identifier from function name.
    /// @param name function name.
    /// @return function identifier or an error (if function was not found).
    Result<FunId> operator[](std::string_view name) const;

    /// Invokes a stored function by identifier.
    /// @tparam Store type that can store calculation results (see Store contract).
    /// @tparam Arg callable, Result<Store>(op::Id).
    /// @tparam It forward iterator type over op::Id values.
    /// @param fun identifier of the function to invoke.
    /// @param arg argument value getter.
    /// @param start argument identifier range start.
    /// @param end argument identifier range end.
    /// @return function's calculation result or an error.
    template<typename Store, typename Arg, typename It>
    Result<Store> invoke(FunId fun, Arg &&arg, const It &start, const It &end) const;
};


/// Defines empty extension function pool variable.
inline const Pool<> none;


/// Constructs an extension function pool from a list of function objects.
/// @see Pool
/// @tparam Funs pack of extension function types.
/// @param functions extension functions to include to the pool.
/// @return extension pool instance.
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
Pool<Funs..., Others...> Pool<Funs...>::operator+(const Pool<Others...> &other) const
{
    auto functions = std::tuple_cat(d_functions, other.d_functions);
    return Pool<Funs..., Others...>(std::move(functions));
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
