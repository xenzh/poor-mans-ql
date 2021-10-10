#pragma once

#include "result.h"

#include <tl/expected.hpp>

#include <utility>
#include <string>
#include <ostream>


namespace pmql {
namespace err {


/// Macro list of available error definitions.
#define ErrorKinds \
    ErrorKind(OP_BAD_ARGUMENT         ) \
    ErrorKind(OP_BAD_SUBSTITUTION     ) \
    ErrorKind(OP_INCOMPATIBLE_TYPES   ) \
    ErrorKind(OP_TERNARY_BAD_CONDITION) \
    ErrorKind(BUILDER_EMPTY           ) \
    ErrorKind(BUILDER_REF_TO_UNKNOWN  ) \
    ErrorKind(BUILDER_DANGLING        ) \
    ErrorKind(BUILDER_BAD_ARGUMENT    ) \
    ErrorKind(BUILDER_BAD_SUBSTITUTION) \
    ErrorKind(CONTEXT_BAD_VARIABLE    ) \
    ErrorKind(EXPR_NOT_READY          ) \
    ErrorKind(EXPR_BAD_SUBST          ) \
    ErrorKind(EXPR_BAD_FUNCTION       ) \
    ErrorKind(EXPR_BAD_FUNCTION_ID    ) \
    ErrorKind(SERIAL_UNKNOWN_TOKEN    ) \
    ErrorKind(SERIAL_BAD_TOKEN        ) \


/// Generated enum that can identify any defined error.
#define ErrorKind(E) E,
enum class Kind { ErrorKinds };
#undef ErrorKind


/// Specializations of this template can describe any defined error.
template<Kind K> struct Details;


/// Stream operator for error kind descriptors.
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
        os << "Operation " << op << " failed to get argument #" << arg << ": " << cause;
    }
};

template<> struct Details<Kind::OP_BAD_SUBSTITUTION>
{
    std::string op;
    size_t sub;
    std::string cause;

    template<typename Op, typename Cause>
    Details(Op &&op, size_t sub, Cause &&cause)
        : op(format(std::forward<Op>(op)))
        , sub(sub)
        , cause(format(std::forward<Cause>(cause)))
    {
    }

    void operator()(std::ostream &os) const
    {
        os << op << " failed to substitute $" << sub << ": " << cause;
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

template<> struct Details<Kind::OP_TERNARY_BAD_CONDITION>
{
    std::string op;
    std::string value;

    template<typename Op, typename Val>
    Details(Op &&op, Val &&val)
        : op(format(op))
        , value(format(val))
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Ternary operator \"" << op << "\"'s condition cannot be converted to bool: " << value;
    }
};

template<> struct Details<Kind::BUILDER_EMPTY>
{
    void operator()(std::ostream &os) const
    {
        os << "Nothing to calculate";
    }
};

template<> struct Details<Kind::BUILDER_REF_TO_UNKNOWN>
{
    std::string ops;
    std::string op;
    size_t ref;
    size_t max;

    template<typename Ops>
    Details(const Ops &ops, std::string_view op, size_t ref, size_t max)
        : ops(format(ops))
        , op(std::move(op))
        , ref(ref)
        , max(max)
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Operation " << op << " cannot be built, "
            "it refers to an unknown argument #" << ref << " (max: #" << max << "); ops:\n" << ops;
    }
};

template<> struct Details<Kind::BUILDER_DANGLING>
{
    std::string ops;
    std::string op;
    size_t id;

    template<typename Ops, typename Op>
    Details(const Ops &ops, const Op &op, size_t id)
        : ops(format(ops))
        , op(format(op))
        , id(id)
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Operation #" << id << " is dangling; ops: " << ops;
    }
};

template<> struct Details<Kind::BUILDER_BAD_ARGUMENT>
{
    std::string ops;
    std::string op;
    size_t id;
    size_t ref;

    template<typename Ops, typename Op>
    Details(const Ops &ops, const Op &op, size_t id, size_t ref)
        : ops(format(ops))
        , op(format(op))
        , id(id)
        , ref(ref)
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Operation #" << id << ": " << op << " refers to an element #" << ref <<
            " up the expression tree; ops:\n" << ops;
    }
};

