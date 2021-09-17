#pragma once

#include <tl/expected.hpp>

#include <variant>
#include <string>
#include <sstream>


namespace pmql {
namespace err {


#define ErrorKinds \
    ErrorKind(OP_BAD_ARGUMENT       ) \
    ErrorKind(OP_INCOMPATIBLE_TYPES ) \
    ErrorKind(BUILDER_REF_TO_UNKNOWN) \


#define ErrorKind(E) E,
enum class Kind { ErrorKinds };
#undef ErrorKind


template<Kind K> struct Details;

template<Kind K>
std::ostream &operator<<(std::ostream &os, const Details<K> &details)
{
    details(os);
    return os;
}


template<> struct Details<Kind::OP_BAD_ARGUMENT>
{
    std::string op;
    size_t arg;
    std::string cause;

    template<typename Op, typename Cause>
    Details(Op &&op, size_t arg, Cause &&cause)
        : op(format(std::forward<Op>(op)))
        , arg(arg)
        , cause(format(std::forward<Cause>(cause)))
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Operation " << op << "failed to get argument #" << arg << ": " << cause;
    }
};

template<> struct Details<Kind::OP_INCOMPATIBLE_TYPES>
{
    std::string op;
    std::string argtypes;

    void operator()(std::ostream &os) const
    {
        os << "Operation " << op << " cannot be called with arguments of following types: " << argtypes;
    }
};

template<> struct Details<Kind::BUILDER_REF_TO_UNKNOWN>
{
    std::string op;
    size_t ref;
    size_t max;

    void operator()(std::ostream &os) const
    {
        os << "Operation " << op << " cannot be built, "
            "it refers to an unknown argument #" << ref << " (max: #" << max << ")";
    }
};


template<Kind K> struct kind_t {};

template<Kind K> inline constexpr kind_t<K> kind;


class Error
{
#define ErrorKind(E) , Kind::E
    template<template<typename...> typename To, Kind... kinds> using expand_t = To<Details<kinds>...>;
    using Variant = expand_t<std::variant ErrorKinds>;
#undef ErrorKind

    Kind d_kind;
    Variant d_details;

public:
    template<Kind K, typename... Args>
    Error(kind_t<K>, Args &&...args)
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

inline std::ostream &operator<<(std::ostream &os, const Error &error)
{
    return error.describe(os);
}


template<typename V> using Result = tl::expected<V, Error>;


template<Kind K, typename... Args>
tl::unexpected<Error> error(Args &&...args)
{
    return tl::unexpected<Error> {Error {kind<K>, std::forward<Args>(args)...}};
}


template<typename E>
tl::unexpected<Error> error(E &&err)
{
    return tl::unexpected<Error> {std::forward<E>(err)};
}


template<typename... Args>
std::string format(Args &&...args)
{
    std::ostringstream ostr;
    (ostr << ... << args);
    return std::move(ostr).str();
}


#undef ErroKinds


} // namespace err


template<typename V> using Result = err::Result<V>;


} // namespace pmql


namespace tl {


template<typename V>
std::ostream &operator<<(std::ostream &os, const expected<V, pmql::err::Error> &result)
{
    if (result)
    {
        if constexpr (std::is_same_v<void, V>)
        {
            return os << "ok()";
        }
        else
        {
            return os << "ok(" << *result << ")";
        }
    }
    else
    {
        return os << "err(" << result.error() << ")";
    }
}


} // namespace tl
