#include <iostream>

#include "expression.h"
#include "store.h"


// TODO:
// * More intuitive builder?..
// * Benchmarks (visit vs virtual?..)
// * cmake
// * unit tests


template<typename T> struct Name;
template<> struct Name<int> { static constexpr std::string_view value = "int"; };

using Value = pmql::Store<Name, int>;


int main()
{
    using namespace pmql;

    std::cout << "Here be dragons:\n\n";

    Builder<Value> builder;
    {
        const auto a   = *builder.var("a");
        const auto b   = *builder.var("b");
        const auto c42 = *builder.constant(42);

        const auto ab = *builder.op<std::plus>(a, b);
        builder.op<std::minus>(ab, c42);
    }

    std::cout << "-- Builder:\n" << builder << "\n";

    auto expr = std::move(builder)();
    if (!expr.has_value())
    {
        std::cout << "-- Builder error: " << expr.error();
        return 1;
    }

    std::cout << "-- Expression: " << *expr << "\n\n";

    auto context = expr->context<Value>();
    for (auto &var : context)
    {
        var = 11;
    }

    context("b")->get() = 77;

    std::cout << "-- Context before:\n" << context << "\n";

    auto result = (*expr)(context);

    std::cout << "-- Result: " << result << "\n\n";
    expr->log(std::cout << "-- Log:\n", context) << "\n";

    return 0;
}