template<> struct Details<Kind::BUILDER_BAD_SUBSTITUTION>
{
    std::string ops;
    std::string op;
    size_t id;
    size_t sub;
    size_t max;

    template<typename Ops, typename Op>
    Details(const Ops &ops, const Op &op, size_t id, size_t sub, size_t max)
        : ops(format(ops))
        , op(format(op))
        , id(id)
        , sub(sub)
        , max(max)
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Variable #" << id << ": " << op << " refers to an unknown substitution $" << sub <<
            " (max: $" << max << "); ops:\n" << ops;
    }
};

template<> struct Details<err::Kind::CONTEXT_BAD_VARIABLE>
{
    std::string_view var;

    void operator()(std::ostream &os) const
    {
        os << "Variable $" << var << " not found in the expression context";
    }
};

template<> struct Details<Kind::EXPR_NOT_READY>
{
    void operator()(std::ostream &os) const
    {
        os << "Not ready";
    }
};

template<> struct Details<Kind::EXPR_BAD_SUBST>
{
    std::string var;

    Details(std::string_view var)
        : var(var)
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Accessor for variable $" << var << " is missing";
    }
};

template<> struct Details<Kind::EXPR_BAD_FUNCTION>
{
    std::string_view name;

    void operator()(std::ostream &os) const
    {
        os << "Unknown extension function requested: " << name;
    }
};

template<> struct Details<Kind::EXPR_BAD_FUNCTION_ID>
{
    size_t id;
    size_t max;

    void operator()(std::ostream &os) const
    {
        os << "Unknown extension function id requested: " << id << "/" << max;
    }
};

template<> struct Details<Kind::SERIAL_UNKNOWN_TOKEN>
{
    std::string stored;
    size_t pos;
    std::string cause;

    template<typename... Cause>
    Details(std::string_view stored, size_t pos, Cause &&...cause)
        : stored(stored)
        , pos(pos)
        , cause(format(std::forward<Cause>(cause)...))
    {
    }

    void operator()(std::ostream &os) const
    {
        std::string_view sv = stored;

        os << "Failed to recognize a token at \"" << sv.substr(pos) << "\"; "
            "cause: " << cause << "; "
            "full expression: \"" << sv << "\"" ;
    }
};

template<> struct Details<Kind::SERIAL_BAD_TOKEN>
{
    std::string_view entity;
    std::string token;
    std::string cause;

    template<typename... Cause>
    Details(std::string_view entity, std::string_view token, Cause &&...cause)
        : entity(entity)
        , token(token)
        , cause(format(std::forward<Cause>(cause)...))
    {
    }

    void operator()(std::ostream &os) const
    {
        os << "Failed to load entity " << entity << " from \"" << token << "\"";
        if (!cause.empty())
        {
            os << ", cause: " << cause;
        }
    }
};


/// Defines an error type that can hold any defined error along with its details.
#define ErrorKind(E) , Kind::E
using Error = ErrorTemplate<Kind, Details ErrorKinds>;
#undef ErrorKind


/// Defines error kind marker type.
template<Kind K> using kind_t = typename Error::template kind_t<K>;

/// Defines error kind marker literal template.
template<Kind K> inline constexpr kind_t<K> kind;


#undef ErroKinds


} // namespace err


/// Monadic result type that can hold either a value (including void) or an error.
template<typename V> using Result = tl::expected<V, err::Error>;


/// Construct result-compatible error descriptor.
/// @tparam K error kind identifier.
/// @tparam Args error descriptor's constructor / direct list initialization arguments.
/// @param args direct list initialization arguments.
/// @return result-compatible error descriptor.
template<err::Kind K, typename... Args>
tl::unexpected<err::Error> error(Args &&...args)
{
    return tl::unexpected<err::Error> {err::Error {err::kind<K>, std::forward<Args>(args)...}};
}

/// Re-pack error descriptor so that it can be used to create a Result object of a different success type.
/// @tparam E error type.
/// @param err error descriptor.
/// @return result-compatible error descriptor.
template<typename E>
tl::unexpected<err::Error> error(E &&err)
{
    return tl::unexpected<err::Error> {std::forward<E>(err)};
}


} // namespace pmql


namespace tl {


/// Streaming operator for Result types.
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
