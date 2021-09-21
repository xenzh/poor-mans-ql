#pragma once

#include "error.h"
#include "extensions.h"

#include <string_view>


namespace pmql::ext {


struct Avail
{
    static std::string_view name()
    {
        return "avail";
    }

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


const auto &builtin()
{
    static const auto all = pool(Avail {});
    return all;
}


} // namespace pmql::ext
