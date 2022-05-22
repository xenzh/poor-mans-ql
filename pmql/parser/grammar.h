#pragma once

#include "../pmql/ops.h"
//#include "tao/pegtl/internal/pegtl_string.hpp"

#include <tao/pegtl.hpp>


namespace pmql::parser::rules {


using namespace tao::pegtl;


/// Type: int
struct type : identifier {};

/// Value: 42
struct value : plus<not_one<'{', '}'>> {};

/// Typed value: int{42}
struct typedvalue : seq<type, one<'{'>, value, one<'}'>> {};

/// An absense of value: null
struct null : TAO_PEGTL_STRING( "null" ) {};

/// Typed constant: int{42}, null
struct constant : sor<typedvalue, null> {};

/// Variable name: hello32
struct varname: value {};

/// Untyped variable: ${}
struct variable : seq<one<'$'>, one<'{'>, varname, one<'}'>> {};

/// Builtin operators: +, -, /, *, ...
#define PmqlStdOp(Ns, Fn, Sign, Arity) \
struct Fn : TAO_PEGTL_STRING(#Sign) {};
PmqlStdOpList
#undef PmqlStdOp

/// Builtin unary operator: -, !, ~
struct unaryop : sor<
    negate     ,
    logical_not,
    bit_not    > {};

/// Builtin binary operator: +, -, *, /, ...
struct binaryop : sor <
    plus         ,
    minus        ,
    multiplies   ,
    divides      ,
    modulus      ,
    equal_to     ,
    not_equal_to ,
    greater_equal,
    greater      ,
    less_equal   ,
    less         ,
    logical_and  ,
    logical_or   ,
    bit_and      ,
    bit_or       ,
    bit_xor      > {};

using s = star<space>;

/// A constant, a variable or ab op in brackets: int{42}, ${hello}, (...), ...
struct expression;

/// Builtin unary operation: -int{42}, !(${hello}), ...
struct unary : seq<unaryop, s, expression> {};

/// Builtin binary operation: int{42} +null, (${hello})* int{42}, ...
struct binary : seq<expression, s, binaryop, s, expression> {};

/// Any builtin operation in brackets.
struct arithmetic : seq<one<'('>, sor<unary, binary>, one<')'>> {};

/// Branching operator
struct ternary : seq<
    TAO_PEGTL_STRING("if"), s,
    one<'('>, s,
    expression, s, one<','>, s,
    expression, s, one<','>, s,
    expression, s,
    one<')'>> {};

/// A constant, a variable or an op in brackets: int{42}, ${hello}, (...), ...
struct expression : sor<constant, variable, arithmetic, ternary> {};

/// Grammar: an expression.
using grammar = expression;


/// Grammar rule type helpers
template <typename T> struct id : std::false_type
{
    static constexpr std::string_view name = "standard/unknown";
};

#define PmqlRuleId(rule) \
template<> struct id<rule> : std::true_type { static constexpr std::string_view name = #rule; };

PmqlRuleId(type      );
PmqlRuleId(value     );
PmqlRuleId(typedvalue);
PmqlRuleId(null      );
PmqlRuleId(constant  );
PmqlRuleId(varname   );
PmqlRuleId(variable  );
PmqlRuleId(unaryop   );
PmqlRuleId(binaryop  );
PmqlRuleId(unary     );
PmqlRuleId(binary    );
PmqlRuleId(arithmetic);
PmqlRuleId(ternary   );
PmqlRuleId(expression);

#undef PmqlRuleId


#define PmqlStdOp(Ns, Fn, Sign, Arity) \
template<> struct id<Fn> : std::true_type { static constexpr std::string_view name = #Fn; };
PmqlStdOpList
#undef PmqlStdOp


} // namespace pmql::parser::rules

