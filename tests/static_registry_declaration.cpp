#include <cstddef>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_member_registry.hpp"

struct StaticWidget
{
    [[=cmm::reflectable]] int value = 0;
};

CMM_USE_STATIC_REGISTRY(cmm::detail::make_single_member_static_registry<^^StaticWidget>())

int main()
{
    const cmm::detail::RegistryView& registry = cmm::detail::active_static_registry();
    const cmm::info widget_id = cmm::detail::hash_entity(^^StaticWidget);

    static constexpr auto members = std::define_static_array(
        std::meta::members_of(^^StaticWidget, std::meta::access_context::unchecked()));
    cmm::info value_id = cmm::invalid_info;
    cmm::info int_id = cmm::invalid_info;
    template for (constexpr std::meta::info member : members)
    {
        if constexpr (cmm::is_reflectable(member) && std::meta::is_nonstatic_data_member(member))
        {
            value_id = cmm::detail::hash_entity(member);
            int_id = cmm::detail::hash_entity(std::meta::type_of(member));
        }
    }

    if (registry.entity_count() != 3) return 1;
    if (registry.get_id_by_name("StaticWidget") != widget_id) return 2;
    if (registry.get_id_by_name("value") != value_id) return 3;
    if (registry.get_id_by_name("int") != int_id) return 4;
    if (registry.get_entity_name(value_id) != std::string_view("value")) return 5;
    if (registry.get_entity_display_name(widget_id) != std::string_view("StaticWidget")) return 6;

    const auto* cls = registry.try_get_as<cmm::detail::Class>(widget_id);
    if (!cls) return 7;
    if (cls->members().size() != 1 || cls->members()[0] != value_id) return 8;
    if (cls->get_member_by_name("value") != value_id) return 9;

    const auto* member = registry.try_get_as<cmm::detail::DataMember>(value_id);
    if (!member) return 10;
    if (member->parent_id() != widget_id) return 11;
    if (member->type_id() != int_id) return 12;
    if (member->offset_bytes() != offsetof(StaticWidget, value)) return 13;

    const auto* type = registry.try_get_as<cmm::detail::Type>(int_id);
    if (!type) return 14;
    if (type->size() != sizeof(int)) return 15;
    if (type->alignment() != alignof(int)) return 16;

    return 0;
}
