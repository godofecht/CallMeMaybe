#ifndef CALLMEMAYBE_STATIC_TYPE_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_TYPE_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <type_traits>
#include <utility>

#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info TypeRefl>
consteval TypeFlags make_static_type_flags()
{
    static_assert(std::meta::is_type(TypeRefl));

    TypeFlags flags{};
    flags.is_void = std::meta::is_void_type(TypeRefl);
    flags.is_null_pointer = std::meta::is_null_pointer_type(TypeRefl);
    flags.is_integral = std::meta::is_integral_type(TypeRefl);
    flags.is_floating_point = std::meta::is_floating_point_type(TypeRefl);
    flags.is_arithmetic = std::meta::is_arithmetic_type(TypeRefl);
    flags.is_fundamental = std::meta::is_fundamental_type(TypeRefl);
    flags.is_pointer = std::meta::is_pointer_type(TypeRefl);
    flags.is_lvalue_reference = std::meta::is_lvalue_reference_type(TypeRefl);
    flags.is_rvalue_reference = std::meta::is_rvalue_reference_type(TypeRefl);
    flags.is_reference = std::meta::is_reference_type(TypeRefl);
    flags.is_class = std::meta::is_class_type(TypeRefl);
    flags.is_union = std::meta::is_union_type(TypeRefl);
    flags.is_enum = std::meta::is_enum_type(TypeRefl);
    flags.is_scoped_enum = std::meta::is_scoped_enum_type(TypeRefl);
    flags.is_array = std::meta::is_array_type(TypeRefl);
    flags.is_function_type = std::meta::is_function_type(TypeRefl);
    flags.is_const = std::meta::is_const_type(TypeRefl);
    flags.is_volatile = std::meta::is_volatile_type(TypeRefl);
    flags.is_signed = std::meta::is_signed_type(TypeRefl);
    flags.is_unsigned = std::meta::is_unsigned_type(TypeRefl);
    return flags;
}

template <std::meta::info TypeRefl>
consteval RegistryEntityVariant make_static_type_entity()
{
    static_assert(std::meta::is_type(TypeRefl));
    using T = typename[:TypeRefl:];
    using DecayedT = std::remove_cvref_t<T>;

    if constexpr (std::meta::is_class_type(TypeRefl) || std::meta::is_union_type(TypeRefl))
    {
        Class value(std::meta::display_string_of(TypeRefl));
        value.set_size(sizeof(T));
        value.set_alignment(alignof(T));
        value.set_flags(make_static_type_flags<TypeRefl>());
        return value;
    }
    else if constexpr (std::meta::is_enum_type(TypeRefl))
    {
        Enum value(std::meta::display_string_of(TypeRefl));
        value.set_size(sizeof(T));
        value.set_alignment(alignof(T));
        value.set_flags(make_static_type_flags<TypeRefl>());
        return value;
    }
    else
    {
        Type value(std::meta::display_string_of(TypeRefl));
        if constexpr (!std::is_void_v<DecayedT> &&
                      !std::is_function_v<DecayedT> &&
                      !std::is_reference_v<T>)
        {
            value.set_size(sizeof(T));
            value.set_alignment(alignof(T));
        }
        value.set_flags(make_static_type_flags<TypeRefl>());
        return value;
    }
}

template <std::meta::info... TypeRefl>
consteval auto make_type_static_registry()
{
    static_assert((std::meta::is_type(TypeRefl) && ...));
    std::array<std::pair<cmm::info, RegistryEntityVariant>, sizeof...(TypeRefl)> entities{{
        {cmm::detail::hash_entity(TypeRefl), make_static_type_entity<TypeRefl>()}...
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_TYPE_REGISTRY_HPP
