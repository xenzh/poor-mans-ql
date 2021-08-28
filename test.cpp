#include "ops.h"
#include "util.h"

#include <utility>
#include <vector>
#include <variant>
#include <optional>
#include <tuple>

#include <iostream>


namespace fql {


/*
expression:
1. Expression      (untyped, unbound) -- contains list<ops>.
2. TypedExpression (  typed, unbound).-- can deduce return type.
3. BoundExpression (  typed,   bound) -- contains list<vals>.

variables:
1. Variable      (untyped, unbound) -- contains token index of the parent expression.
2. TypedVariable (  typed, unbound) -- defines value type and token index.
3. BoundVariable (  typed,   bound) -- defines value type and token index, provides a value.

Note:
* It's not feasible to use some kind of field list -> template pack unwrapper, as it generates _a lot_
  of pack visitor definitions (see an example below).

Alternative approach:
* bound variables to 'visitables': object([] (const auto &value) { value type is our type! }).

// ------


void typed_flow(const Expression &expression)
{
    auto vars = expression.variables();

    auto typed = expression(ops::typed<int> {vars[0]}, ops::typed<double>{vars[1]});
    using result_type = typename typed::type;

    auto result = typed(42, 999.88);
}

void less_typed_flow(const Expression &expression)
{
    using Type = Types<int, double>;

    apply_iterable(
        [] (auto... types)
        {
            expression.typecheck(
                [] (auto result)
                {
                    output.makeScalarBuilder<decltype(result)>();
                },
                types...);
        },
        expression.variables(),
        [] (const auto &var)
        {
            const auto meta = input.inputFieldMetadata(var.name());
            switch (meta.type)
            {
            case FieldType::INT : types.emplace_back(Type::of<int>()); break;
            case FieldType::DBL : types.emplace_back(Type::of<double>()); break;
            };
        }
    );
}


// -----

*/

void vec2args()
{
    auto printall = [] (auto... args) { (std::cout << ... << args) << std::endl; };
    auto sumall   = [] (auto... args) { return (0ull + ... + args); };

    std::vector<size_t> args {1, 2, 3, 9, 8, 7};
    util::apply_iterable(printall, args, [] (auto v) { return double(v) / 2; });
    std::cout << "sumall: " << util::apply_iterable(sumall, args) << std::endl;
}


template<typename... Ts>
class VisitableVariant
{
    std::variant<Ts...> d_value;

public:
    template<typename V>
    explicit VisitableVariant(V &&value)
        : d_value(std::forward<V>(value))
    {
    }

    template<typename F>
    decltype(auto) operator()(F &&callable) const
    {
        return std::visit(std::forward<F>(callable), d_value);
    }
};


template<typename... Ts>
class Types
{
    static constexpr std::tuple<Ts...> d_defaults {};
    const size_t d_index;

    template<typename V, size_t Idx, typename... Rest> struct index_of : std::integral_constant<size_t, size_t(-1)> {};

    template<typename V, size_t Idx, typename T, typename... Rest>
    struct index_of<V, Idx, T, Rest...>
    {
        static constexpr size_t value = std::is_same_v<V, T> ? Idx : index_of<V, Idx + 1, Rest...>::value;
    };

    Types(size_t index) : d_index(index) {};

public:
    template<typename V>
    static Types<Ts...> of()
    {
        constexpr size_t index = index_of<V, 0, Ts...>::value;
        static_assert(index < sizeof...(Ts), "Type is not allowed");

        return Types<Ts...> {index};
    }

    template<size_t Idx = 0, typename F>
    void operator()(F &&callable) const
    {
        if constexpr (Idx < sizeof...(Ts))
        {
            if (d_index == Idx)
            {
                callable(std::get<Idx>(d_defaults));
                return;
            }

            operator()<Idx + 1>(std::forward<F>(callable));
        }
    }
};


void test_visitable()
{
    using Types = Types<int, double, char>;

    auto one = Types::of<double>();
    one([] (auto def) { std::cout << "Is double? " << std::is_same_v<double, decltype(def)> << std::endl; });
}


void test_utils()
{
    util::collect_apply_iterable(
        [] (auto ...selected)
        {
            std::cout << "util::collect_apply_iterable({3, 1}, 11, 22, 33, 44): ";
            (std::cout << ... << selected) << std::endl;
        },
        std::vector<size_t> {0},
        11
    );
}


} // namespace fql


int main()
{
    std::cout << "Vec2Args\n";
    fql::vec2args();
    fql::test_visitable();
    fql::test_utils();
    return 0;
}
