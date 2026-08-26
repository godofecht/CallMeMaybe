#include <string_view>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_zero_arg_function_registry.hpp"
#include "cmm/static_meta.hpp"

int static_zero_arg()
{
    return 77;
}

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_zero_arg_function_static_registry<^^static_zero_arg>()))

int main()
{
    const cmm::info function_id = cmm::detail::hash_entity(^^static_zero_arg);
    const cmm::info int_id = cmm::detail::hash_entity(^^int);

    if (cmm::reflect_name("static_zero_arg") != function_id) return 1;
    if (cmm::identifier_of(function_id) != std::string_view("static_zero_arg")) return 2;
    if (cmm::size_of(int_id) != sizeof(int)) return 3;
    if (!cmm::parameters_view_of(function_id).empty()) return 4;

    cmm::Value result;
    if (cmm::reflect_invoke(function_id, {}, result) != cmm::Error::Success) return 5;
    if (result.get<int>() != 77) return 6;

    return 0;
}
