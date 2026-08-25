#include <array>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_base_method_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticAdjustedBase
{
    int state = 17;

    [[=cmm::reflectable]] int increment()
    {
        return ++state;
    }
};

struct StaticAdjustedDerived : public virtual StaticAdjustedBase
{
    int marker = 3;
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_single_base_method_static_registry<^^StaticAdjustedDerived>()))

int main()
{
    constexpr std::meta::info base_refl =
        cmm::detail::SingleBaseStaticMetadata<^^StaticAdjustedDerived>::reflected_base;
    constexpr std::meta::info method_refl =
        cmm::detail::SingleMethodStaticMetadata<^^StaticAdjustedBase>::reflected_method;

    const cmm::info derived_id = cmm::detail::hash_entity(^^StaticAdjustedDerived);
    const cmm::info base_spec_id = cmm::detail::hash_entity(base_refl);
    const cmm::info base_type_id = cmm::detail::hash_entity(^^StaticAdjustedBase);
    const cmm::info method_id = cmm::detail::hash_entity(method_refl);

    const auto bases = cmm::bases_view_of(derived_id);
    if (bases.size() != 1 || bases[0] != base_spec_id) return 1;
    if (!cmm::is_base(base_spec_id)) return 2;
    if (!cmm::is_public_base(base_spec_id)) return 3;
    if (!cmm::is_virtual_base(base_spec_id)) return 4;
    if (cmm::type_of(base_spec_id) != base_type_id) return 5;
    if (cmm::parent_of(base_spec_id) != derived_id) return 6;
    if (cmm::lookup::get_member(base_type_id, "increment") != method_id) return 7;
    if (cmm::parent_of(method_id) != base_type_id) return 8;

    StaticAdjustedDerived derived{};
    StaticAdjustedBase* native_base = static_cast<StaticAdjustedBase*>(&derived);
    if (static_cast<void*>(&derived) == static_cast<void*>(native_base)) return 9;

    std::array<cmm::Value, 1> derived_args{cmm::Value(&derived)};
    cmm::Value derived_result;
    if (cmm::reflect_invoke(method_id, derived_args, derived_result) != cmm::Error::Success) return 10;
    const int* first = derived_result.get_if<int>();
    if (!first || *first != 18 || derived.state != 18) return 11;

    std::array<cmm::Value, 1> base_args{cmm::Value(native_base)};
    cmm::Value base_result;
    if (cmm::reflect_invoke(method_id, base_args, base_result) != cmm::Error::Success) return 12;
    const int* second = base_result.get_if<int>();
    if (!second || *second != 19 || derived.state != 19) return 13;

    const StaticAdjustedDerived* const_derived = &derived;
    std::array<cmm::Value, 1> const_args{cmm::Value(const_derived)};
    cmm::Value ignored;
    if (cmm::reflect_invoke(method_id, const_args, ignored) != cmm::Error::ConstViolation) return 14;
    if (derived.state != 19) return 15;

    return 0;
}
