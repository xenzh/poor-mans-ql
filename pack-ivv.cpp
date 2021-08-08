#include <vector>
#include <variant>
#include <optional>
#include <tuple>

#include <iostream>


namespace fql {


struct Ivv
{
    virtual void visit_int   (int    value, bool null) = 0;
    virtual void visit_double(double value, bool null) = 0;
};

struct Iv
{
    std::variant<std::optional<int>, std::optional<double>> stored;

    void accept(Ivv &visitor) const
    {
        std::visit(
        [&visitor] (const auto &value) mutable
        {
            using type = typename std::decay_t<decltype(value)>::value_type;

            if constexpr (std::is_same_v<type, int>)
            {
                visitor.visit_int(value.value_or(type {}), !value.has_value());
            }

            if constexpr (std::is_same_v<type, double>)
            {
                visitor.visit_double(value.value_or(type {}), !value.has_value());
            }
        },
        stored);
    }
};

struct I
{
    std::vector<Iv> inputvalues;
};


template<typename Recv, typename... Ts>
struct PackIvv : Ivv
{
    template<typename = void, typename... Vs> struct Tuple { using type = std::tuple<>; };
    template<typename... Vs> struct Tuple<std::enable_if_t<(sizeof...(Vs) > 0)>, Vs...>
    {
        using type = std::tuple<std::optional<Vs>...>;
    };

    static constexpr size_t LIMIT = 7;

    const Recv &recv;
    typename Tuple<void, Ts...>::type values;
    const I &input;
    const std::vector<size_t> &indices;
    size_t current;

    PackIvv(
        const Recv &recv,
        typename Tuple<void, Ts...>::type values,
        const I &input,
        const std::vector<size_t> &indices,
        size_t current = 0)
        : recv(recv)
        , values(std::move(values))
        , input(input)
        , indices(indices)
        , current(current)
    {
    }

    template<typename V>
    void visit(const V &value, bool null)
    {
        auto nextuple = std::tuple_cat(values, std::make_tuple(null ? std::nullopt : std::make_optional(value)));

        if (current >= indices.size())
        {
            std::apply(recv, nextuple);
            return;
        }

        static_assert(sizeof...(Ts) <= LIMIT, "Pack expansion limit reached");
        if constexpr (sizeof...(Ts) < LIMIT)
        {
            PackIvv<Recv, Ts..., V> next {
                recv,
                std::move(nextuple),
                input,
                indices,
                current + 1};

            const auto field = indices[current];
            input.inputvalues[field].accept(next);
        }
    }

    void visit_int   (int    value, bool null) override { visit(value, null); }
    void visit_double(double value, bool null) override { visit(value, null); }

    static void apply(
        const Recv &recv,
        const I &input,
        const std::vector<size_t> &indices)
    {
        PackIvv<Recv> start {recv, std::make_tuple(), input, indices, 1u};
        input.inputvalues[indices.front()].accept(start);
    }
};

struct Receiver
{
    template<typename... Ts>
    void operator()(Ts &&...values) const
    {
        (std::cout << ... << *std::forward<Ts>(values)) << std::endl;
    }
};

void test()
{
    I input {{ Iv {std::make_optional(4.2)}, Iv {std::make_optional(42)} }};

    Receiver recv;
    std::vector<size_t> indices {0, 1};

    PackIvv<Receiver>::apply(recv, input, indices);
};


} // namespace fql


int main()
{
    std::cout << "Print out indices [1, 0] of input\n";
    fql::test();
    return 0;
}
