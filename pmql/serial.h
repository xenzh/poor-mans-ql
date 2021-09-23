#include "error.h"
#include "ops.h"
#include "extensions.h"

#include <utility>
#include <vector>
#include <string>
#include <sstream>


namespace pmql::serial {


template<typename Store>
using Ingredients = std::pair<std::vector<Store>, op::List>;


template<typename Store>
Result<std::string> store(const op::List &ops, const std::vector<Store> &consts);


template<typename Store, typename... Funs>
Result<Ingredients<Store>> load(std::string_view stored, const ext::Pool<Funs...> &ext = ext::none);


namespace detail {


inline constexpr std::string_view CONST = "_";
inline constexpr std::string_view VAR   = "$";
inline constexpr std::string_view OP    = "#";
inline constexpr std::string_view FUN   = "@";


template<typename Store, typename Op> struct Token;


template<typename Store>
Result<void> dispatch(std::ostream &os, op::Id op, const op::List &ops, const std::vector<Store> &consts)
{
    return std::visit(
        [&os, &ops, &consts] (const auto &op) mutable -> Result<void>
        {
            using Op = std::decay_t<decltype(op)>;
            return Token<Store, Op>::store(os, op, ops, consts);
        },
        ops[op]);
}


template<typename Store> struct MultiArg
{
    std::ostream &os;
    const op::List &ops;
    const std::vector<Store> &consts;

    Result<void> result;
    bool first = true;

    const std::string_view separator;
    const bool preindent;

    MultiArg(
        std::ostream &os,
        const op::List &ops,
        const std::vector<Store> &consts,
        std::string_view separator,
        bool preindent)
        : os(os)
        , ops(ops)
        , consts(consts)
        , separator(separator)
        , preindent(preindent)
    {
    }

    void operator()(op::Id ref)
    {
        if (!result.has_value())
        {
            return;
        }

        if (first)
        {
            result = dispatch(os, ref, ops, consts);
            first = false;
        }
        else
        {
            os << (preindent ? " " : "") << separator << " ";
            result = dispatch(os, ref, ops, consts);
        }
    }

    operator Result<void>() &&
    {
        return std::move(result);
    }
};


template<typename Store> struct Token<Store, op::Const>
{
    static Result<void> store(std::ostream &os, const op::Const &cn, const op::List &, const std::vector<Store> &consts)
    {
        os << CONST << "{";
        cn.refers([&os, &consts] (op::Id ref) mutable
        {
            consts.at(ref).store(os);
        });
        os << "}";

        return {};
    }
};

template<typename Store> struct Token<Store, op::Var>
{
    static Result<void> store(std::ostream &os, const op::Var &var, const op::List &, const std::vector<Store> &)
    {
        os << VAR << "{" << var.name() << "}";
        return {};
    }
};

template<typename Store, template<typename> typename Fn> struct Token<Store, op::Unary<Fn>>
{
    static Result<void> store(
        std::ostream &os,
        const op::Unary<Fn> &op,
        const op::List &ops,
        const std::vector<Store> &consts)
    {
        Result<void> res;

        os << op::Traits<Fn>::sign;
        op.refers([&res, &os, &ops, &consts] (op::Id ref) mutable { res = dispatch(os, ref, ops, consts); });

        return res;
    }
};

template<typename Store, template<typename> typename Fn> struct Token<Store, op::Binary<Fn>>
{
    static Result<void> store(
        std::ostream &os,
        const op::Binary<Fn> &op,
        const op::List &ops,
        const std::vector<Store> &consts)
    {
        os << "(";
        MultiArg writer {os, ops, consts, op::Traits<Fn>::sign, true};
        op.refers(writer);
        os << ")";

        return std::move(writer);
    }
};

template<typename Store> struct Token<Store, op::Ternary>
{
    static constexpr std::string_view name = op::OpTraits<op::Ternary>::type::name;

    static Result<void> store(
        std::ostream &os,
        const op::Ternary &cond,
        const op::List &ops,
        const std::vector<Store> &consts)
    {
        os << name << "(";
        MultiArg writer {os, ops, consts, ",", false};
        cond.refers(writer);
        os << ")";

        return std::move(writer);
    }
};

template<typename Store> struct Token<Store, op::Extension>
{
    static Result<void> store(
        std::ostream &os,
        const op::Extension &fun,
        const op::List &ops,
        const std::vector<Store> &consts)
    {
        os << FUN << fun.name() << "(";
        MultiArg writer {os, ops, consts, ",", false};
        fun.refers(writer);
        os << ")";

        return std::move(writer);
    }
};


} // namespace detail


template<typename Store>
Result<std::string> store(const op::List &ops, const std::vector<Store> &consts)
{
    std::ostringstream os;
    return detail::dispatch(os, ops.size() - 1, ops, consts)
        .map([os = std::move(os)] () mutable
        {
            return std::move(os).str();
        });
}

template<typename Store, typename... Funs>
Result<Ingredients<Store>> load(std::string_view stored, const ext::Pool<Funs...> &ext /*= ext::none*/)
{
    (void) stored;
    (void) ext;
    return {};
}


} // namespace pmql::serial
