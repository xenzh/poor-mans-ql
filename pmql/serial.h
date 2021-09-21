#include "error.h"
#include "ops.h"

#include <utility>
#include <vector>
#include <string>
#include <sstream>


namespace pmql::serial {


template<typename Store>
using Ingredients = std::pair<std::vector<Store>, op::List>;


template<typename Store>
std::string store(const op::List &ops, const std::vector<Store> &consts);

template<typename Store>
Result<Ingredients<Store>> load(std::string_view stored);



template<typename Store>
std::string store(const op::List &ops, const std::vector<Store> &consts)
{
    (void) consts;
    (void) ops;
    return {};
}

template<typename Store>
Result<Ingredients<Store>> load(std::string_view stored)
{
    (void) stored;
    return {};
}


} // namespace pmql::serial
