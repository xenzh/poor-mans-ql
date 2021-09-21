#include "../pmql/expression.h"
#include "../pmql/store.h"

#include <gtest/gtest.h>

#include <iostream>


template<typename T> struct Name;
template<> struct Name<int > { static constexpr std::string_view value = "int" ; };
template<> struct Name<bool> { static constexpr std::string_view value = "bool"; };

using Value = pmql::Variant<Name, int, bool>;


/// ((a + b) > 0) ? (a + b - 42) : (a + b + null)
///
/// Showcases:
/// * variables (+ 3 different ways of substitutions)
/// * constants
/// * (nested) operators
/// * conditions
/// * null handling
/// * streamability, evaluation log.
TEST(Example, ExpressionShowcase)
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
