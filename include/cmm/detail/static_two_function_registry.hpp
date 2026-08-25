#ifndef CALLMEMAYBE_STATIC_TWO_FUNCTION_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_TWO_FUNCTION_REGISTRY_HPP

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

template <std::meta::info FirstFuncRefl, std::meta::info SecondFuncRefl>
consteval auto make_two_function_static_registry()
{
    static_assert(std::meta::is_function(FirstFuncRefl));
    static_assert(std::meta::is_function(SecondFuncRefl));
    static_assert(StaticFunctionMetadata<FirstFuncRefl>::parameter_ids.size() == 1);
    static_assert(StaticFunctionMetadata<SecondFuncRefl>::parameter_ids.size() == 1);

    static constexpr auto first_parameters =
        std::define_static_array(std::meta::parameters_of(FirstFuncRefl));
    static constexpr auto second_parameters =
        std::define_static_array(std::meta::parameters_of(SecondFuncRefl));

    constexpr std::meta::info first_parameter = first_parameters[0];
    constexpr std::meta::info second_parameter = second_parameters[0];
    constexpr std::meta::info FirstParameterTypeRefl = std::meta::type_of(first_parameter);
    constexpr std::meta::info SecondParameterTypeRefl = std::meta::type_of(second_parameter);
    constexpr std::meta::info FirstReturnTypeRefl = std::meta::return_type_of(FirstFuncRefl);
    constexpr std::meta::info SecondReturnTypeRefl = std::meta::return_type_of(SecondFuncRefl);

    static_assert(cmm::detail::hash_entity(FirstParameterTypeRefl) ==
                  cmm::detail::hash_entity(SecondParameterTypeRefl));
    static_assert(cmm::detail::hash_entity(FirstReturnTypeRefl) ==
                  cmm::detail::hash_entity(SecondReturnTypeRefl));
    static_assert(cmm::detail::hash_entity(FirstParameterTypeRefl) ==
                  cmm::detail::hash_entity(FirstReturnTypeRefl));

    using ScalarT = typename[:FirstParameterTypeRefl:];

    const cmm::info first_func_id = cmm::detail::hash_entity(FirstFuncRefl);
    const cmm::info second_func_id = cmm::detail::hash_entity(SecondFuncRefl);
    const cmm::info first_parameter_id = cmm::detail::hash_entity(first_parameter);
    const cmm::info second_parameter_id = cmm::detail::hash_entity(second_parameter);
    const cmm::info scalar_type_id = cmm::detail::hash_entity(FirstParameterTypeRefl);

    Function first(std::meta::identifier_of(FirstFuncRefl));
    first.set_display_name(std::meta::display_string_of(FirstFuncRefl));
    first.set_return_type_id(scalar_type_id);
    first.set_parameter_ids(StaticFunctionMetadata<FirstFuncRefl>::parameter_ids);
    first.set_thunk(cmm::detail::create_thunk<FirstFuncRefl>());

    Parameter first_param(std::meta::identifier_of(first_parameter),
                          scalar_type_id,
                          first_func_id,
                          0);
    first_param.set_display_name(std::meta::display_string_of(first_parameter));
    first_param.set_decayed_type_id(scalar_type_id);

    Function second(std::meta::identifier_of(SecondFuncRefl));
    second.set_display_name(std::meta::display_string_of(SecondFuncRefl));
    second.set_return_type_id(scalar_type_id);
    second.set_parameter_ids(StaticFunctionMetadata<SecondFuncRefl>::parameter_ids);
    second.set_thunk(cmm::detail::create_thunk<SecondFuncRefl>());

    Parameter second_param(std::meta::identifier_of(second_parameter),
                           scalar_type_id,
                           second_func_id,
                           0);
    second_param.set_display_name(std::meta::display_string_of(second_parameter));
    second_param.set_decayed_type_id(scalar_type_id);

    Type scalar_type(std::meta::display_string_of(FirstParameterTypeRefl));
    if constexpr (!std::is_reference_v<ScalarT> &&
                  !std::is_void_v<ScalarT> &&
                  !std::is_function_v<ScalarT>)
    {
        scalar_type.set_size(sizeof(ScalarT));
        scalar_type.set_alignment(alignof(ScalarT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 5> entities{{
        {first_func_id, first},
        {first_parameter_id, first_param},
        {second_func_id, second},
        {second_parameter_id, second_param},
        {scalar_type_id, scalar_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_TWO_FUNCTION_REGISTRY_HPP
