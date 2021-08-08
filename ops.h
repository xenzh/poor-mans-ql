#include "op.h"


namespace ops {


using Op = traits::expand_t<std::variant, Var, Const>;

std::ostream &operator<<(std::ostream &os, const Op &op);


using Ops = std::vector<Op>;

std::ostream &operator<<(std::ostream &os, const Ops &ops);


template<typename F, typename... Os>
class OpsIterator
{
    const Ops &d_ops;
    Ops::const_iterator d_base;
    F d_transform;

    bool matching() const;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(std::declval<F>()(*std::declval<Ops::const_iterator>()));
    using reference = value_type;
    using pointer = value_type *;

    using iterator_category = std::forward_iterator_tag;

    template<typename Fn>
    OpsIterator(const Ops &ops, bool begin, Fn &&transform);

    OpsIterator(const OpsIterator &) = default;
    OpsIterator &operator=(const OpsIterator &) = default;

    bool operator==(const OpsIterator &other) const;
    bool operator!=(const OpsIterator &other) const;

    OpsIterator &operator++();
    reference operator*() const;
    pointer operator->() const;
};



inline std::ostream &operator<<(std::ostream &os, const Op &item)
{
    return std::visit(
        [&os] (const auto &op) mutable -> std::ostream &
        {
            return os << op;
        },
        item);
}


inline std::ostream &operator<<(std::ostream &os, const Ops &item)
{
    for (size_t id = 0; id < item.size(); ++id)
    {
        os << "{" << id << "} " << item[id] << (id == item.size() - 1 ? "" : ", ");
    }
    return os;
}


template<typename F, typename... Os>
bool OpsIterator<F, Os...>::matching() const
{
    return std::visit(
        [] (const auto &op)
        {
            return (false || ... || std::is_same_v<std::decay_t<decltype(op)>, Os>);
        },
        *d_base);
}

template<typename F, typename... Os>
template<typename Fn>
OpsIterator<F, Os...>::OpsIterator(const Ops &ops, bool begin, Fn &&transform)
    : d_ops(ops)
    , d_base(begin? d_ops.begin() : d_ops.end())
    , d_transform(std::forward<Fn>(transform))
{
    if (begin && !matching())
    {
        operator++();
    }
}

template<typename F, typename... Os>
bool OpsIterator<F, Os...>::operator==(const OpsIterator &other) const
{
    return d_base == other.d_base;
}

template<typename F, typename... Os>
bool OpsIterator<F, Os...>::operator!=(const OpsIterator &other) const
{
    return !operator==(other);
}

template<typename F, typename... Os>
OpsIterator<F, Os...> &OpsIterator<F, Os...>::operator++()
{
    if (d_base != d_ops.end())
    {
        ++d_base;
    }

    while (d_base != d_ops.end() && !matching())
    {
        ++d_base;
    }
}

template<typename F, typename... Os>
typename OpsIterator<F, Os...>::reference OpsIterator<F, Os...>::operator*() const
{
    return *d_base;
}

template<typename F, typename... Os>
typename OpsIterator<F, Os...>::pointer OpsIterator<F, Os...>::operator->() const
{
    return &(*d_base);
}



} // namespace ops
