#pragma once

#include <ostream>


namespace pmql {



/// Type of null  literal used inside pmql to indicate an absense of value.
/// All standard operations with null  literals are well-defined:
/// * Arithmetic and bitwise operations: if any argument is null, then the result is null.
/// * Comparison: null is equal only to itself and always less than any non-null value.
struct null
{
    /// Nulls are implicitly convertible to bool(false) to be usable as conditions.
    operator bool() const { return false; }
};


/// Stream operator for null  literals.
inline std::ostream &operator<<(std::ostream &os, const null &)
{
    return os << "<null>";
}

template<typename V> const null &operator+(const V &     , const null &na) { return na; }
template<typename V> const null &operator+(const null &na, const V &     ) { return na; }
template<typename V> const null &operator+(const null &na, const null &  ) { return na; }

template<typename V> const null &operator-(const V &     , const null &na) { return na; }
template<typename V> const null &operator-(const null &na, const V &     ) { return na; }
template<typename V> const null &operator-(const null &na, const null &  ) { return na; }

template<typename V> const null &operator*(const V &     , const null &na) { return na; }
template<typename V> const null &operator*(const null &na, const V &     ) { return na; }
template<typename V> const null &operator*(const null &na, const null &  ) { return na; }

template<typename V> const null &operator/(const V &     , const null &na) { return na; }
template<typename V> const null &operator/(const null &na, const V &     ) { return na; }
template<typename V> const null &operator/(const null &na, const null &  ) { return na; }

template<typename V> const null &operator%(const V &     , const null &na) { return na; }
template<typename V> const null &operator%(const null &na, const V &     ) { return na; }
template<typename V> const null &operator%(const null &na, const null &  ) { return na; }

template<typename V> const null &operator-(const null &na) { return na; }

template<typename V> bool operator==(const V &   , const null &  ) { return false; }
template<typename V> bool operator==(const null &, const V &     ) { return false; }
template<typename V> bool operator==(const null &, const null &  ) { return true ; }

template<typename V> bool operator!=(const V &   , const null &  ) { return true ; }
template<typename V> bool operator!=(const null &, const V &     ) { return true ; }
template<typename V> bool operator!=(const null &, const null &  ) { return false; }

template<typename V> bool operator>(const V &   , const null &  ) { return true ; }
template<typename V> bool operator>(const null &, const V &     ) { return false; }
template<typename V> bool operator>(const null &, const null &  ) { return false; }

template<typename V> bool operator<(const V &   , const null &  ) { return false; }
template<typename V> bool operator<(const null &, const V &     ) { return true ; }
template<typename V> bool operator<(const null &, const null &  ) { return false; }

template<typename V> bool operator>=(const V &   , const null &  ) { return true ; }
template<typename V> bool operator>=(const null &, const V &     ) { return false; }
template<typename V> bool operator>=(const null &, const null &  ) { return true ; }

template<typename V> bool operator<=(const V &   , const null &  ) { return false; }
template<typename V> bool operator<=(const null &, const V &     ) { return true ; }
template<typename V> bool operator<=(const null &, const null &  ) { return true ; }


} // namespace pmql
