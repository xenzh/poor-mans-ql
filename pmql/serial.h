#include "error.h"
#include "ops.h"
#include "extensions.h"
#include "builder.h"

#include <utility>
#include <string>
#include <sstream>


namespace pmql::serial {


inline constexpr std::pair<char, char> ROUND = std::make_pair('(', ')');
inline constexpr std::pair<char, char> CURLY = std::make_pair('{', '}');

inline constexpr char CONST = '_';
inline constexpr char VAR   = '$';
inline constexpr char BINOP = '(';
inline constexpr char COND  = '?';
inline constexpr char FUN   = '@';
inline constexpr char BINFN = ' ';
inline constexpr char ARG   = ',';


template<typename Till>
Result<std::string_view> extract(std::string_view source, size_t &pos, Till &&till)
{
    size_t start = pos, nested = 0;
    while (pos < source.size())
    {
        if (source[pos] == ROUND.first || source[pos] == CURLY.first)
        {
            ++nested;
        }
        if (nested > 0 && (source[pos] == ROUND.second || source[pos] == CURLY.second))
        {
            --nested;
        }

        if (till(source.substr(pos)) && nested == 0)
        {
            if (start == pos)
            {
                return error<err::Kind::SERIAL_UNKNOWN_TOKEN>(source, start, "extracted token is empty");
            }

            return source.substr(start, pos - start - 1);
        }
    }

    return error<err::Kind::SERIAL_UNKNOWN_TOKEN>(source, start, "failed to extract token before EOF");
}


inline Result<std::string_view> extract(std::string_view source, size_t &pos, char till)
{
    return extract(source, pos, [till] (std::string_view c) { return c[0] == till; });
}


template<typename Op, typename Store, typename... Funs> struct Save;
template<typename Op, typename Store, typename... Funs> struct Load;


template<typename Store, typename... Funs>
Result<void> dispatch(std::ostream &os, op::Id op, const Ingredients<Store, Funs...> &data)
{
    return std::visit(
        [&os, &data] (const auto &op) mutable -> Result<void>
        {
            return Save<std::decay_t<decltype(op)>, Store, Funs...> {os, data}(op);
        },
        data.ops[op]);
}

template<typename Store, typename... Funs>
Result<op::Id> dispatch(std::string_view entity, Builder<Store, Funs...> &builder)
{
    if (entity.empty())
    {
        return {};
    }

    size_t pos = 1;
    switch(entity[0])
    {
    case CONST:
        {
            Load<op::Const, Store, Funs...> load {builder};
            return extract(entity, pos, CURLY.second).and_then(load);
        }
        break;
    case VAR:
        {
            Load<op::Const, Store, Funs...> load {builder};
            return extract(entity, pos, CURLY.second).and_then(load);
        }
        break;
    case COND:
        {
            Load<op::Ternary, Store, Funs...> load(builder);
            return extract(entity, pos, ROUND.second).and_then(load);
        }
        break;
    case FUN:
        {
            Load<op::Extension, Store, Funs...> load(builder);
            return extract(entity, pos, ROUND.second).and_then(load);
        }
        break;
    case BINOP:
        {
            //extract(entity, pos, [] (std::string_view c)
            //{
            //    auto it = op::Signs.find_if([&c] (auto sign) {  });
            //    for (const auto &sign : op::Signs)

            //});
        }
        break;
    default:
        {
            pos = 0;
            return op::identify(entity, pos, [entity, &builder, &pos] (auto traits) mutable -> Result<op::Id>
            {
                using Tr = std::decay_t<decltype(traits)>;
                if constexpr (std::is_same_v<Tr, std::nullopt_t>)
                {
                    return error<err::Kind::SERIAL_UNKNOWN_TOKEN>(entity, 0, "unknown operation");
                }
                else if constexpr (Tr::max_arity != 1)
                {
                    return error<err::Kind::SERIAL_UNKNOWN_TOKEN>(entity, 0, "binary operation is used as unary");
                }
                else
                {
                    Load<typename Tr::op, Store, Funs...> load(builder);
                    return extract(entity, pos, ROUND.second).and_then(load);
                }
            });
        }
    }
}


template<typename Store, typename... Funs>
struct MultiArg
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> result;
    bool first = true;

    const std::string_view separator;
    const bool preindent;

    MultiArg(
        std::ostream &os,
        const Ingredients<Store, Funs...> &data,
        std::string_view separator,
        bool preindent)
        : os(os)
        , data(data)
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
            result = dispatch(os, ref, data);
            first = false;
        }
        else
        {
            os << (preindent ? " " : "") << separator << " ";
            result = dispatch(os, ref, data);
        }
    }

    operator Result<void>() &&
    {
        return std::move(result);
    }
};


