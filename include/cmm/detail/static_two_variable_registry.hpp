#ifndef CALLMEMAYBE_STATIC_TWO_VARIABLE_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_TWO_VARIABLE_REGISTRY_HPP

#include <array>
#include <meta>
#include <type_traits>
#include <utility>

#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/entities/variable.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info VarRefl>
consteval Variable make_static_variable()
{
    static_assert(std::meta::is_variable(VarRefl));

    constexpr std::meta::info TypeRefl = std::meta::type_of(VarRefl);
    constexpr bool is_const = std::meta::is_const_type(TypeRefl);
    using ValueT = std::remove_cvref_t<typename[:TypeRefl:]>;

    Variable variable(std::meta::identifier_of(VarRefl), hash_entity(TypeRefl));
    variable.set_display_name(std::meta::display_string_of(VarRefl));
    variable.set_is_const(is_const);
    variable.set_address(const_cast<void*>(static_cast<const void*>(&[:VarRefl:])));
    variable.set_getter_thunk(&StaticThunks<ValueT>::get);

    if constexpr (is_const)
    {
        variable.set_ref_getter_thunk(&StaticThunks<ValueT>::get_cref);
    }
    else
    {
        variable.set_ref_getter_thunk(&StaticThunks<ValueT>::get_ref);
        variable.set_setter_thunk(&StaticThunks<ValueT>::set);
    }

    return variable;
}

template <std::meta::info TypeRefl>
consteval Type make_static_variable_type()
{
    using T = typename[:TypeRefl:];
    Type type(std::meta::display_string_of(TypeRefl));
    if constexpr (!std::is_void_v<T> && !std::is_reference_v<T> && !std::is_function_v<T>)
    {
        type.set_size(sizeof(T));
        type.set_alignment(alignof(T));
    }
    return type;
}

template <std::meta::info FirstVarRefl, std::meta::info SecondVarRefl>
consteval auto make_two_variable_static_registry()
{
    static_assert(std::meta::is_variable(FirstVarRefl));
    static_assert(std::meta::is_variable(SecondVarRefl));

    constexpr std::meta::info FirstTypeRefl = std::meta::type_of(FirstVarRefl);
    constexpr std::meta::info SecondTypeRefl = std::meta::type_of(SecondVarRefl);

    const cmm::info first_id = hash_entity(FirstVarRefl);
    const cmm::info second_id = hash_entity(SecondVarRefl);
    const cmm::info first_type_id = hash_entity(FirstTypeRefl);
    const cmm::info second_type_id = hash_entity(SecondTypeRefl);

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 4> entities{{
        {first_id, make_static_variable<FirstVarRefl>()},
        {second_id, make_static_variable<SecondVarRefl>()},
        {first_type_id, make_static_variable_type<FirstTypeRefl>()},
        {second_type_id, make_static_variable_type<SecondTypeRefl>()},
    }};

    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_TWO_VARIABLE_REGISTRY_HPP
