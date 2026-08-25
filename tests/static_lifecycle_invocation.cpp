#include <array>

#include "cmm/annotations.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_lifecycle_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticLifecycleProbe
{
    inline static int live_instances = 0;

    [[=cmm::reflectable]] StaticLifecycleProbe()
    {
        ++live_instances;
    }

    ~StaticLifecycleProbe()
    {
        --live_instances;
    }
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_static_lifecycle_registry<^^StaticLifecycleProbe>()))

int main()
{
    using Metadata = cmm::detail::StaticLifecycleMetadata<^^StaticLifecycleProbe>;

    const cmm::info class_id = cmm::detail::hash_entity(^^StaticLifecycleProbe);
    const cmm::info constructor_id = Metadata::constructor_id;
    const cmm::info destructor_id = Metadata::destructor_id;

    if (cmm::reflect_name("StaticLifecycleProbe") != class_id) return 1;
    if (cmm::parent_of(constructor_id) != class_id) return 2;
    if (cmm::parent_of(destructor_id) != class_id) return 3;
    if (!cmm::parameters_view_of(constructor_id).empty()) return 4;
    if (!cmm::parameters_view_of(destructor_id).empty()) return 5;

    const auto members = cmm::members_view_of(class_id);
    if (members.size() != 2) return 6;
    if (members[0] != constructor_id || members[1] != destructor_id) return 7;

    std::array<cmm::Value, 0> constructor_args{};
    cmm::Value constructed;
    if (cmm::reflect_invoke(constructor_id, constructor_args, constructed) != cmm::Error::Success) return 8;

    StaticLifecycleProbe** stored_pointer = constructed.get_if<StaticLifecycleProbe*>();
    if (!stored_pointer || !*stored_pointer) return 9;
    StaticLifecycleProbe* instance = *stored_pointer;
    if (StaticLifecycleProbe::live_instances != 1) return 10;

    std::array<cmm::Value, 1> destructor_args{cmm::Value(instance)};
    cmm::Value destroyed;
    if (cmm::reflect_invoke(destructor_id, destructor_args, destroyed) != cmm::Error::Success) return 11;
    if (StaticLifecycleProbe::live_instances != 0) return 12;
    if (destroyed.has_value()) return 13;

    return 0;
}
