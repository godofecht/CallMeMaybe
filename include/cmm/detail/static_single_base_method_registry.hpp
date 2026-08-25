#ifndef CALLMEMAYBE_STATIC_SINGLE_BASE_METHOD_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_SINGLE_BASE_METHOD_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <type_traits>

#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_class_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"
#include "cmm/detail/static_single_base_registry.hpp"
#include "cmm/detail/static_single_method_registry.hpp"

namespace cmm::detail {

template <std::meta::info DerivedRefl>
consteval auto make_single_base_method_static_registry()
{
    static_assert(std::meta::is_class_type(DerivedRefl));

    constexpr std::meta::info base = SingleBaseStaticMetadata<DerivedRefl>::reflected_base;
    constexpr std::meta::info BaseTypeRefl = SingleBaseStaticMetadata<DerivedRefl>::base_type_refl;
    constexpr std::meta::info method = SingleMethodStaticMetadata<BaseTypeRefl>::reflected_method;
    static constexpr auto parameters = std::define_static_array(std::meta::parameters_of(method));
    static_assert(parameters.empty(), "single-base-method static registry currently requires a zero-argument method");

    constexpr std::meta::info ReturnTypeRefl = std::meta::return_type_of(method);
    using DerivedT = typename[:DerivedRefl:];
    using BaseT = typename[:BaseTypeRefl:];
    using ReturnT = typename[:ReturnTypeRefl:];

    const cmm::info derived_id = cmm::detail::hash_entity(DerivedRefl);
    const cmm::info base_id = cmm::detail::hash_entity(base);
    const cmm::info base_type_id = cmm::detail::hash_entity(BaseTypeRefl);
    const cmm::info method_id = cmm::detail::hash_entity(method);
    const cmm::info return_type_id = cmm::detail::hash_entity(ReturnTypeRefl);

    Class derived(std::meta::display_string_of(DerivedRefl));
    derived.set_size(sizeof(DerivedT));
    derived.set_alignment(alignof(DerivedT));
    derived.set_bases(StaticClassMetadata<DerivedRefl>::base_ids);

    Class base_type(std::meta::display_string_of(BaseTypeRefl));
    base_type.set_size(sizeof(BaseT));
    base_type.set_alignment(alignof(BaseT));
    base_type.set_members(SingleMethodStaticMetadata<BaseTypeRefl>::member_ids);
    base_type.set_member_names(SingleMethodStaticMetadata<BaseTypeRefl>::member_names);

    Base base_entity(std::meta::display_string_of(base), base_type_id, derived_id);
    base_entity.set_is_virtual(std::meta::is_virtual(base));
    if constexpr (std::meta::is_public(base))
    {
        base_entity.set_access(Access::Public);
        base_entity.set_upcast_thunk(+[](const void* instance) -> const void*
        {
            return static_cast<const BaseT*>(static_cast<const DerivedT*>(instance));
        });
    }
    else if constexpr (std::meta::is_protected(base))
    {
        base_entity.set_access(Access::Protected);
    }
    else
    {
        base_entity.set_access(Access::Private);
    }

    Function fn(std::meta::identifier_of(method), true, false);
    fn.set_display_name(std::meta::display_string_of(method));
    fn.set_parent_id(base_type_id);
    fn.set_return_type_id(return_type_id);
    fn.set_is_const_member_function(std::meta::is_const(method));
    fn.set_thunk(cmm::detail::create_thunk<method>());

    Type return_type(std::meta::display_string_of(ReturnTypeRefl));
    if constexpr (!std::is_reference_v<ReturnT> &&
                  !std::is_void_v<ReturnT> &&
                  !std::is_function_v<ReturnT>)
    {
        return_type.set_size(sizeof(ReturnT));
        return_type.set_alignment(alignof(ReturnT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 5> entities{{
        {derived_id, derived},
        {base_id, base_entity},
        {base_type_id, base_type},
        {method_id, fn},
        {return_type_id, return_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_SINGLE_BASE_METHOD_REGISTRY_HPP
