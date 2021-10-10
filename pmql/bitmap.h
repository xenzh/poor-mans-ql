#pragma once

#include <cstdint>
#include <climits>

#include <optional>
#include <vector>


namespace pmql {


/// Bare-bones implementation of dynamic bitset.
/// Size is defined on construction (not resizable), supports a handful of bitwise operators.
/// Interface mimics std::bitset.
class Bitmap
{
    /// Single buffer element.
    using Elem = uint64_t;

    /// Number of bits in buffer element.
    static constexpr size_t ELEM_BIT = sizeof(Elem) * CHAR_BIT;

    /// Buffer element value with all bits reset.
    static constexpr Elem FILL_FALSE = 0;

    /// Buffer element value with all bits set.
    static constexpr Elem FILL_TRUE  = -1;

    /// Bit storage type that can hold dynamic number of elements.
    using Buffer = std::vector<Elem>;

    /// Bit storage.
    Buffer d_buffer;

    /// Number of available bits.
    size_t d_size = 0;

    /// Construct bitmap instance directly from element buffer.
    /// @param buffer element buffer.
    /// @param size number of available bits.
    Bitmap(Buffer &&buffer, size_t size);

public:
    /// Defines bitmap collection type.
    using List = std::vector<Bitmap>;

    /// Defines bitmap element type.
    using value_type = bool;

    /// Proxy type that allows to manipulate single bits as booleans.
    class Bit;

    /// Bitset iterator type.
    class const_iterator;

    /// Construct empty bitset instance.
    Bitmap() = default;

    /// Construct and initialize bitset instance.
    /// @param count number of bits to allocate.
    /// @param fill initial value for all allocated bits.
    Bitmap(size_t count, bool fill);

    /// Get number of available bits.
    /// @return number of bits.
    size_t size() const;

    /// Construct iterator, pointing to the first stored bit.
    /// @return begin iterator.
    const_iterator begin() const;

    /// Construct iterator, pointing past the end of the last stored bit.
    /// @return end iterator.
    const_iterator end() const;

    /// Check bit state.
    /// @param bit bit number.
    /// @return true if bit is set.
    bool test(size_t bit) const;

    /// Set bit value to 1.
    /// @param bit bit number to set.
    /// @return reference to self.
    Bitmap &set(size_t bit);

    /// Reset bit value to 0.
    /// @param bit bit number to reset.
    /// @return reference to self.
    Bitmap &reset(size_t bit);

    /// Check bit state.
    /// @param bit number to test.
    /// @return true if bit is set.
    bool operator[](size_t bit) const;

    /// Construct modifiable bit state proxy.
    /// @param bit bit number to expose.
    /// @return bit state proxy.
    Bit operator[](size_t bit);

    /// Construct a new bitset with all allocated bits flipped.
    /// @return inverted bitset instance.
    Bitmap operator~() const;

    /// Combine this bitset with another using bitwise OR.
    /// @param other other bitset instance.
    /// @return reference to self.
    Bitmap &operator|=(const Bitmap &other);

    /// Combine this bitset with another using bitwise AND.
    /// @param other other bitset instance.
    /// @return reference to self.
    Bitmap &operator&=(const Bitmap &other);
};


/// Bit state proxy that allows to manipulate the value as boolean.
/// It's the same idea as std::bitset::reference.
class Bitmap::Bit
{
    /// Parent bitset reference.
    Bitmap &d_bitmap;

    /// Bit number.
    const size_t d_bit;

public:
    /// Construct bit state proxy instance.
    /// @param bitmap parent bitset reference.
    /// @param bit bit number.
    Bit(Bitmap &bitmap, size_t bit);

    /// Update bit state from provided boolean value.
    /// @param value new bit state.
    /// @return reference to self.
    Bit &operator=(bool value);

    /// Convert bit state to boolean.
    /// @return true if the bit is set.
    operator bool() const;
};


/// Bit state iterator type for Bitset container.
/// Implements standard LegacyForwardIterator requirements.
class Bitmap::const_iterator
{
    /// Pointer to parent bitmap instance.
    const Bitmap *d_bitmap;

    /// Stores last bit state, accesses via operator-> (implementation is required to return a pointer).
    mutable bool d_last;

    /// Current bit number.
    size_t d_bit = 0;

public:
    /// Defines iterator difference type.
    using difference_type = std::ptrdiff_t;

    /// Defines iterator value type.
    using value_type = bool;

    /// Defines iterator reference type (value type used for simplicity).
    using reference = value_type;

    /// Defines iterator pointer type for operator->.
    using pointer = value_type *;

    /// Identifies this iterator type as a forward input iterator.
    using iterator_category = std::forward_iterator_tag;

    /// Construct iterator instance.
    /// @param bitmap parent bitmap reference.
    /// @param begin if set to true, iterator is pointed to the first bit and past the last one otherwise.
    const_iterator(const Bitmap &bitmap, bool begin);

    /// Copy-construct iterator instance.
    const_iterator(const const_iterator &) = default;

