#include "../pmql/parser/grammar.h"
#include "../pmql/parser/dryrun.h"

#include "tao/pegtl/internal/pegtl_string.hpp"

#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/trace.hpp>

#include <gtest/gtest.h>


namespace pmql::parser {
namespace {


using namespace ::testing;


struct Rule
{
#define PmqlStdOp(Ns, Fn, Sign, Arity) rules::Fn,
    using rule_t = std::variant<
        PmqlStdOpList
        rules::type,
        rules::value,
        rules::typedvalue,
        rules::null,
        rules::constant,
        rules::varname,
        rules::variable,
        rules::unaryop,
        rules::binaryop,
        rules::unary,
        rules::binary,
        rules::arithmetic,
        rules::ternary,
        rules::expression
    >;
#undef PmqlStdOp

    rule_t rule;
    std::string sample;
    bool outcome;
};


inline std::ostream &operator<<(std::ostream &os, const Rule &rule)
{
    os << "[";
    std::visit(
        [&os] (auto rule) mutable
        {
            os << rules::id<decltype(rule)>::name << ", ";
        },
        rule.rule);

    os << "'" << rule.sample << "'" << ": " << (rule.outcome ? "match" : "no match");
    return os << "]";
}


} // unnamed namespace


struct PegtlRuleParse : TestWithParam<Rule> {};


/// Check if a grammar rule is valid and can be parsed.
TEST_P(PegtlRuleParse, Sample)
{
    using namespace tao::pegtl;

    std::visit(
        [&param = GetParam()] (auto rule)
        {
            if (analyze<decltype(rule)>() != 0)
            {
                FAIL() << "Grammar is invalid";
            }

            memory_input input(param.sample, param.sample);
            auto actual = parse<decltype(rule), nothing>(input);

            EXPECT_EQ(param.outcome, actual);
            if (HasFailure())
            {
                standard_trace<decltype(rule)>(input);
            }
        },
        GetParam().rule);
}


INSTANTIATE_TEST_SUITE_P(Type, PegtlRuleParse, Values
(
    Rule {rules::type {}, "int"                 , true },
    Rule {rules::type {}, "valid_c_Identifier69", true },
    Rule {rules::type {}, "  "                  , false}
));

INSTANTIATE_TEST_SUITE_P(Value, PegtlRuleParse, Values
(
    Rule {rules::value {}, "42"                , true },
    Rule {rules::value {}, "  anything $! G0ES", true },
    Rule {rules::value {}, ""                  , false}
));

INSTANTIATE_TEST_SUITE_P(Store, PegtlRuleParse, Values
(
    Rule {rules::typedvalue {}, "int{42}"       , true },
    Rule {rules::typedvalue {}, "_c11_ident{_'}", true },
    Rule {rules::typedvalue {}, "int(42)"       , false},
    Rule {rules::typedvalue {}, "int{}"         , false},
    Rule {rules::typedvalue {}, "int{"          , false},
    Rule {rules::typedvalue {}, "int{{}"        , false},
    Rule {rules::typedvalue {}, " int{42}"      , false}
));

INSTANTIATE_TEST_SUITE_P(Null, PegtlRuleParse, Values
(
    Rule {rules::null {}, "null"    , true },
    Rule {rules::null {}, "nullnull", true },
    Rule {rules::null {}, "nullptr" , true },
    Rule {rules::null {}, "Null"    , false},
    Rule {rules::null {}, "nulL"    , false}
));

INSTANTIATE_TEST_SUITE_P(Constant, PegtlRuleParse, Values
(
    Rule {rules::constant {}, "null"                  , true },
    Rule {rules::constant {}, "double{31.32}"         , true },
    Rule {rules::constant {}, "string{'hello quoted'}", true },
    Rule {rules::constant {}, "+null"                 , false},
    Rule {rules::constant {}, "42(32)"                , false}
));

INSTANTIATE_TEST_SUITE_P(VarName, PegtlRuleParse, Values
(
    Rule {rules::varname {}, "42"                , true },
    Rule {rules::varname {}, "  anything $! G0ES", true },
    Rule {rules::varname {}, ""                  , false}
));

INSTANTIATE_TEST_SUITE_P(Variable, PegtlRuleParse, Values
(
    Rule {rules::variable {}, "${a}"                , true },
    Rule {rules::variable {}, "${ anything $! G0ES}", true },
    Rule {rules::variable {}, "${}"                 , false},
    Rule {rules::variable {}, "${"                  , false},
    Rule {rules::variable {}, "${{}"                , false}
));

INSTANTIATE_TEST_SUITE_P(UnaryOp, PegtlRuleParse, Values
(
    Rule {rules::unaryop {}, "-" , true },
    Rule {rules::unaryop {}, "!" , true },
    Rule {rules::unaryop {}, "~" , true },

    Rule {rules::unaryop {}, "+" , false},
    Rule {rules::unaryop {}, "*" , false},
    Rule {rules::unaryop {}, "/" , false},
    Rule {rules::unaryop {}, "%" , false},
    Rule {rules::unaryop {}, "==", false},
    Rule {rules::unaryop {}, ">" , false},
    Rule {rules::unaryop {}, "<" , false},
    Rule {rules::unaryop {}, ">=", false},
    Rule {rules::unaryop {}, "<=", false},
    Rule {rules::unaryop {}, "&&", false},
    Rule {rules::unaryop {}, "||", false},
    Rule {rules::unaryop {}, "&" , false},
    Rule {rules::unaryop {}, "|" , false},
    Rule {rules::unaryop {}, "^" , false},

    Rule {rules::unaryop {}, ""       , false},
    Rule {rules::unaryop {}, "${a}"   , false},
    Rule {rules::unaryop {}, "int{42}", false}
));

INSTANTIATE_TEST_SUITE_P(BinaryOp, PegtlRuleParse, Values
(
    Rule {rules::binaryop {}, "!" , false},
    Rule {rules::binaryop {}, "~" , false},

    Rule {rules::binaryop {}, "-" , true},
    Rule {rules::binaryop {}, "*" , true},
    Rule {rules::binaryop {}, "/" , true},
    Rule {rules::binaryop {}, "%" , true},
    Rule {rules::binaryop {}, "==", true},
    Rule {rules::binaryop {}, "!=", true},
    Rule {rules::binaryop {}, ">" , true},
    Rule {rules::binaryop {}, "<" , true},
    Rule {rules::binaryop {}, ">=", true},
    Rule {rules::binaryop {}, "<=", true},
    Rule {rules::binaryop {}, "&&", true},
    Rule {rules::binaryop {}, "||", true},
    Rule {rules::binaryop {}, "&" , true},
    Rule {rules::binaryop {}, "|" , true},
    Rule {rules::binaryop {}, "^" , true},

    Rule {rules::binaryop {}, ""       , false},
    Rule {rules::binaryop {}, "${a}"   , false},
    Rule {rules::binaryop {}, "int{42}", false}
));

INSTANTIATE_TEST_SUITE_P(Unary, PegtlRuleParse, Values
(
    Rule {rules::unary {}, "-int{42}" , true },
    Rule {rules::unary {}, "! null"    , true },
    Rule {rules::unary {}, "~\n${var}"  , true },

    Rule {rules::unary {}, "*null"    , false},
    Rule {rules::unary {}, "(!null)"  , false},
    Rule {rules::unary {}, "~(null)"  , false},
    Rule {rules::unary {}, "null+null", false}
));

INSTANTIATE_TEST_SUITE_P(Binary, PegtlRuleParse, Values
(
    Rule {rules::binary {}, "null+null"          , true },
    Rule {rules::binary {}, "null* int{42}"      , true },
    Rule {rules::binary {}, "${a} /null"         , true },

    Rule {rules::binary {}, "int{42}%double{4.2}", true },
    Rule {rules::binary {}, "int{42}== null"     , true },
    Rule {rules::binary {}, "${a} !=int{42}"     , true },

    Rule {rules::binary {}, "${a}>${b}"          , true },
    Rule {rules::binary {}, "${a}<= null"        , true },
    Rule {rules::binary {}, "int{42} ||${a}"     , true },

    Rule {rules::binary {}, "(null+null)"        , false},
    Rule {rules::binary {}, "(null + ~null)"     , false},
    Rule {rules::binary {}, "(~null)"            , false},
    Rule {rules::binary {}, "-int{42}"           , false}
));

INSTANTIATE_TEST_SUITE_P(Arithmetic, PegtlRuleParse, Values
(
    Rule {rules::arithmetic {}, "(!null)"         , true },
    Rule {rules::arithmetic {}, "(null ^ null)"   , true },
    Rule {rules::arithmetic {}, "(null ^ (!null))", true },

    Rule {rules::arithmetic {}, "!null"           , false},
    Rule {rules::arithmetic {}, "null ^ null"     , false}
));

INSTANTIATE_TEST_SUITE_P(Ternary, PegtlRuleParse, Values
(
    Rule {rules::ternary {}, "if(null,null,null)"                                  , true },
    Rule {rules::ternary {}, "if (${a},int{42},null)"                              , true },
    Rule {rules::ternary {}, "if ( int{42},null , null)"                           , true },
    Rule {rules::ternary {}, "if((-int{42}) ,${a},null )"                          , true },
    Rule {rules::ternary {}, "if(if(null,null,null), if (${a} , ${b} ,${c}),null )", true },

    Rule {rules::ternary {}, "if(null, null, null,)", false},
    Rule {rules::ternary {}, "if(null, null null)"  , false}
));

INSTANTIATE_TEST_SUITE_P(Expression, PegtlRuleParse, Values
(
    Rule {rules::grammar {}, "(${a} + (-int{42}))", true}
));



TEST(PegtlRuleParse, Scratchpad)
//TEST(PegtlRuleParse, DISABLED_Scratchpad)
{
    using namespace tao::pegtl;

    if (analyze<rules::grammar>() != 0)
    {
        FAIL() << "Grammar is invalid";
    }

    const auto *expr = "(${a} + (-int{42}))";
    memory_input input(expr, "test expression");
    std::ostringstream ostr;
    auto actual = parse<rules::grammar, actions::DryRun>(input, ostr);

    EXPECT_TRUE(actual);
    if (HasFailure())
    {
        standard_trace<rules::grammar>(input);
    }

    FAIL() << "Parsing expression: \"" << expr << "\":\n" << ostr.str();
}


} // namespace pmql::parser
