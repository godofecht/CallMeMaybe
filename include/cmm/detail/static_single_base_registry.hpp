#ifndef CALLMEMAYBE_STATIC_SINGLE_BASE_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_SINGLE_BASE_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <type_traits>

#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_class_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info DerivedRefl>
struct SingleBaseStaticMetadata
{
    inline static constexpr auto reflected_base = []() consteval
    {
        static constexpr auto bases = std::define_static_array(
            std::meta::bases_of(DerivedRefl, std::meta::access_context::unchecked()));
        if (bases.size() != 1) throw "single-base static registry requires exactly one direct base";
        return bases[0];
    }();

    inline static constexpr std::meta::info base_type_refl = std::meta::type_of(reflected_base);
    inline static constexpr cmm::info base_id = cmm::detail::hash_entity(reflected_base);
    inline static constexpr cmm::info base_type_id = cmm::detail::hash_entity(base_type_refl);
};

template <std::meta::info DerivedRefl>
consteval auto make_single_base_static_registry()
{
    static_assert(std::meta::is_class_type(DerivedRefl));

    constexpr std::meta::info base = SingleBaseStaticMetadata<DerivedRefl>::reflected_base;
    constexpr std::meta::info BaseTypeRefl = SingleBaseStaticMetadata<DerivedRefl>::base_type_refl;
    using DerivedT = typename[:DerivedRefl:];
    using BaseT = typename[:BaseTypeRefl:];

    const cmm::info derived_id = cmm::detail::hash_entity(DerivedRefl);
    const cmm::info base_id = cmm::detail::hash_entity(base);
    const cmm::info base_type_id = cmm::detail::hash_entity(BaseTypeRefl);

    Class derived(std::meta::display_string_of(DerivedRefl));
    derived.set_size(sizeof(DerivedT));
    derived.set_alignment(alignof(DerivedT));
    derived.set_bases(StaticClassMetadata<DerivedRefl>::base_ids);

    Class base_type(std::meta::display_string_of(BaseTypeRefl));
    base_type.set_size(sizeof(BaseT));
    base_type.set_alignment(alignof(BaseT));

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

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 3> entities{{
        {derived_id, derived},
        {base_id, base_entity},
        {base_type_id, base_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_SINGLE_BASE_REGISTRY_HPP