    /// Copy-assign iterator instance.
    const_iterator &operator=(const const_iterator &) = default;

    /// Compare two iterators for equality.
    /// @param other iterator to compare to.
    /// @return true if iterators point to the same bit from the same bitset.
    bool operator==(const const_iterator &other) const;

    /// Compare two iterators for non-equality.
    /// @param other iterator to compare to.
    /// @return true if iterators do not point to the same bit or created from different bitsets.
    bool operator!=(const const_iterator &other) const;

    /// Point iterator to the next bit. It is safe to increment end iterators.
    /// @return reference to self.
    const_iterator &operator++();

    /// Return the state of the bit iterator points to. Not safe to call on end iterators.
    /// @return current bit state.
    reference operator*() const;

    /// Return the state of the bit iterator points to. Not safe to call on end iterators.
    /// @return current bit state pointer.
    pointer operator->() const;
};



inline Bitmap::Bit::Bit(Bitmap &bitmap, size_t bit)
    : d_bitmap(bitmap)
    , d_bit(bit)
{
}

inline Bitmap::Bit &Bitmap::Bit::operator=(bool value)
{
    value ? d_bitmap.set(d_bit) : d_bitmap.reset(d_bit);
    return *this;
}

inline Bitmap::Bit::operator bool() const
{
    return d_bitmap.test(d_bit);
}


inline Bitmap::const_iterator::const_iterator(const Bitmap &bitmap, bool begin)
    : d_bitmap(&bitmap)
    , d_bit(begin ? 0 : bitmap.size())
{
}

inline bool Bitmap::const_iterator::operator==(const const_iterator &other) const
{
    return d_bitmap == other.d_bitmap && d_bit == other.d_bit;
}

inline bool Bitmap::const_iterator::operator!=(const const_iterator &other) const
{
    return !operator==(other);
}

inline Bitmap::const_iterator &Bitmap::const_iterator::operator++()
{
    if (d_bit < d_bitmap->size())
    {
        ++d_bit;
    }

    return *this;
}

inline Bitmap::const_iterator::reference Bitmap::const_iterator::operator*() const
{
    return d_bitmap->test(d_bit);
}

inline Bitmap::const_iterator::pointer Bitmap::const_iterator::operator->() const
{
    d_last = d_bitmap->test(d_bit);
    return &d_last;
}


inline Bitmap::Bitmap(Buffer &&buffer, size_t size)
    : d_buffer(std::move(buffer))
    , d_size(size)
{
}

inline Bitmap::Bitmap(size_t count, bool fill)
    : d_size(count)
{
    const size_t elems = (count / ELEM_BIT) + bool(count % ELEM_BIT);
    d_buffer.resize(elems, fill ? FILL_TRUE : FILL_FALSE);
}

inline size_t Bitmap::size() const
{
    return d_size;
}

inline Bitmap::const_iterator Bitmap::begin() const
{
    return {*this, true};
}

inline Bitmap::const_iterator Bitmap::end() const
{
    return {*this, false};
}

inline bool Bitmap::test(size_t bit) const
{
    const size_t elem = bit / ELEM_BIT;
    const size_t offset = bit % ELEM_BIT;

    return (d_buffer[elem] >> offset) & 1;
}

inline Bitmap &Bitmap::set(size_t bit)
{
    const size_t elem = bit / ELEM_BIT;
    const size_t offset = bit % ELEM_BIT;

    d_buffer[elem] |= Elem(1 << offset);
    return *this;
}

inline Bitmap &Bitmap::reset(size_t bit)
{
    const size_t elem = bit / ELEM_BIT;
    const size_t offset = bit % ELEM_BIT;

    d_buffer[elem] &= ~Elem(1 << offset);
    return *this;
}

inline bool Bitmap::operator[](size_t bit) const
{
    return test(bit);
}

inline Bitmap::Bit Bitmap::operator[](size_t bit)
{
    return {*this, bit};
}

inline Bitmap Bitmap::operator~() const
{
    Buffer inverted(d_buffer.size());
    for (size_t elem = 0; elem < d_buffer.size(); ++elem)
    {
        inverted[elem] = ~d_buffer[elem];
    }

    const size_t trailing = d_size % ELEM_BIT;
    if (trailing)
    {
        const auto mask = (FILL_TRUE << trailing) >> trailing;
        inverted[inverted.size() - 1] &= mask;
    }

    return {std::move(inverted), d_size};
}

inline Bitmap &Bitmap::operator|=(const Bitmap &other)
{
    for (size_t elem = 0; elem < d_buffer.size() && elem < other.d_buffer.size(); ++elem)
    {
        d_buffer[elem] |= other.d_buffer[elem];
    }

    return *this;
}

inline Bitmap &Bitmap::operator&=(const Bitmap &other)
{
    for (size_t elem = 0; elem < d_buffer.size() && elem < other.d_buffer.size(); ++elem)
    {
        d_buffer[elem] &= other.d_buffer[elem];
    }

    return *this;
}


} // namespace pmql
