#pragma once

#include "unwrap.h"

#include <variant>
#include <string>
#include <sstream>


namespace pmql::err {


/// Error description container that can hold an enum error identifier along with corresponding struct specialization.
/// @tparam Kind error identifier type (typically an enum).
/// @tparam Details class template over enum identifier, that contains information about the error.
/// @tparam Kinds pack of error identifiers that can be held by the container.
template<typename Kind, template<Kind> typename Details, Kind ...Kinds>
class ErrorTemplate
{
    /// Error identifier.
    Kind d_kind;

    /// Error description object.
    std::variant<Details<Kinds>...> d_details;

    /// File name.
    std::string_view d_file;

    /// Line number.
    size_t d_line = 0;

public:
    /// Defines marker type for holding error identifiers.
    template<Kind> struct kind_t {};

    /// Construct error container instance.
    /// @tparam K error identifier.
    /// @tparam Args error descriptor initializers pack.
    /// @param args error description initializers.
    template<Kind K, typename... Args>
    ErrorTemplate(kind_t<K>, Args &&...args)
        : d_kind {K}
        , d_details {Details<K> {std::forward<Args>(args)...}}
    {
    }

    /// Specify where the error was noticed first.
    /// @param file file name.
    /// @param line line number.
    void at(std::string_view file, size_t line)
    {
        if (d_file.empty())
        {
            d_file = file;
            d_line = line;
        }
    }

    /// Return stored error identifier.
    /// @return error identifier.
    Kind kind() const
    {
        return d_kind;
    }

    /// Access error description for provided error type.
    /// @tparam K error identifier.
    /// @return pointer to stored error descriptor or nullptr, if the type does not match.
    template<Kind K>
    const Details<K> *details() const
    {
        return std::visit(
            [] (const auto &detail)
            {
                using Actual = std::decay_t<decltype(detail)>;
                if constexpr (std::is_same_v<Details<K>, Actual>)
                {
                    return &detail;
                }
                return nullptr;
            },
            d_details);
    }

    /// Write error contents to provided output stream.
    /// @param os output stream.
    /// @return modified output stream.
    std::ostream &describe(std::ostream &os) const
    {
        return std::visit(
            [this, &os] (const auto &detail) mutable -> std::ostream &
            {
                if (!d_file.empty())
                {
                    os << d_file << ":" << d_line << " ";
                }

                return os << detail;
            },
            d_details);
    }

    /// Return error description string.
    /// @return error description string.
    std::string description() const
    {
        std::ostringstream ostr;
        describe(ostr);
        return std::move(ostr).str();
    }

    [[noreturn]] void raise() const
    {
        throw std::runtime_error(description());
    }
};


/// Stream operator for error container types.
template<typename Kind, template<Kind> typename Details, Kind ...Kinds>
std::ostream &operator<<(std::ostream &os, const ErrorTemplate<Kind, Details, Kinds...> &error)
{
    return error.describe(os);
}


/// Format arguments using a string stream.
/// @tparam Args types to format.
/// @param args values to format.
/// @return formatted string.
template<typename... Args>
std::string format(Args &&...args)
{
    std::ostringstream ostr;
    (ostr << ... << args);
    return std::move(ostr).str();
}


} // namespace pmql::err
