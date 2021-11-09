#pragma once

#include <tl/expected.hpp>

#include <type_traits>
#include <string_view>


namespace pmql::err {


/// Helper template for unwrapping values from result types. Used in Try() macro.
/// @tparam R non-compatible result type.
template<typename R> struct Unwrap
{
    static_assert(std::is_same_v<R, R *>, "Trying to unwrap a non-result value");

    /// Stub error type used to minimize compiler error output.
    struct InvalidErrorType { template<typename... Ts> void raise(Ts &&...) {} };

    /// Non-result object.
    R result;

    /// Unwrapping location: file.
    std::string_view file;

    /// Unwrapping location: line.
    size_t line;

    /// Stub implementation used to minimize compiler error output.
    operator bool() { return true; }

    /// Stub implementation used to minimize compiler error output.
    R operator*() && { return std::move(result); }

    /// Stub implementation used to minimize compiler error output.
    tl::unexpected<InvalidErrorType> operator()() &&
    {
        return tl::unexpected<InvalidErrorType> {{}};
    }
};

/// Helper template for unwrapping values from result types. Used in Try() macro.
/// @tparam T success type.
/// @tparam E error type.
template<typename T, typename E> struct Unwrap<tl::expected<T, E>>
{
    /// Result object.
    tl::expected<T, E> result;

    /// Unwrapping location: file.
    std::string_view file;

    /// Unwrapping location: line.
    size_t line;

    /// Returns true if result object contains success value.
    operator bool() { return result.has_value(); }

    /// Generates an expression containing success value.
    T operator*() && { return *std::move(result); }

    /// Generates an expression containing error value.
    tl::unexpected<E> operator()() &&
    {
        result.error().at(file, line);
        return tl::unexpected<E> {std::move(result).error()};
    }
};

/// Helper template for unwrapping values from result types. Used in Try() macro.
/// @tparam E error type.
template<typename E> struct Unwrap<tl::expected<void, E>>
{
    /// Result object.
    tl::expected<void, E> result;

    /// Unwrapping location: file.
    std::string_view file;

    /// Unwrapping location: line.
    size_t line;

    /// Returns true if result object contains success value.
    operator bool() { return result.has_value(); }

    /// Generates a void expression;
    /// When called at the end of a statement expression (GCC/clang macro extension), makes it a void expression.
    void operator*() && {}

    /// Generates an expression containing error value.
    tl::unexpected<E> operator()() &&
    {
        result.error().at(file, line);
        return tl::unexpected<E> {std::move(result).error()};
    }
};

/// Factory function for Unwrap helper template.
/// @tparam R result type.
/// @param result result object rvalue.
/// @return Unwrapper object that holds passed result object.
template<typename R>
Unwrap<std::decay_t<R>> unwrap(R &&result, std::string_view file, size_t line)
{
    static_assert(std::is_rvalue_reference_v<
        decltype(result)>,
        "Try() and TryThrow() expect rvalue references");

    return Unwrap<std::decay_t<R>> {std::move(result), file, line};
}


/// Unwrap tl::expected value or do an early return if result contains an error.
/// Must be used in a function that returns tl::expected with the same error type.
/// @note This macro is a statement expression, a non standard compiler extension.
/// @param ResultExpr expression, that returns tl::expected.
/// @return success object if tl::expected contains success value.
#define Try(ResultExpr) \
    ({ \
        auto _result = ::pmql::err::unwrap((ResultExpr), __FILE__, __LINE__); \
        if (!_result) \
        { \
            return std::move(_result)(); \
        } \
        *std::move(_result); \
    })


/// Unwrap tl::expected value or throw an exception.
/// May be used in any function that is allowed to throw exceptions (i.e. without noexcept specifier).
/// @note This macro is a statement expression, a non standard compiler extension.
/// @throw std::runtime_error
/// @param ResultExpr expression, that returns tl::expected.
/// @param ... optional arguments to be passed to Error::raise().
/// @return success object if tl::expected contains success value.
#define TryThrow(ResultExpr, ...) \
    ({ \
        auto _result = ::pmql::err::unwrap((ResultExpr), __FILE__, __LINE__); \
        if (!_result) \
        { \
            std::move(_result)().value().raise(##__VA_ARGS__); \
        } \
        *std::move(_result); \
    })


} // namespace pmql::err
