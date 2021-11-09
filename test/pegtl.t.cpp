#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>

#include <gtest/gtest.h>


namespace pmql {


namespace pegtl = tao::pegtl;


namespace rules {


using namespace pegtl;


struct type : plus<not_at<space>> {};


} // namespace rules


template<typename Rule> struct action : pegtl::nothing<Rule> {};

template<> struct action<rules::type>
{
    template<typename ActionInput>
    void apply(const ActionInput &input, std::string &out)
    {
        out += input.string();
    }
};


TEST(PegTL, SimpleUnary)
{
    using namespace tao;

    if (pegtl::analyze<rules::type>() != 0)
    {
        FAIL() << "Grammar is invalid";
    }


}


} // namespace pmql {
