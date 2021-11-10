#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/trace.hpp>

#include <gtest/gtest.h>


namespace pmql {
namespace rules {


using namespace tao::pegtl;


// Grammar:
//
// type: int
// value: 42 (anything!)
// const: int{any} OR null
// var: ${any}
// unary: optoken
// binary: (token op token)
// ternary: if(token, token, token)
// extension @avail(token, token, token)


struct type : identifier
{
    static constexpr std::string_view name = "type";
};

struct value : plus<not_one<'{', '}'>>
{
    static constexpr std::string_view name = "value";
};

struct store : seq<type, one<'{'>, value, one<'}'>>
{
    static constexpr std::string_view name = "store";
};

struct null : TAO_PEGTL_STRING("null")
{
    static constexpr std::string_view name = "null";
};

struct constant : sor<store, null>
{
    static constexpr std::string_view name = "constant";
};

struct variable : seq<one<'$'>, one<'{'>, value, one<'}'>>
{
    static constexpr std::string_view name = "variable";
};

struct token;

struct brackets : seq<one<'('>, token, one<')'>>
{
    static constexpr std::string_view name = "brackets";
};

struct grammar : constant
{
    static constexpr std::string_view name = "grammar";
};


} // namespace rules


namespace actions {


using namespace tao::pegtl;


//template<typename B> struct Builder
//{
//    struct Const
//    {
//        std::string_view type;
//        std::string_view value;
//    };

//    B builder;
//};


//template<typename Rule> struct action : nothing<Rule> {};

//template<> struct action<rules::type>
//{
//    template<typename ActionInput>
//    void apply(const ActionInput &input, std::string &out)
//    {
//        out += input.string();
//    }
//};


template<typename Rule> struct action
{
    template<typename In>
    static void apply(const In &input, std::ostream &os)
    {
        static size_t idx = 0;
        os << idx++ << ": " << input.string() << "\n";
    }
};


} // namespace actions


namespace {


using namespace ::testing;


struct Rule
{
    using rule_t = std::variant<
        rules::type,
        rules::value,
        rules::store,
        rules::null,
        rules::constant,
        rules::variable
    >;

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
            os << rule.name << ", ";
        },
        rule.rule);

    os << "'" << rule.sample << "'" << ": " << (rule.outcome ? "match" : "no match");
    return os << "]";
}


} // unnamed namespace


struct RuleParse : TestWithParam<Rule> {};


TEST_P(RuleParse, Sample)
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


INSTANTIATE_TEST_SUITE_P(Type, RuleParse, Values
(
    Rule {rules::type {}, "int"                 , true },
    Rule {rules::type {}, "valid_c_Identifier69", true },
    Rule {rules::type {}, "  "                  , false}
));

INSTANTIATE_TEST_SUITE_P(Value, RuleParse, Values
(
    Rule {rules::value {}, "42"                , true },
    Rule {rules::value {}, "  anything $! G0ES", true },
    Rule {rules::value {}, ""                  , false}
));

INSTANTIATE_TEST_SUITE_P(Store, RuleParse, Values
(
    Rule {rules::store {}, "int{42}"       , true },
    Rule {rules::store {}, "_c11_ident{_'}", true },
    Rule {rules::store {}, "int(42)"       , false},
    Rule {rules::store {}, "int{}"         , false},
    Rule {rules::store {}, "int{"          , false},
    Rule {rules::store {}, "int{{}"        , false},
    Rule {rules::store {}, " int{42}"      , false}
));

INSTANTIATE_TEST_SUITE_P(Null, RuleParse, Values
(
    Rule {rules::null {}, "null"    , true },
    Rule {rules::null {}, "nullnull", true },
    Rule {rules::null {}, "nullptr" , true },
    Rule {rules::null {}, "Null"    , false},
    Rule {rules::null {}, "nulL"    , false}
));

INSTANTIATE_TEST_SUITE_P(Constant, RuleParse, Values
(
    Rule {rules::constant {}, "null"                  , true },
    Rule {rules::constant {}, "double{31.32}"         , true },
    Rule {rules::constant {}, "string{'hello quoted'}", true },
    Rule {rules::constant {}, "+null"                 , false},
    Rule {rules::constant {}, "42(32)"                , false}
));

INSTANTIATE_TEST_SUITE_P(Variable, RuleParse, Values
(
    Rule {rules::variable {}, "${a}"                , true },
    Rule {rules::variable {}, "${ anything $! G0ES}", true },
    Rule {rules::variable {}, "${}"                 , false},
    Rule {rules::variable {}, "${"                  , false},
    Rule {rules::variable {}, "${{}"                , false}
));



TEST(RuleParse, DISABLED_Scratchpad)
//TEST(RuleParse, Scratchpad)
{
    using namespace tao::pegtl;

    struct test : one<'}'> {};

    memory_input input("}", "test parameter");

    if (!analyze<test>())
    {
        FAIL() << "Grammar is invalid";
    }

    auto actual = parse<test, nothing>(input);

    EXPECT_TRUE(actual);
    if (HasFailure())
    {
        standard_trace<test>(input);
    }
}


} // namespace pmql
