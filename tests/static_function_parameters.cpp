#include <array>
#include <string_view>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_function_registry.hpp"
#include "cmm/static_meta.hpp"

int static_param_one(int value)
{
    return value;
}

int static_param_two(int value)
{
    return value + 1;
}

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_two_function_static_registry<^^static_param_one, ^^static_param_two>()))

int main()
{
    const cmm::info one_id = cmm::detail::hash_entity(^^static_param_one);
    const cmm::info two_id = cmm::detail::hash_entity(^^static_param_two);
    const cmm::info int_id = cmm::detail::hash_entity(^^int);

    if (cmm::reflect_name("static_param_one") != one_id) return 1;
    if (cmm::reflect_name("static_param_two") != two_id) return 2;
    if (cmm::reflect_name("value") != cmm::invalid_info) return 3;

    const auto one_params = cmm::parameters_view_of(one_id);
    const auto two_params = cmm::parameters_view_of(two_id);
    if (one_params.size() != 1 || two_params.size() != 1) return 4;

    const cmm::info one_param = one_params[0];
    const cmm::info two_param = two_params[0];
    if (one_param == two_param) return 5;
    if (cmm::identifier_of(one_param) != std::string_view("value")) return 6;
    if (cmm::identifier_of(two_param) != std::string_view("value")) return 7;
    if (cmm::parent_of(one_param) != one_id) return 8;
    if (cmm::parent_of(two_param) != two_id) return 9;
    if (cmm::type_of(one_param) != int_id) return 10;
    if (cmm::type_of(two_param) != int_id) return 11;

    std::array<cmm::Value, 1> first_args{cmm::Value(41)};
    cmm::Value first_result;
    if (cmm::reflect_invoke(one_id, first_args, first_result) != cmm::Error::Success) return 12;
    if (first_result.get<int>() != 41) return 13;

    std::array<cmm::Value, 1> second_args{cmm::Value(41)};
    cmm::Value second_result;
    if (cmm::reflect_invoke(two_id, second_args, second_result) != cmm::Error::Success) return 14;
    if (second_result.get<int>() != 42) return 15;

    cmm::Value not_invocable_result;
    if (cmm::reflect_invoke(int_id, {}, not_invocable_result) != cmm::Error::NotInvocable) return 16;

    return 0;
}
