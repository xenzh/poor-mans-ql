# Poor man's query language

This library allows to build and evaluate formula expressions based on std-like functional objects. Type system is data-driven: library borrows allowed types from object storage, provided by the client, evaluation result types are inferred from variable substitutions.

## Features

- [x] Builtin support for all std operator objects from `<functional>`.
- [x] Typed constants and untyped variables.
- [x] Flow control: `if` operator.
- [x] Value nullability.
- [x] Step-by-step evaluation log.
- [x] External function support (i.e. `avail()`).
- [ ] String serialization.
- [ ] Evaluation caching / partial invalidation based on variable substitutions.

### Code TODOs:

- [ ] Proper unit tests.
- [ ] More detailed benchmarks.

## Build

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
ninja
```

## Example

```cpp
#include <pmql/expression.h>
#include <pmql/store.h>


// Define storage for calculated values.

template<typename T> struct Name;
template<> struct Name<int    > { static constexpr std::string_view value = "int"   ; };
template<> struct Name<idouble> { static constexpr std::string_view value = "double"; };

using Value = pmql::Variant<Name, int, double>;


pmql::Result<void> test()
{
    // Build an expression : (a + 42) / b

    auto builder = pmql::builder<Value>();
    {
        auto a    = *builder.var("a");
        auto b    = *builder.var("b");
        auto c42  = *builder.constant(42);

        auto ac42 = *builder.op<std::plus>(1, c42);
        builder.op<std::divides>(ac42, b);
    }

    auto result = std::move(builder)();
    if (!result)
    {
        return pmql::error(result.error());
    }

    auto expr = *std::move(result);

    // Do variable substitutions, evaluate the expression.

    auto context = expr.context<Value>();
    auto &a = context("b")->get();
    auto &b = context("v")->get();

    a = 11.2;
    b = 99;
    auto evaluated = expr(context);

    if (!evaluated)
    {
        return pmql::error(evaluated.error());
    }

    evaluated.value()([] (const auto &value)
    {
        std::cout << "(a + b) / 42 [with a=11.2, b=99] = " << value << std::endl;
    });
}
```

See [these showcase unit tests](./test/example.t.cpp) for more features.

## Performance

This version is more of a proof of concept, and benchmarks do not look too inspiring (100k rows, Release):

```
Running ./bench/pmql_bench
Run on (12 X 2592.01 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 12288 KiB (x1)
Load Average: 0.62, 0.51, 0.36
------------------------------------------------------------------------------------------
Benchmark                                                Time             CPU   Iterations
------------------------------------------------------------------------------------------
SingleConst_Native/100000                            0.024 ms        0.024 ms        27924
SingleConst_Pmql<SingleInt>/100000                   0.722 ms        0.722 ms          959
SingleConst_Pmql<VariantInt>/100000                  0.810 ms        0.810 ms          866
SingleConst_Pmql<VariantIntDouble>/100000             1.07 ms         1.07 ms          653
VarPlusConstFixed_Native/100000                      0.024 ms        0.024 ms        27947
VarPlusConstFixed_Pmql<SingleInt>/100000              1.89 ms         1.89 ms          362
VarPlusConstFixed_Pmql<VariantInt>/100000             2.35 ms         2.35 ms          293
VarPlusConstFixed_Pmql<VariantIntDouble>/100000       3.15 ms         3.15 ms          222
VarPlusConstParam_Native/100000                      0.025 ms        0.025 ms        28387
VarPlusConstParam_Pmql<SingleInt>/100000              1.95 ms         1.95 ms          362
VarPlusConstParam_Pmql<VariantInt>/100000             2.41 ms         2.41 ms          292
VarPlusConstParam_Pmql<VariantIntDouble>/100000       3.20 ms         3.20 ms          219
AvgOfThree_Native/100000                             0.031 ms        0.031 ms        22855
AvgOfThree_Pmql<SingleDouble>/100000                  7.04 ms         7.04 ms           99
AvgOfThree_Pmql<VariantDouble>/100000                 7.61 ms         7.61 ms           91
AvgOfThree_Pmql<VariantIntDouble>/100000              7.72 ms         7.72 ms           87
```

