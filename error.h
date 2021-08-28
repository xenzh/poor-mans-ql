#pragma once

#include <tl/expected.hpp>

#include <variant>
#include <string>
#include <sstream>


namespace err {


#define ErrorKinds \
    ErrorKind(OPS_REF_TO_UNKNOWN) \
    ErrorKind(OPS_EMPTY) \
    ErrorKind(OPS_CYCLE) \
    ErrorKind(OPS_DANGLING) \
    ErrorKind(OPS_TOO_MANY) \
    ErrorKind(OPS_NOT_FOUND)


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


template<> struct Details<Kind::OPS_REF_TO_UNKNOWN>
{
    std::string op;
    size_t ref;
    size_t max;

    void operator()(std::ostream &os) const
    {
        os << "Operation " << op << " references #" << ref << " which is unknown (max id: " << max << ")";
    }
};

template<> struct Details<Kind::OPS_EMPTY>
{
    void operator()(std::ostream &os) const
    {
        os << "Expression is empty";
    }
};

template<> struct Details<Kind::OPS_CYCLE>
{
    std::string ops;
    size_t detected;

    void operator()(std::ostream &os) const
    {
        os << "Cycle detected at #" << detected << " in sequence: " << ops;
    }
};

template<> struct Details<Kind::OPS_DANGLING>
{
    std::string ops;
    size_t dangling;

    void operator()(std::ostream &os) const
    {
        os << "Dangling operation is detected at #" << dangling << " in sequence " << ops;
    }
};

template<> struct Details<Kind::OPS_TOO_MANY>
{
    std::string ops;
    size_t limit;
    size_t count;

    void operator()(std::ostream &os) const
    {
        os << "Expression operation limit exceeded (" << count << "/" << limit << "): " << ops;
    }
};

template<> struct Details<Kind::OPS_NOT_FOUND>
{
    size_t overflow;

    void operator()(std::ostream &os) const
    {
        os << "Requested operation that is " << overflow << " positions past what is available";
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


template<Kind K, typename... Args>
tl::unexpected<Error> error(Args &&...args)
{
    return tl::unexpected<Error> {Error {kind<K>, std::forward<Args>(args)...}};
}


template<typename V> using Result = tl::expected<V, Error>;


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


namespace tl {


template<typename V>
std::ostream &operator<<(std::ostream &os, const expected<V, err::Error> &result)
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
