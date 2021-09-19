#pragma once

#include "result.h"

#include <tl/expected.hpp>

#include <utility>
#include <string>
#include <ostream>


namespace pmql {
namespace err {


#define ErrorKinds \
    ErrorKind(OP_BAD_ARGUMENT         ) \
    ErrorKind(OP_BAD_SUBSTITUTION     ) \
    ErrorKind(OP_INCOMPATIBLE_TYPES   ) \
    ErrorKind(BUILDER_EMPTY           ) \
    ErrorKind(BUILDER_REF_TO_UNKNOWN  ) \
    ErrorKind(BUILDER_DANGLING        ) \
    ErrorKind(BUILDER_BAD_ARGUMENT    ) \
    ErrorKind(BUILDER_BAD_SUBSTITUTION) \
    ErrorKind(CONTEXT_BAD_VARIABLE    ) \
    ErrorKind(EXPR_NOT_READY          ) \
    ErrorKind(EXPR_BAD_SUBST          ) \


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


#define ErrorKind(E) , Kind::E
using Error = ErrorTemplate<Kind, Details ErrorKinds>;
#undef ErrorKind


template<Kind K> using kind_t = typename Error::template kind_t<K>;

template<Kind K> inline constexpr kind_t<K> kind;


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
