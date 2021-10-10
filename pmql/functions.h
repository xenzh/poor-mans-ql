#pragma once

#include "error.h"
#include "extensions.h"

#include <string_view>


namespace pmql::ext {


/// Builtin extension function that evaluates to first non-null argument value.
struct Avail
{
    /// Returns function name.
    /// @return function name.
    static std::string_view name()
    {
        return "avail";
    }

    /// Returns a first non-null argument value, or null of all arguments are null.
    /// @tparam Store type that can store calculation results (see Store contract).
    /// @tparam Arg callable, Result<Store>(op::Id).
    /// @tparam It forward iterator type over op::Id values.
    /// @param arg argument value getter.
    /// @param begin argument identifier range start.
    /// @param end argument identifier range end.
    /// @return function's calculation result or an error.
    template<typename Store, typename Arg, typename It>
    Result<Store> eval(Arg &&arg, const It &begin, const It &end) const
    {
        for (auto it = begin; it != end; ++it)
        {
            const auto &result = arg(*it);
            if (!result)
            {
                return result;
            }

            if (bool(*result))
            {
                return *result;
            }
        }

        return Store {};
    }
};


/// Returns an extension pool reference containing all builtin functions.
/// @return builtin extension functions pool.
const auto &builtin()
{
    static const auto all = pool(Avail {});
    return all;
}


} // namespace pmql::ext
