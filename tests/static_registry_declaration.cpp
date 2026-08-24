#include <cstddef>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_member_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticWidget
{
    [[=cmm::reflectable]] int value = 0;
};

CMM_USE_STATIC_REGISTRY(cmm::detail::make_single_member_static_registry<^^StaticWidget>())

int main()
{
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

    if (cmm::reflect_name("StaticWidget") != widget_id) return 1;
    if (cmm::reflect_name("value") != value_id) return 2;
    if (cmm::reflect_name("int") != int_id) return 3;
    if (cmm::identifier_of(value_id) != std::string_view("value")) return 4;
    if (cmm::display_string_of(widget_id) != std::string_view("StaticWidget")) return 5;
    if (cmm::type_of(value_id) != int_id) return 6;
    if (cmm::type_of(int_id) != int_id) return 7;
    if (cmm::parent_of(value_id) != widget_id) return 8;

    const auto member_view = cmm::members_view_of(widget_id);
    if (member_view.size() != 1 || member_view[0] != value_id) return 9;
    const auto data_member_view = cmm::nonstatic_data_members_view_of(widget_id);
    if (data_member_view.size() != 1 || data_member_view[0] != value_id) return 10;
    if (cmm::lookup::get_member(widget_id, "value") != value_id) return 11;
    if (cmm::offset_of(value_id) != offsetof(StaticWidget, value)) return 12;
    if (cmm::size_of(widget_id) != sizeof(StaticWidget)) return 13;
    if (cmm::alignment_of(widget_id) != alignof(StaticWidget)) return 14;
    if (cmm::size_of(int_id) != sizeof(int)) return 15;
    if (cmm::alignment_of(int_id) != alignof(int)) return 16;

    return 0;
}
