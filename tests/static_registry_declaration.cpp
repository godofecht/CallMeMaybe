#include <cstddef>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_class_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticCollisionA
{
    [[=cmm::reflectable]] int value = 0;
};

struct StaticCollisionB
{
    double prefix = 0.0;
    [[=cmm::reflectable]] int value = 0;
};

CMM_USE_STATIC_REGISTRY(
    cmm::detail::make_two_class_static_registry<^^StaticCollisionA, ^^StaticCollisionB>())

int main()
{
    const cmm::info a_id = cmm::detail::hash_entity(^^StaticCollisionA);
    const cmm::info b_id = cmm::detail::hash_entity(^^StaticCollisionB);
    const cmm::info a_value = cmm::detail::SingleMemberStaticMetadata<^^StaticCollisionA>::member_id;
    const cmm::info b_value = cmm::detail::SingleMemberStaticMetadata<^^StaticCollisionB>::member_id;
    const cmm::info int_id = cmm::detail::hash_entity(^^int);

    if (cmm::reflect_name("StaticCollisionA") != a_id) return 1;
    if (cmm::reflect_name("StaticCollisionB") != b_id) return 2;
    if (cmm::reflect_name("value") != cmm::invalid_info) return 3;
    if (cmm::reflect_name("int") != int_id) return 4;

    if (a_value == b_value) return 5;
    if (cmm::identifier_of(a_value) != std::string_view("value")) return 6;
    if (cmm::identifier_of(b_value) != std::string_view("value")) return 7;
    if (cmm::parent_of(a_value) != a_id) return 8;
    if (cmm::parent_of(b_value) != b_id) return 9;
    if (cmm::type_of(a_value) != int_id || cmm::type_of(b_value) != int_id) return 10;
    if (cmm::type_of(int_id) != int_id) return 11;

    const auto a_members = cmm::members_view_of(a_id);
    const auto b_members = cmm::members_view_of(b_id);
    if (a_members.size() != 1 || a_members[0] != a_value) return 12;
    if (b_members.size() != 1 || b_members[0] != b_value) return 13;
    if (cmm::lookup::get_member(a_id, "value") != a_value) return 14;
    if (cmm::lookup::get_member(b_id, "value") != b_value) return 15;

    if (cmm::offset_of(a_value) != offsetof(StaticCollisionA, value)) return 16;
    if (cmm::offset_of(b_value) != offsetof(StaticCollisionB, value)) return 17;
    if (cmm::offset_of(a_value) == cmm::offset_of(b_value)) return 18;

    if (cmm::size_of(a_id) != sizeof(StaticCollisionA)) return 19;
    if (cmm::alignment_of(a_id) != alignof(StaticCollisionA)) return 20;
    if (cmm::size_of(b_id) != sizeof(StaticCollisionB)) return 21;
    if (cmm::alignment_of(b_id) != alignof(StaticCollisionB)) return 22;
    if (cmm::size_of(int_id) != sizeof(int)) return 23;
    if (cmm::alignment_of(int_id) != alignof(int)) return 24;

    return 0;
}
