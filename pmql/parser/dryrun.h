#pragma once

#include "grammar.h"

#include <tao/pegtl.hpp>
#include <type_traits>


namespace pmql::parser::actions {


using namespace tao::pegtl;


/// Debug action that prints out rules in order of traversal.
template<typename Rule, typename = void> struct DryRun : nothing<Rule> {};

template<typename Rule> struct DryRun<Rule, std::enable_if_t<rules::id<Rule>::value>>
{
    template<typename In>
    static void apply(const In &input, std::ostream &os)
    {
        os << rules::id<Rule>::name << ": " << input.string() << "\n";
    }
};



} // namespace pmql::parser::actions
