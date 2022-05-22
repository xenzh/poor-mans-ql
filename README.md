# Poor man's query language

[![cmake](https://github.com/xenzh/poor-mans-ql/actions/workflows/cmake.yml/badge.svg)](https://github.com/xenzh/poor-mans-ql/actions/workflows/cmake.yml)
[![sphinx](https://github.com/xenzh/poor-mans-ql/actions/workflows/sphinx.yml/badge.svg)](https://github.com/xenzh/poor-mans-ql/actions/workflows/sphinx.yml)
[![pages](https://img.shields.io/badge/sphinx-documentation-informational)](https://xenzh.github.io/poor-mans-ql/)

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

- [ ] PEG: add custom rule for extensions.
- [ ] PEG: implement expression parser.
- [ ] default serialization in pmql lib.
- [ ] type mismatch unit tests
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

TODO:
- [ ] log with sample output in comments

```cpp
#include <pmql/expression.h>
#include <pmql/store.h>


// Define storage for calculated values.

template<typename T> struct Name;
template<> struct Name<int   > { [[maybe_unused]] static constexpr std::string_view value = "int"   ; };
template<> struct Name<double> { [[maybe_unused]] static constexpr std::string_view value = "double"; };

using Value = pmql::Variant<Name, int, double>;


pmql::Result<void> test()
{
    // Build an expression : (a + 42) / b

    auto builder = pmql::builder<Value>();
    {
        auto a    = Try(builder.var("a"));
        auto b    = Try(builder.var("b"));
        auto c42  = Try(builder.constant(42));

        auto ac42 = Try(builder.op<std::plus>(a, c42));
        Try(builder.op<std::divides>(ac42, b));
    }

    auto expr = Try(std::move(builder)());

    // Do variable substitutions, evaluate the expression.

    auto context = expr.context<Value>();
    auto &a = context("b")->get();
    auto &b = context("v")->get();

    a = 11.2;
    b = 99;

    auto evaluated = Try(expr(context));

    evaluated([] (const auto &value)
    {
        std::cout << "(a + b) / 42 [with a=11.2, b=99] = " << value << std::endl;
    });
}
```

See [these showcase unit tests](./test/example.t.cpp) for more features.

## Performance

Exceptional performance is not really a goal of this project, so the benchmarks below are mostly useful for demonstrating and comparing complexities of different scenarios.

```
Running ./bench/pmql_bench
Run on (12 X 2592.01 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 12288 KiB (x1)
Load Average: 0.80, 0.29, 0.10
---------------------------------------------------------------------------------------------
Benchmark                                                   Time             CPU   Iterations
---------------------------------------------------------------------------------------------
SingleConst_Native/100000                               0.024 ms        0.024 ms        28117
SingleConst_Pmql<SingleInt>/100000                      0.357 ms        0.357 ms         1922
SingleConst_Pmql<VariantInt>/100000                     0.356 ms        0.356 ms         1950
SingleConst_Pmql<VariantIntDouble>/100000               0.355 ms        0.355 ms         1941
VarPlusConstFixed_Native/100000                         0.025 ms        0.025 ms        28187
VarPlusConstFixed_Pmql<SingleInt>/100000                0.353 ms        0.353 ms         1949
VarPlusConstFixed_Pmql<VariantInt>/100000               0.355 ms        0.355 ms         1941
VarPlusConstFixed_Pmql<VariantIntDouble>/100000         0.356 ms        0.356 ms         1974
VarPlusConstParam_Native/100000                         0.025 ms        0.025 ms        28486
VarPlusConstParam_Pmql<SingleInt>/100000                 2.24 ms         2.24 ms          307
VarPlusConstParam_Pmql<VariantInt>/100000                2.63 ms         2.63 ms          266
VarPlusConstParam_Pmql<VariantIntDouble>/100000          2.95 ms         2.95 ms          235
AvgOfThree_Native/100000                                0.031 ms        0.031 ms        22430
AvgOfThree_Pmql<SingleDouble>/100000                     10.2 ms         10.2 ms           68
AvgOfThree_Pmql<VariantDouble>/100000                    11.4 ms         11.4 ms           62
AvgOfThree_Pmql<VariantIntDouble>/100000                 11.3 ms         11.3 ms           62
AvgOfThree_Cache_Disabled<VariantIntDouble>/100000       11.2 ms         11.2 ms           62
AvgOfThree_Cache_Enabled0<VariantIntDouble>/100000       11.3 ms         11.3 ms           59
AvgOfThree_Cache_Enabled1<VariantIntDouble>/100000       8.96 ms         8.96 ms           79
AvgOfThree_Cache_Enabled2<VariantIntDouble>/100000       5.21 ms         5.21 ms          137
AvgOfThree_Cache_Enabled3<VariantIntDouble>/100000      0.347 ms        0.347 ms         2047
```

