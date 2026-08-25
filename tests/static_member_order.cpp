#include <cstddef>
#include <iostream>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_mixed_member_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticOrderProbe
{
    [[=cmm::reflectable]] int first = 0;
    [[=cmm::reflectable]] void middle() {}
    [[=cmm::reflectable]] int last = 0;
};

CMM_DEFINE_STATIC_REGISTRY(
    cmm::detail::make_mixed_member_static_registry<^^StaticOrderProbe>());

static bool expect(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

int main()
{
    bool ok = true;

    const cmm::info class_id = cmm::reflect_name("StaticOrderProbe");
    const auto members = cmm::members_view_of(class_id);
    const auto data_members = cmm::nonstatic_data_members_view_of(class_id);

    ok &= expect(class_id != cmm::invalid_info, "class id exists");
    ok &= expect(members.size() == 3, "mixed member view size");
    ok &= expect(data_members.size() == 2, "data member view size");

    if (members.size() == 3)
    {
        ok &= expect(cmm::identifier_of(members[0]) == "first", "first declaration remains first");
        ok &= expect(cmm::identifier_of(members[1]) == "middle", "method remains between data members");
        ok &= expect(cmm::identifier_of(members[2]) == "last", "last declaration remains last");
        ok &= expect(cmm::parent_of(members[0]) == class_id, "first parent");
        ok &= expect(cmm::parent_of(members[1]) == class_id, "middle parent");
        ok &= expect(cmm::parent_of(members[2]) == class_id, "last parent");
    }

    if (data_members.size() == 2)
    {
        ok &= expect(cmm::identifier_of(data_members[0]) == "first", "first data member order");
        ok &= expect(cmm::identifier_of(data_members[1]) == "last", "last data member order");
        ok &= expect(cmm::offset_of(data_members[0]) == offsetof(StaticOrderProbe, first), "first offset");
        ok &= expect(cmm::offset_of(data_members[1]) == offsetof(StaticOrderProbe, last), "last offset");
    }

    ok &= expect(cmm::lookup::get_member(class_id, "first") == members[0], "first lookup");
    ok &= expect(cmm::lookup::get_member(class_id, "middle") == members[1], "middle lookup");
    ok &= expect(cmm::lookup::get_member(class_id, "last") == members[2], "last lookup");

    return ok ? 0 : 1;
}
