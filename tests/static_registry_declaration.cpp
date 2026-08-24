#include <array>
#include <string_view>
#include <utility>

#include "cmm/detail/static_active_registry.hpp"

inline constexpr std::array<cmm::info, 1> widget_members{{20}};
inline constexpr std::array<cmm::info, 1> widget_nonstatic_members{{20}};
inline constexpr std::array<std::pair<std::string_view, cmm::info>, 1> widget_member_names{{
    {"value", 20}
}};

consteval auto make_declared_registry()
{
    using V = cmm::detail::RegistryEntityVariant;

    cmm::detail::Type int_type("int");
    int_type.set_size(sizeof(int));
    int_type.set_alignment(alignof(int));

    cmm::detail::Class widget("Widget");
    widget.set_members(widget_members);
    widget.set_nonstatic_data_members(widget_nonstatic_members);
    widget.set_member_names(widget_member_names);

    cmm::detail::DataMember value("value", false);
    value.set_type_id(30);
    value.set_parent_id(10);
    value.set_offset_bytes(0);

    std::array<std::pair<cmm::info, V>, 3> entities{{
        {30, int_type},
        {10, widget},
        {20, value},
    }};
    return cmm::detail::make_static_registry_data(entities);
}

CMM_USE_STATIC_REGISTRY(make_declared_registry())

int main()
{
    const cmm::detail::RegistryView& registry = cmm::detail::active_static_registry();
    if (registry.entity_count() != 3) return 1;
    if (registry.get_id_by_name("Widget") != 10) return 2;
    if (registry.get_id_by_name("value") != 20) return 3;
    if (registry.get_id_by_name("int") != 30) return 4;
    if (registry.get_entity_name(20) != std::string_view("value")) return 5;
    if (registry.get_entity_display_name(10) != std::string_view("Widget")) return 6;
    if (registry.contains(40)) return 7;

    const auto* cls = registry.try_get_as<cmm::detail::Class>(10);
    if (!cls) return 8;
    if (cls->members().size() != 1 || cls->members()[0] != 20) return 9;
    if (cls->get_member_by_name("value") != 20) return 10;

    const auto* member = registry.try_get_as<cmm::detail::DataMember>(20);
    if (!member) return 11;
    if (member->parent_id() != 10) return 12;
    if (member->type_id() != 30) return 13;
    if (member->offset_bytes() != 0) return 14;

    const auto* type = registry.try_get_as<cmm::detail::Type>(30);
    if (!type) return 15;
    if (type->size() != sizeof(int)) return 16;
    if (type->alignment() != alignof(int)) return 17;

    return 0;
}
