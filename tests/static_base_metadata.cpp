#include <string_view>

#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_base_registry.hpp"

struct StaticBase
{
    int payload = 0;
};

struct StaticDerived : public StaticBase
{
    double extra = 0.0;
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_single_base_static_registry<^^StaticDerived>()))

int main()
{
    constexpr std::meta::info base_refl =
        cmm::detail::SingleBaseStaticMetadata<^^StaticDerived>::reflected_base;

    const cmm::info derived_id = cmm::detail::hash_entity(^^StaticDerived);
    const cmm::info base_spec_id = cmm::detail::hash_entity(base_refl);
    const cmm::info base_type_id = cmm::detail::hash_entity(^^StaticBase);

    const auto& registry = cmm::detail::active_static_registry();

    const auto* derived = registry.try_get_as<cmm::detail::Class>(derived_id);
    const auto* base_spec = registry.try_get_as<cmm::detail::Base>(base_spec_id);
    const auto* base_type = registry.try_get_as<cmm::detail::Class>(base_type_id);

    if (!derived || !base_spec || !base_type) return 1;
    if (derived->bases().size() != 1 || derived->bases()[0] != base_spec_id) return 2;
    if (base_spec->parent_id() != derived_id) return 3;
    if (base_spec->type_id() != base_type_id) return 4;
    if (base_spec->access() != cmm::detail::Access::Public) return 5;
    if (base_spec->is_virtual()) return 6;
    if (derived->size() != sizeof(StaticDerived)) return 7;
    if (derived->alignment() != alignof(StaticDerived)) return 8;
    if (base_type->size() != sizeof(StaticBase)) return 9;
    if (base_type->alignment() != alignof(StaticBase)) return 10;
    if (registry.get_id_by_name("StaticDerived") != derived_id) return 11;
    if (registry.get_id_by_name("StaticBase") != base_type_id) return 12;

    return 0;
}
