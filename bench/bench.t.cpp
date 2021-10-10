#include "../pmql/expression.h"
#include "../pmql/store.h"

#include <benchmark/benchmark.h>


namespace pmql {
namespace {


template<typename T> struct Name;
template<> struct Name<int>    { [[maybe_unused]] static constexpr std::string_view value = "int"   ; };
template<> struct Name<double> { [[maybe_unused]] static constexpr std::string_view value = "double"; };


using SingleInt    = pmql::Single<Name, int   >;
using SingleDouble = pmql::Single<Name, double>;

using VariantInt       = pmql::Variant<Name, int        >;
using VariantDouble    = pmql::Variant<Name, double     >;
using VariantIntDouble = pmql::Variant<Name, int, double>;


template<typename Expr, typename Ctx>
void passcheck(benchmark::State &state, const Expr &expr, Ctx &context)
{
    auto result = expr(context);
    if (!result)
    {
        state.SkipWithError(result.error().description().c_str());
    }
}


void params(benchmark::internal::Benchmark *settings)
{
    settings->Range(100000, 100000);
    settings->Unit(benchmark::kMillisecond);
}


template<typename Store>
void avgOfThreeNegated(Builder<Store> &builder)
{
    const auto a   = *builder.var("a");
    const auto b   = *builder.var("b");
    const auto c   = *builder.var("c");

    const auto na  = *builder.template op<std::negate>(a);
    const auto nb  = *builder.template op<std::negate>(b);
    const auto nc  = *builder.template op<std::negate>(c);

    const auto ab  = *builder.template op<std::plus>(na, nb);
    const auto abc = *builder.template op<std::plus>(ab, nc);
    const auto cnt = *builder.constant(3.0);

    builder.template op<std::divides>(abc, cnt);
}


} // unnamed namespace


void SingleConst_Native(benchmark::State &state)
{
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            benchmark::DoNotOptimize(42);
        }
    }
}

template<typename Store>
void SingleConst_Pmql(benchmark::State &state)
{
    Builder<Store> builder;
    builder.constant(42);

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>();

    passcheck(state, expr, context);
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK(SingleConst_Native)->Apply(params);
BENCHMARK_TEMPLATE(SingleConst_Pmql, SingleInt       )->Apply(params);
BENCHMARK_TEMPLATE(SingleConst_Pmql, VariantInt      )->Apply(params);
BENCHMARK_TEMPLATE(SingleConst_Pmql, VariantIntDouble)->Apply(params);


void VarPlusConstFixed_Native(benchmark::State &state)
{
    int a = 42;

    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            benchmark::DoNotOptimize(a + 42);
        }
    }
}

template<typename Store>
void VarPlusConstFixed_Pmql(benchmark::State &state)
{
    Builder<Store> builder;
    {
        auto a = *builder.var("a");
        auto b = *builder.constant(42);
        builder.template op<std::plus>(a, b);
    }

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>();
    context("a")->get() = 42;

    passcheck(state, expr, context);
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK(VarPlusConstFixed_Native)->Apply(params);
BENCHMARK_TEMPLATE(VarPlusConstFixed_Pmql, SingleInt       )->Apply(params);
BENCHMARK_TEMPLATE(VarPlusConstFixed_Pmql, VariantInt      )->Apply(params);
BENCHMARK_TEMPLATE(VarPlusConstFixed_Pmql, VariantIntDouble)->Apply(params);


void VarPlusConstParam_Native(benchmark::State &state)
{
    int a = 0;

    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            a = 42;
            benchmark::DoNotOptimize(a + 42);
        }
    }
}

