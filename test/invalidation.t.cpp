#include "../pmql/results.h"

#include "../pmql/store.h"
#include "../pmql/builder.h"
#include "../pmql/expression.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace {


using namespace ::testing;


template<typename T> struct Name;
template<> struct Name<int > { [[maybe_unused]] static constexpr std::string_view value = "int" ; };
template<> struct Name<bool> { [[maybe_unused]] static constexpr std::string_view value = "bool"; };

using V = pmql::Variant<Name, int, bool>;


template<typename B> using Expr = pmql::Result<typename std::decay_t<B>::value_type>;


} // unnamed namespace


/// Check invalidation map of variable a for expression (-42 + -a);
TEST(Invalidations, InvalidationSingle)
{
    auto build = [] (auto &&builder) -> Expr<decltype(builder)>
    {

        auto _42 = Try(builder.constant(42));
        auto a   = Try(builder.var("a"));
        auto na  = Try(builder.template op<std::negate>(a));
        auto n42 = Try(builder.template op<std::negate>(_42));

        Try(builder.template op<std::plus>(n42, na));

        return std::move(builder)();
    };

    const auto expr = TryThrow(build(pmql::builder<V>()));
    const auto &ingredients = expr.ingredients();

    const auto invs = pmql::op::bitmap::invalidations(ingredients.ops, /* invert */ false);

    ASSERT_EQ(1, invs.size());
    ASSERT_THAT(invs.front(), ElementsAre(false, true, true, false, true));
}

/// Check invalidation maps for variables a and b for expresion ((-42 + -a) - b)
TEST(Invalidations, InvalidationDouble)
{
    auto build = [] (auto &&builder) -> Expr<decltype(builder)>
    {
        auto _42  = Try(builder.constant(42));
        auto a    = Try(builder.var("a"));
        auto na   = Try(builder.template op<std::negate>(a));
        auto n42  = Try(builder.template op<std::negate>(_42));

        auto ap42 = Try(builder.template op<std::plus>(n42, na));

        auto b   = Try(builder.var("b"));
        Try(builder.template op<std::minus>(ap42, b));

        return std::move(builder)();
    };

    const auto expr = TryThrow(build(pmql::builder<V>()));
    const auto &ingredients = expr.ingredients();

    const auto invs = pmql::op::bitmap::invalidations(ingredients.ops, /* invert */ false);

    ASSERT_EQ(2, invs.size());
    ASSERT_THAT(invs.front(), ElementsAre(false, true , true , false, true , false, true));
    ASSERT_THAT(invs.back (), ElementsAre(false, false, false, false, false, true , true));
}
