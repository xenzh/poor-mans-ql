# Poor man's query language

[![example workflow](https://github.com/xenzh/poor-mans-ql/actions/workflows/cmake.yml/badge.svg)](https://github.com/xenzh/poor-mans-ql/actions/workflows/cmake.yml)

This library allows to build and evaluate formula expressions based on std-like functional objects. Type system is data-driven: library borrows allowed types from object storage, provided by the client, evaluation result types are inferred from variable substitutions.

* Operations are stored by flattening a tree into a list (leaves first, root is the last element).
* This list is validated and stored inside the `Expression` object.
* In order to run a calculation, client must allocate a `Context` object, that contains all necessary "moving parts": variables, their substitutions and results for every operation.
* Storing results for every calculation allows to
    * Enable shortcut evaluation (i.e. using stored results instead of calculating them from scratch when running the expression for the second time);
    * provide step-by-step calcualtion log for troubleshooting.

## Features

- [x] Builtin support for all std operator objects from `<functional>`.
- [x] Typed constants and untyped variables.
- [x] Flow control: `if` operator.
- [x] Value nullability.
- [x] Step-by-step evaluation log.
- [x] External function support (i.e. `avail()`).
- [x] Evaluation results caching / partial invalidation based on variable substitutions.
- [ ] String serialization.

### Code TODOs:

- [ ] Proper unit tests.
- [ ] More detailed benchmarks.

## Build

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
ninja

./test/pmql_test
./bench/pmql_bench
```

## Example

```cpp
#include <pmql/expression.h>
#include <pmql/store.h>


// Define storage for calculated values.

template<typename T> struct Name;
template<> struct Name<int    > { [[maybe_unused]] static constexpr std::string_view value = "int"   ; };
template<> struct Name<idouble> { [[maybe_unused]] static constexpr std::string_view value = "double"; };

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

Exceptional performance is not really a goal of this projects, so the benchmarks below are mostly useful for demonstrating and comparing complexities of different scenarios.

```
Running ./bench/pmql_bench
Run on (12 X 2592.01 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 12288 KiB (x1)
Load Average: 0.35, 0.34, 0.29
---------------------------------------------------------------------------------------------
Benchmark                                                   Time             CPU   Iterations
---------------------------------------------------------------------------------------------
SingleConst_Native/100000                               0.513 ms        0.513 ms         1324
SingleConst_Pmql<SingleInt>/100000                       7.87 ms         7.87 ms           88
SingleConst_Pmql<VariantInt>/100000                      7.88 ms         7.88 ms           91
SingleConst_Pmql<VariantIntDouble>/100000                7.69 ms         7.69 ms           89
VarPlusConstFixed_Native/100000                         0.519 ms        0.519 ms         1336
VarPlusConstFixed_Pmql<SingleInt>/100000                 7.69 ms         7.69 ms           90
VarPlusConstFixed_Pmql<VariantInt>/100000                7.68 ms         7.68 ms           90
VarPlusConstFixed_Pmql<VariantIntDouble>/100000          7.75 ms         7.75 ms           90
VarPlusConstParam_Native/100000                         0.557 ms        0.557 ms         1286
VarPlusConstParam_Pmql<SingleInt>/100000                 58.3 ms         58.3 ms           12
VarPlusConstParam_Pmql<VariantInt>/100000                89.0 ms         89.0 ms            8
VarPlusConstParam_Pmql<VariantIntDouble>/100000          90.1 ms         90.1 ms            8
AvgOfThree_Native/100000                                0.617 ms        0.617 ms         1125
AvgOfThree_Pmql<SingleDouble>/100000                      222 ms          222 ms            3
AvgOfThree_Pmql<VariantDouble>/100000                     354 ms          354 ms            2
AvgOfThree_Pmql<VariantIntDouble>/100000                  352 ms          352 ms            2
AvgOfThree_Cache_Disabled<VariantIntDouble>/100000        348 ms          348 ms            2
AvgOfThree_Cache_Enabled1<VariantIntDouble>/100000        285 ms          285 ms            3
AvgOfThree_Cache_Enabled2<VariantIntDouble>/100000        163 ms          163 ms            4
AvgOfThree_Cache_Enabled3<VariantIntDouble>/100000       7.77 ms         7.77 ms           89
```

