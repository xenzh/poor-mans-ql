#pragma once

#include <ostream>


namespace pmql {



struct null
{
    operator bool() const { return false; }
};


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
