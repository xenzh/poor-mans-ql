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



//template<typename T>
//void fail(benchmark::State &state, const Result<T> &result)
//{
//    state.SkipWithError(result.error().description().c_str());
//}


void params(benchmark::internal::Benchmark *settings)
{
    settings->Range(10000, 10000);
    settings->Unit(benchmark::kMillisecond);
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
    {
        const auto a   = *builder.var("a");
        const auto b   = *builder.var("b");
        const auto c   = *builder.var("c");
        const auto ab  = *builder.template op<std::plus>( a, b);
        const auto abc = *builder.template op<std::plus>(ab, c);
        const auto cnt = *builder.constant(3.0);

        builder.template op<std::divides>(abc, cnt);
    }

    auto expr = *std::move(builder)();
    auto context = expr.template context<Store>();

    auto &a = context("a")->get();
    auto &b = context("b")->get();
    auto &c = context("c")->get();

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
//BENCHMARK_TEMPLATE(AvgOfThree_Pmql, SingleDouble    )->Apply(params);
//BENCHMARK_TEMPLATE(AvgOfThree_Pmql, VariantDouble   )->Apply(params);
//BENCHMARK_TEMPLATE(AvgOfThree_Pmql, VariantIntDouble)->Apply(params);


} // namespace pmql
