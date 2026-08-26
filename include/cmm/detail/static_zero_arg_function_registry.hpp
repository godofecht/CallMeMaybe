#ifndef CALLMEMAYBE_STATIC_ZERO_ARG_FUNCTION_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_ZERO_ARG_FUNCTION_REGISTRY_HPP

#include <array>
#include <meta>
#include <type_traits>
#include <utility>

#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_function_enum_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info FuncRefl>
consteval auto make_zero_arg_function_static_registry()
{
    static_assert(std::meta::is_function(FuncRefl));
    static_assert(StaticFunctionMetadata<FuncRefl>::parameter_ids.empty());

    constexpr std::meta::info ReturnTypeRefl = std::meta::return_type_of(FuncRefl);
    using ReturnT = typename[:ReturnTypeRefl:];

    const cmm::info function_id = cmm::detail::hash_entity(FuncRefl);
    const cmm::info return_type_id = cmm::detail::hash_entity(ReturnTypeRefl);

    Function function(std::meta::identifier_of(FuncRefl));
    function.set_display_name(std::meta::display_string_of(FuncRefl));
    function.set_return_type_id(return_type_id);
    function.set_parameter_ids(StaticFunctionMetadata<FuncRefl>::parameter_ids);
    function.set_thunk(cmm::detail::create_thunk<FuncRefl>());

    Type return_type(std::meta::display_string_of(ReturnTypeRefl));
    if constexpr (!std::is_reference_v<ReturnT> &&
                  !std::is_void_v<ReturnT> &&
                  !std::is_function_v<ReturnT>)
    {
        return_type.set_size(sizeof(ReturnT));
        return_type.set_alignment(alignof(ReturnT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 2> entities{{
        {function_id, function},
        {return_type_id, return_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_ZERO_ARG_FUNCTION_REGISTRY_HPP
