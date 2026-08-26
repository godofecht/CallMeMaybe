#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_base_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticProtectedBase
{
    int payload = 0;
};

struct StaticProtectedDerived : protected StaticProtectedBase
{
    double extra = 0.0;
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_single_base_static_registry<^^StaticProtectedDerived>()))

int main()
{
    constexpr std::meta::info base_refl =
        cmm::detail::SingleBaseStaticMetadata<^^StaticProtectedDerived>::reflected_base;

    const cmm::info derived_id = cmm::detail::hash_entity(^^StaticProtectedDerived);
    const cmm::info base_spec_id = cmm::detail::hash_entity(base_refl);
    const cmm::info base_type_id = cmm::detail::hash_entity(^^StaticProtectedBase);

    if (!cmm::is_base(base_spec_id)) return 1;
    if (!cmm::is_protected_base(base_spec_id)) return 2;
    if (cmm::is_public_base(base_spec_id)) return 3;
    if (cmm::is_private_base(base_spec_id)) return 4;
    if (cmm::is_virtual_base(base_spec_id)) return 5;
    if (cmm::parent_of(base_spec_id) != derived_id) return 6;
    if (cmm::type_of(base_spec_id) != base_type_id) return 7;

    StaticProtectedDerived instance{};
    const void* adjusted = nullptr;
    const cmm::Error error = cmm::detail::active_static_registry().adjust_instance_pointer(
        derived_id, base_type_id, &instance, adjusted);
    if (error != cmm::Error::InvalidArgumentType) return 8;
    if (adjusted != nullptr) return 9;

    return 0;
}
