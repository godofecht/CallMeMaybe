#include <iostream>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_static_member_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticDataMemberProbe
{
    [[=cmm::reflectable]] inline static int value = 7;
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_single_static_member_registry<^^StaticDataMemberProbe>()))

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

    const cmm::info class_id = cmm::reflect_name("StaticDataMemberProbe");
    ok &= expect(class_id != cmm::invalid_info, "class id exists");

    const auto members = cmm::members_view_of(class_id);
    const auto static_members = cmm::static_data_members_view_of(class_id);
    const auto nonstatic_members = cmm::nonstatic_data_members_view_of(class_id);

    ok &= expect(members.size() == 1, "class member view contains static member");
    ok &= expect(static_members.size() == 1, "static member view contains static member");
    ok &= expect(nonstatic_members.empty(), "nonstatic member view excludes static member");
    if (static_members.size() != 1) return 1;

    const cmm::info member_id = static_members[0];
    ok &= expect(members[0] == member_id, "member and static-member views agree");
    ok &= expect(cmm::identifier_of(member_id) == "value", "static member identifier");
    ok &= expect(cmm::parent_of(member_id) == class_id, "static member parent");
    ok &= expect(cmm::type_of(member_id) == cmm::detail::hash_entity(^^int), "static member type");
    ok &= expect(cmm::is_static_member(member_id), "static-member classification");
    ok &= expect(!cmm::is_nonstatic_data_member(member_id), "nonstatic classification rejected");
    ok &= expect(!cmm::is_const_data_member(member_id), "mutable static member is non-const");
    ok &= expect(cmm::lookup::get_member(class_id, "value") == member_id, "class-local lookup");

    const auto* member =
        cmm::detail::active_static_registry().try_get_as<cmm::detail::DataMember>(member_id);
    ok &= expect(member != nullptr, "static member materialized as DataMember");
    if (!member) return 1;

    ok &= expect(member->address() == &StaticDataMemberProbe::value, "static member address");
    ok &= expect(member->mutable_address() == &StaticDataMemberProbe::value, "mutable static member address");

    cmm::Value copied;
    ok &= expect(member->get_value(copied) == cmm::Error::Success, "static value read");
    ok &= expect(copied.get<int>() == 7, "static copied value");

    cmm::Value reference;
    ok &= expect(member->get_ref(reference) == cmm::Error::Success, "static reference read");
    int* ptr = reference.get_if<int>();
    ok &= expect(ptr != nullptr, "static mutable reference type");
    if (ptr)
    {
        *ptr = 9;
        ok &= expect(StaticDataMemberProbe::value == 9, "static reference write-through");
    }

    ok &= expect(member->set_value(cmm::Value(11)) == cmm::Error::Success, "static setter");
    ok &= expect(StaticDataMemberProbe::value == 11, "static setter mutation");

    cmm::Value ignored;
    ok &= expect(member->get_value(static_cast<const void*>(&StaticDataMemberProbe::value), ignored) ==
                     cmm::Error::StaticMismatch,
                 "instance getter rejects static member");

    return ok ? 0 : 1;
}
