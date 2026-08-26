#include <array>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_parameter_function_registry.hpp"
#include "cmm/static_meta.hpp"

int static_unnamed_params(int, int)
{
    return 0;
}

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_two_parameter_function_static_registry<^^static_unnamed_params>()))

int main()
{
    const cmm::info function_id = cmm::detail::hash_entity(^^static_unnamed_params);
    const cmm::info int_id = cmm::detail::hash_entity(^^int);

    if (cmm::reflect_name("static_unnamed_params") != function_id) return 1;
    if (cmm::reflect_name("") != cmm::invalid_info) return 2;

    const auto parameters = cmm::parameters_view_of(function_id);
    if (parameters.size() != 2) return 3;
    if (parameters[0] == parameters[1]) return 4;

    if (!cmm::identifier_of(parameters[0]).empty()) return 5;
    if (!cmm::identifier_of(parameters[1]).empty()) return 6;
    if (cmm::parent_of(parameters[0]) != function_id) return 7;
    if (cmm::parent_of(parameters[1]) != function_id) return 8;
    if (cmm::type_of(parameters[0]) != int_id) return 9;
    if (cmm::type_of(parameters[1]) != int_id) return 10;

    std::array<cmm::Value, 2> args{cmm::Value(1), cmm::Value(2)};
    cmm::Value result;
    if (cmm::reflect_invoke(function_id, args, result) != cmm::Error::Success) return 11;
    if (result.get<int>() != 0) return 12;

    return 0;
}