template<typename Store>
void VarPlusConstParam_Pmql(benchmark::State &state)
{
    Builder<Store> builder;
    {
        auto a = *builder.var("a");
        auto b = *builder.constant(42);
        builder.template op<std::plus>(a, b);
    }

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>();

    auto &var = *context.find("a");

    var = 1;
    passcheck(state, expr, context);

    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            var = 42;
            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK(VarPlusConstParam_Native)->Apply(params);
BENCHMARK_TEMPLATE(VarPlusConstParam_Pmql, SingleInt       )->Apply(params);
BENCHMARK_TEMPLATE(VarPlusConstParam_Pmql, VariantInt      )->Apply(params);
BENCHMARK_TEMPLATE(VarPlusConstParam_Pmql, VariantIntDouble)->Apply(params);


void AvgOfThree_Native(benchmark::State &state)
{
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            double a = 22.2;
            double b = 42.2;
            double c = 82.2;

            auto avg = ((a + b) + c) / 3;
            benchmark::DoNotOptimize(avg);
        }
    }
}

template<typename Store>
void AvgOfThree_Pmql(benchmark::State &state)
{
    Builder<Store> builder;
    avgOfThreeNegated(builder);

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>();

    auto &a = context("a")->get();
    auto &b = context("b")->get();
    auto &c = context("c")->get();

    a = 1.1;
    b = 2.2;
    c = 3.3;
    passcheck(state, expr, context);

    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            a = 22.2;
            b = 42.2;
            c = 82.2;

            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK(AvgOfThree_Native)->Apply(params);
BENCHMARK_TEMPLATE(AvgOfThree_Pmql, SingleDouble    )->Apply(params);
BENCHMARK_TEMPLATE(AvgOfThree_Pmql, VariantDouble   )->Apply(params);
BENCHMARK_TEMPLATE(AvgOfThree_Pmql, VariantIntDouble)->Apply(params);


template<typename Store>
void AvgOfThree_Cache_Disabled(benchmark::State &state)
{
    Builder<Store> builder;
    avgOfThreeNegated(builder);

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>(/* cache */ false);

    auto &a = context("a")->get();
    auto &b = context("b")->get();
    auto &c = context("c")->get();

    for (auto _ : state)
    {
        a = 22.2;
        b = 42.2;
        c = 82.2;

        for (int i = 0; i < state.range(0); ++i)
        {
            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK_TEMPLATE(AvgOfThree_Cache_Disabled, VariantIntDouble)->Apply(params);


template<typename Store>
void AvgOfThree_Cache_Enabled1(benchmark::State &state)
{
    Builder<Store> builder;
    avgOfThreeNegated(builder);

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>(/* cache */ true);

    auto &a = context("a")->get();
    auto &b = context("b")->get();
    auto &c = context("c")->get();

    for (auto _ : state)
    {
        a = 22.2;

        for (int i = 0; i < state.range(0); ++i)
        {
            b = 42.2;
            c = 82.2;

            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK_TEMPLATE(AvgOfThree_Cache_Enabled1, VariantIntDouble)->Apply(params);


template<typename Store>
void AvgOfThree_Cache_Enabled2(benchmark::State &state)
{
    Builder<Store> builder;
    avgOfThreeNegated(builder);

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>(/* cache */ true);

    auto &a = context("a")->get();
    auto &b = context("b")->get();
    auto &c = context("c")->get();

    for (auto _ : state)
    {
        a = 22.2;
        b = 42.2;

        for (int i = 0; i < state.range(0); ++i)
        {
            c = 82.2;

            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK_TEMPLATE(AvgOfThree_Cache_Enabled2, VariantIntDouble)->Apply(params);


template<typename Store>
void AvgOfThree_Cache_Enabled3(benchmark::State &state)
{
    Builder<Store> builder;
    avgOfThreeNegated(builder);

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>(/* cache */ true);

    auto &a = context("a")->get();
    auto &b = context("b")->get();
    auto &c = context("c")->get();

    for (auto _ : state)
    {
        a = 22.2;
        b = 42.2;
        c = 82.2;

        for (int i = 0; i < state.range(0); ++i)
        {
            benchmark::DoNotOptimize(expr(context));
        }
    }
}

BENCHMARK_TEMPLATE(AvgOfThree_Cache_Enabled3, VariantIntDouble)->Apply(params);


} // namespace pmql
