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


} // unnamed namespace


/// Check invalidation map of variable a for expression (-42 + -a);
TEST(Invalidations, InvalidationSingle)
{
    auto builder = pmql::builder<V>();
    {
        auto _42 = *builder.constant(42);
        auto a   = *builder.var("a");
        auto na  = *builder.op<std::negate>(a);
        auto n42 = *builder.op<std::negate>(_42);

        builder.op<std::plus>(n42, na);
    }

    const auto expr = *std::move(builder)();
    const auto &ingredients = expr.ingredients();

    const auto invs = pmql::op::bitmap::invalidations(ingredients.ops, /* invert */ false);

    ASSERT_EQ(1, invs.size());
    ASSERT_THAT(invs.front(), ElementsAre(false, true, true, false, true));
}

/// Check invalidation maps for variables a and b for expresion ((-42 + -a) - b)
TEST(Invalidations, InvalidationDouble)
{
    auto builder = pmql::builder<V>();
    {
        auto _42  = *builder.constant(42);
        auto a    = *builder.var("a");
        auto na   = *builder.op<std::negate>(a);
        auto n42  = *builder.op<std::negate>(_42);

        auto ap42 = *builder.op<std::plus>(n42, na);

        auto b   = *builder.var("b");
        builder.op<std::minus>(ap42, b);
    }

    const auto expr = *std::move(builder)();
    const auto &ingredients = expr.ingredients();

    const auto invs = pmql::op::bitmap::invalidations(ingredients.ops, /* invert */ false);

    ASSERT_EQ(2, invs.size());
    ASSERT_THAT(invs.front(), ElementsAre(false, true , true , false, true , false, true));
    ASSERT_THAT(invs.back (), ElementsAre(false, false, false, false, false, true , true));
}
