#include "../pmql/expression.h"
#include "../pmql/store.h"
#include "../pmql/functions.h"

#include <gtest/gtest.h>

#include <iostream>


namespace {


template<typename T> struct Name;
template<> struct Name<int > { static constexpr std::string_view value = "int" ; };
template<> struct Name<bool> { static constexpr std::string_view value = "bool"; };

using Value = pmql::Variant<Name, int, bool>;


} // unnamed namespace


/// ((a + b) > 0) ? (a + b - 42) : (a + b + null)
///
/// Showcases:
/// * variables (+ 3 different ways of substitutions)
/// * constants
/// * (nested) operators
/// * conditions
/// * null handling
/// * streamability, evaluation log.
TEST(Showcase, Expression)
{
    using namespace pmql;

    std::cout << "Here be dragons:\n\n";

    Builder<Value> builder;
    {
        const auto a   = *builder.var("a");
        const auto b   = *builder.var("b");
        const auto c42 = *builder.constant(42);
        const auto c0  = *builder.constant(0);
        const auto cn  = *builder.constant(null {});

        const auto ab = *builder.op<std::plus>(a, b);

        const auto abp42 = *builder.op<std::plus >(ab, cn);
        const auto abm42 = *builder.op<std::minus>(ab, c42);
        const auto abg0  = *builder.op<std::greater>(ab, c0);

        builder.branch(abg0, abm42, abp42);
    }

    std::cout << "-- Builder:\n" << builder << "\n";

    auto expr = std::move(builder)();
    if (!expr.has_value())
    {
        FAIL() << "-- Builder error: " << expr.error();
    }

    std::cout << "-- Expression: " << *expr << "\n\n";

    auto context = expr->context<Value>();
    std::cout << "-- Context before:\n" << context << "\n";

    for (auto &var : context)
    {
        var = 11;
    }
    context("b")->get() = 77;

    auto result = (*expr)(context);

    std::cout << "-- Result with a=11, b=77: " << result << "\n\n";
    expr->log(std::cout << "-- Log with a=11, b=77:\n", context) << "\n";

    context[0] = -20;
    context[1] = 13;

    result = (*expr)(context);

    std::cout << "-- Result with a=-20, b=13: " << result << "\n\n";
    expr->log(std::cout << "-- Log with a=-20, b=13:\n", context) << "\n";
}

// avail(<null>, a, b)
// * Extension function pipeline
// * Avail() builtin
TEST(Showcase, Extensions)
{
    auto builder = pmql::builder<Value>(pmql::ext::builtin());
    {
        auto a = *builder.constant(pmql::null {});
        auto b = *builder.var("b");
        auto c = *builder.var("c");

        auto result = builder.fun("avail", a, b, c);
        ASSERT_TRUE(result) << result.error();
    }

    auto expr = std::move(builder)();
    ASSERT_TRUE(expr) <<expr.error();

    auto context = expr->context<Value>();

    auto &b = context("b")->get();
    auto &c = context("c")->get();

    // avail(null, null, 42)
    {
        b = pmql::null {};
        c = 42;

        auto result = (*expr)(context);
        ASSERT_TRUE(result) << result.error();
        result.value()([] (const auto &value)
        {
            ASSERT_EQ(42, value);
        });
    }

    // avail(null, 21, 42)
    {
        b = 21;
        c = 42;

        auto result = (*expr)(context);
        ASSERT_TRUE(result) << result.error();
        result.value()([] (const auto &value)
        {
            ASSERT_EQ(21, value);
        });
    }

    expr->log(std::cout << "-- Log:\n", context) << "\n";
}

//TEST(Showcase, Serialization)
//{
//    auto builder = pmql::builder<Value>(pmql::ext::builtin());
//    {
//        auto a  = *builder.constant(pmql::null {});
//        auto b  = *builder.var("b");
//        auto c  = *builder.var("name space");
//        auto d  = *builder.constant(42);
//        auto nb = *builder.op<std::negate>(b);
//        auto ab = *builder.op<std::plus>(a, nb);

//        auto nn = *builder.branch(a, b, c);
//        builder.fun("avail", a, ab, nn, d);
//    }

//    auto result = std::move(builder)();
//    ASSERT_TRUE(result.has_value()) << result;
//    auto expr = *std::move(result);

//    const auto stored = expr.ingredients().store();
//    std::cout << "\nSerialized expression:\n\n" << *stored << "\n\n";
//}
