#pragma once

#include <variant>
#include <string>
#include <sstream>


namespace pmql::err {


template<typename Kind, template<Kind> typename Details, Kind ...Kinds>
class ErrorTemplate
{
    Kind d_kind;
    std::variant<Details<Kinds>...> d_details;

public:
    template<Kind> struct kind_t {};

    template<Kind K, typename... Args>
    ErrorTemplate(kind_t<K>, Args &&...args)
        : d_kind {K}
        , d_details {Details<K> {std::forward<Args>(args)...}}
    {
    }

    Kind kind() const
    {
        return d_kind;
    }

    template<Kind K>
    const Details<K> *details() const
    {
        return std::visit(
            [] (const auto &detail)
            {
                using Actual = std::decay_t<decltype(detail)>;
                if constexpr (std::is_same_v<Details<K>, Actual>)
                {
                    return &detail;
                }
                return nullptr;
            },
            d_details);
    }

    std::ostream &describe(std::ostream &os) const
    {
        return std::visit(
            [&os] (const auto &detail) mutable -> std::ostream &
            {
                return os << detail;
            },
            d_details);
    }

    std::string description() const
    {
        std::ostringstream ostr;
        describe(ostr);
        return std::move(ostr).str();
    }
};


template<typename Kind, template<Kind> typename Details, Kind ...Kinds>
std::ostream &operator<<(std::ostream &os, const ErrorTemplate<Kind, Details, Kinds...> &error)
{
    return error.describe(os);
}


template<typename... Args>
std::string format(Args &&...args)
{
    std::ostringstream ostr;
    (ostr << ... << args);
    return std::move(ostr).str();
}


} // namespace pmql::err
