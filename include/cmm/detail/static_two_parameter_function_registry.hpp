#ifndef CALLMEMAYBE_STATIC_TWO_PARAMETER_FUNCTION_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_TWO_PARAMETER_FUNCTION_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_function_enum_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info FuncRefl>
consteval auto make_two_parameter_function_static_registry()
{
    static_assert(std::meta::is_function(FuncRefl));
    static_assert(StaticFunctionMetadata<FuncRefl>::parameter_ids.size() == 2);

    static constexpr auto parameters =
        std::define_static_array(std::meta::parameters_of(FuncRefl));

    constexpr std::meta::info first_parameter = parameters[0];
    constexpr std::meta::info second_parameter = parameters[1];
    constexpr std::meta::info FirstParameterTypeRefl = std::meta::type_of(first_parameter);
    constexpr std::meta::info SecondParameterTypeRefl = std::meta::type_of(second_parameter);
    constexpr std::meta::info ReturnTypeRefl = std::meta::return_type_of(FuncRefl);

    constexpr cmm::info first_parameter_type_id = cmm::detail::hash_entity(FirstParameterTypeRefl);
    constexpr cmm::info second_parameter_type_id = cmm::detail::hash_entity(SecondParameterTypeRefl);
    constexpr cmm::info return_type_id = cmm::detail::hash_entity(ReturnTypeRefl);

    constexpr bool second_parameter_type_is_new =
        second_parameter_type_id != first_parameter_type_id;
    constexpr bool return_type_is_new =
        return_type_id != first_parameter_type_id &&
        return_type_id != second_parameter_type_id;
    constexpr std::size_t type_count =
        1 +
        static_cast<std::size_t>(second_parameter_type_is_new) +
        static_cast<std::size_t>(return_type_is_new);

    using FirstParameterT = typename[:FirstParameterTypeRefl:];
    using SecondParameterT = typename[:SecondParameterTypeRefl:];
    using ReturnT = typename[:ReturnTypeRefl:];

    const cmm::info function_id = cmm::detail::hash_entity(FuncRefl);
    const cmm::info first_parameter_id = cmm::detail::hash_entity(first_parameter);
    const cmm::info second_parameter_id = cmm::detail::hash_entity(second_parameter);

    Function function(std::meta::identifier_of(FuncRefl));
    function.set_display_name(std::meta::display_string_of(FuncRefl));
    function.set_return_type_id(return_type_id);
    function.set_parameter_ids(StaticFunctionMetadata<FuncRefl>::parameter_ids);
    function.set_thunk(cmm::detail::create_thunk<FuncRefl>());

    const std::string_view first_name = std::meta::has_identifier(first_parameter)
        ? std::meta::identifier_of(first_parameter)
        : std::string_view{};
    Parameter first_param(first_name,
                          first_parameter_type_id,
                          function_id,
                          0);
    first_param.set_display_name(std::meta::display_string_of(first_parameter));
    first_param.set_decayed_type_id(first_parameter_type_id);

    const std::string_view second_name = std::meta::has_identifier(second_parameter)
        ? std::meta::identifier_of(second_parameter)
        : std::string_view{};
    Parameter second_param(second_name,
                           second_parameter_type_id,
                           function_id,
                           1);
    second_param.set_display_name(std::meta::display_string_of(second_parameter));
    second_param.set_decayed_type_id(second_parameter_type_id);

    Type first_parameter_type(std::meta::display_string_of(FirstParameterTypeRefl));
    if constexpr (!std::is_reference_v<FirstParameterT> &&
                  !std::is_void_v<FirstParameterT> &&
                  !std::is_function_v<FirstParameterT>)
    {
        first_parameter_type.set_size(sizeof(FirstParameterT));
        first_parameter_type.set_alignment(alignof(FirstParameterT));
    }

    Type second_parameter_type(std::meta::display_string_of(SecondParameterTypeRefl));
    if constexpr (!std::is_reference_v<SecondParameterT> &&
                  !std::is_void_v<SecondParameterT> &&
                  !std::is_function_v<SecondParameterT>)
    {
        second_parameter_type.set_size(sizeof(SecondParameterT));
        second_parameter_type.set_alignment(alignof(SecondParameterT));
    }

    Type return_type(std::meta::display_string_of(ReturnTypeRefl));
    if constexpr (!std::is_reference_v<ReturnT> &&
                  !std::is_void_v<ReturnT> &&
                  !std::is_function_v<ReturnT>)
    {
        return_type.set_size(sizeof(ReturnT));
        return_type.set_alignment(alignof(ReturnT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 3 + type_count> entities{};
    std::size_t entity_index = 0;
    entities[entity_index++] = {function_id, function};
    entities[entity_index++] = {first_parameter_id, first_param};
    entities[entity_index++] = {second_parameter_id, second_param};
    entities[entity_index++] = {first_parameter_type_id, first_parameter_type};

    if constexpr (second_parameter_type_is_new)
    {
        entities[entity_index++] = {second_parameter_type_id, second_parameter_type};
    }
    if constexpr (return_type_is_new)
    {
        entities[entity_index++] = {return_type_id, return_type};
    }

    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_TWO_PARAMETER_FUNCTION_REGISTRY_HPP
