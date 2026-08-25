#include <array>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_single_method_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticMethodProbe
{
    int value = 41;

    [[=cmm::reflectable]] int increment()
    {
        return ++value;
    }
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_single_method_static_registry<^^StaticMethodProbe>()))

int main()
{
    const cmm::info class_id = cmm::detail::hash_entity(^^StaticMethodProbe);
    const cmm::info method_id = cmm::detail::SingleMethodStaticMetadata<^^StaticMethodProbe>::method_id;
    const cmm::info int_id = cmm::detail::hash_entity(^^int);

    if (cmm::reflect_name("StaticMethodProbe") != class_id) return 1;
    if (cmm::reflect_name("increment") != method_id) return 2;
    if (cmm::identifier_of(method_id) != std::string_view("increment")) return 3;
    if (cmm::parent_of(method_id) != class_id) return 4;
    if (cmm::lookup::get_member(class_id, "increment") != method_id) return 5;

    const auto members = cmm::members_view_of(class_id);
    if (members.size() != 1 || members[0] != method_id) return 6;
    if (!cmm::parameters_view_of(method_id).empty()) return 7;
    if (cmm::size_of(class_id) != sizeof(StaticMethodProbe)) return 8;
    if (cmm::alignment_of(class_id) != alignof(StaticMethodProbe)) return 9;
    if (cmm::size_of(int_id) != sizeof(int)) return 10;

    StaticMethodProbe probe{};
    std::array<cmm::Value, 1> args{cmm::Value(&probe)};
    cmm::Value result;
    if (cmm::reflect_invoke(method_id, args, result) != cmm::Error::Success) return 11;

    const int* value = result.get_if<int>();
    if (!value || *value != 42 || probe.value != 42) return 12;

    const StaticMethodProbe const_probe{99};
    std::array<cmm::Value, 1> const_args{cmm::Value(&const_probe)};
    cmm::Value const_result;
    if (cmm::reflect_invoke(method_id, const_args, const_result) != cmm::Error::ConstViolation) return 13;
    if (const_probe.value != 99) return 14;

    return 0;
}
