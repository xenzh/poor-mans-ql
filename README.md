# Poor man's query language

This library allows to build and evaluate formula expressions based on std-like functional objects. Type system is data-driven: library borrows allowed types from object storage, provided by the client, evaluation result types are inferred from variable substitutions.

## Build

```sh
mkdir build && cd build
cmake ..
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


pmql::Result<Value> test()
{
    // Build an expression : (a + 42) / b

    pmql::Builder<Value> builder;
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
        return result.error();
    }

    auto expr = *std::move(result);

    // Do variable substitutions, evaluate the expression.

    auto context = expr.context<Value>();
    auto &a = context("b")->get();
    auto &b = context("v")->get();

    a = 11.2;
    b = 99;
    return expr(context);
}
```

## Performance

This version is more of a proof of concept, and benchmarks do not look too inspiring:

```
Running ./build/bench/pmql_bench
Run on (12 X 2592.01 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 12288 KiB (x1)
Load Average: 0.05, 0.06, 0.06
-----------------------------------------------------------------------------------------
Benchmark                                               Time             CPU   Iterations
-----------------------------------------------------------------------------------------
SingleConst_Native/10000                            0.059 ms        0.059 ms        11486
SingleConst_Pmql<SingleInt>/10000                    2.59 ms         2.59 ms          266
SingleConst_Pmql<VariantInt>/10000                   3.37 ms         3.37 ms          209
SingleConst_Pmql<VariantIntDouble>/10000             3.65 ms         3.65 ms          203
VarPlusConstFixed_Native/10000                      0.059 ms        0.059 ms        11815
VarPlusConstFixed_Pmql<SingleInt>/10000              7.38 ms         7.38 ms           94
VarPlusConstFixed_Pmql<VariantInt>/10000             11.6 ms         11.6 ms           65
VarPlusConstFixed_Pmql<VariantIntDouble>/10000       11.5 ms         11.5 ms           61
VarPlusConstParam_Native/10000                      0.061 ms        0.061 ms        11254
VarPlusConstParam_Pmql<SingleInt>/10000              8.07 ms         8.07 ms           91
VarPlusConstParam_Pmql<VariantInt>/10000             11.7 ms         11.7 ms           60
VarPlusConstParam_Pmql<VariantIntDouble>/10000       11.8 ms         11.8 ms           59
```

## TODO

- [x] Prevent full specialization for all variant types x ops
- [ ] Ternary (n-ary) ops.
- [ ] Custom ops: if (3), avail (N).
- [ ] Store/load from string.
- [ ] Context caching / selective invalidation.
- [ ] Better benchmark converage.
- [ ] Actual unit tests.