template<typename Store, typename... Funs> struct Save<op::Const, Store, Funs...>
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> operator()(const op::Const &cn)
    {
        os << CONST << "{";
        cn.refers([this] (op::Id ref) mutable
        {
            data.consts.at(ref).store(os);
        });
        os << "}";

        return {};
    }
};

template<typename Store, typename... Funs> struct Load<op::Const, Store, Funs...>
{
    Builder<Store, Funs...> &builder;

    Result<op::Id> operator()(std::string_view token)
    {
        return Store::load(token).and_then([this] (auto &&cn) mutable { return builder.constant(std::move(cn)); });
    }
};


template<typename Store, typename... Funs> struct Save<op::Var, Store, Funs...>
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> operator()(const op::Var &var)
    {
        os << VAR << "{" << var.name() << "}";
        return {};
    }
};

template<typename Store, typename... Funs> struct Load<op::Var, Store, Funs...>
{
    Builder<Store, Funs...> &builder;

    Result<op::Id> operator()(std::string_view token)
    {
        return builder.var(token);
    }
};


template<typename Store, template<typename> typename Fn, typename... Funs> struct Save<op::Unary<Fn>, Store, Funs...>
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> operator()(const op::Unary<Fn> &op)
    {
        Result<void> res;

        os << op::Traits<Fn>::sign;
        op.refers([this, &res] (op::Id ref) mutable { res = dispatch(os, ref, data); });

        return res;
    }
};

template<typename Store, template<typename> typename Fn, typename... Funs> struct Load<op::Unary<Fn>, Store, Funs...>
{
    Builder<Store, Funs...> &builder;

    Result<op::Id> operator()(std::string_view token)
    {
        return dispatch<Store, Funs...>(token, builder)
            .and_then([&bld = builder] (op::Id arg) mutable
            {
                return bld.template op<Fn>(arg);
            });
    }
};


template<typename Store, template<typename> typename Fn, typename... Funs> struct Save<op::Binary<Fn>, Store, Funs...>
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> operator()(const op::Binary<Fn> &op)
    {
        os << "(";
        MultiArg writer {os, data, op::Traits<Fn>::sign, true};
        op.refers(writer);
        os << ")";

        return std::move(writer);
    }
};

template<typename Store, template<typename> typename Fn, typename... Funs> struct Load<op::Binary<Fn>, Store, Funs...>
{
    Builder<Store, Funs...> &builder;

    Result<op::Id> operator()(std::string_view token)
    {
        return {};
    }
};


template<typename Store, typename... Funs> struct Save<op::Ternary, Store, Funs...>
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> operator()(const op::Ternary &cond)
    {
        os << op::OpTraits<op::Ternary>::type::name << "(";
        MultiArg writer {os, data, ",", false};
        cond.refers(writer);
        os << ")";

        return std::move(writer);
    }
};

template<typename Store, typename... Funs> struct Load<op::Ternary, Store, Funs...>
{
    Builder<Store, Funs...> &builder;

    Result<op::Id> operator()(std::string_view token)
    {
        size_t start = 0;
        auto next = [token, &start, &bld = builder] () mutable
        {
            return extract(token, start, ARG).and_then([&bld] (std::string_view arg) mutable
            {
                return dispatch<Store, Funs...>(arg, bld);
            });
        };

        return next().and_then([&next, &bld = builder] (op::Id cond) mutable
        {
            return next().and_then([&next, &bld, cond] (op::Id iftrue)
            {
                return next().and_then([&next, &bld, cond, iftrue] (op::Id iffalse)
                {
                    return bld.branch(cond, iftrue, iffalse);
                });
            });
        });
    }
};


template<typename Store, typename... Funs> struct Save<op::Extension, Store, Funs...>
{
    std::ostream &os;
    const Ingredients<Store, Funs...> &data;

    Result<void> operator()(const op::Extension &fun)
    {
        os << FUN << fun.name() << "(";
        MultiArg writer {os, data, ",", false};
        fun.refers(writer);
        os << ")";

        return std::move(writer);
    }
};

template<typename Store, typename... Funs> struct Load<op::Extension, Store, Funs...>
{
    Builder<Store, Funs...> &builder;

    Result<op::Id> operator()(std::string_view token)
    {
        size_t pos = 0;

        auto name = extract(token, pos, ROUND.first);
        if (!name)
        {
            return error(std::move(name).error());
        }

        std::vector<op::Id> args;
        while (pos < token.size())
        {
            auto ok = extract(token, pos, ARG)
                .and_then([this] (std::string_view arg) mutable
                {
                    return dispatch<Store, Funs...>(arg, builder);
                })
                .map([&args] (op::Id arg) mutable
                {
                    args.push_back(arg);
                });

            if (!ok)
            {
                return error(std::move(ok).error());
            }
        }

        return builder.fun(*name, std::move(args));
    }
};


} // namespace pmql::serial
