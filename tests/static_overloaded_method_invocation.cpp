#include <array>
#include <string_view>

#include "cmm/annotations.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_overloaded_method_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticOverloadedMethodProbe
{
    int value = 40;

    [[=cmm::reflectable]] int call()
    {
        return ++value;
    }

    [[=cmm::reflectable]] int call() const
    {
        return value + 100;
    }
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_overloaded_method_static_registry<^^StaticOverloadedMethodProbe>()))

int main()
{
    using Metadata = cmm::detail::OverloadedMethodStaticMetadata<^^StaticOverloadedMethodProbe>;

    const cmm::info class_id = cmm::detail::hash_entity(^^StaticOverloadedMethodProbe);
    const cmm::info mutable_id = Metadata::mutable_method_id;
    const cmm::info const_id = Metadata::const_method_id;

    if (mutable_id == const_id) return 1;
    if (cmm::reflect_name("StaticOverloadedMethodProbe") != class_id) return 2;
    if (cmm::reflect_name("call") != cmm::invalid_info) return 3;
    if (cmm::lookup::get_member(class_id, "call") != cmm::invalid_info) return 4;
    if (cmm::parent_of(mutable_id) != class_id || cmm::parent_of(const_id) != class_id) return 5;
    if (cmm::identifier_of(mutable_id) != std::string_view("call")) return 6;
    if (cmm::identifier_of(const_id) != std::string_view("call")) return 7;

    const auto members = cmm::members_view_of(class_id);
    if (members.size() != 2) return 8;
    if (!cmm::parameters_view_of(mutable_id).empty()) return 9;
    if (!cmm::parameters_view_of(const_id).empty()) return 10;

    StaticOverloadedMethodProbe mutable_probe{};
    std::array<cmm::Value, 1> mutable_args{cmm::Value(&mutable_probe)};
    cmm::Value mutable_result;
    if (cmm::reflect_invoke(mutable_id, mutable_args, mutable_result) != cmm::Error::Success) return 11;
    const int* mutable_value = mutable_result.get_if<int>();
    if (!mutable_value || *mutable_value != 41 || mutable_probe.value != 41) return 12;

    const StaticOverloadedMethodProbe const_probe{7};
    std::array<cmm::Value, 1> const_args{cmm::Value(&const_probe)};
    cmm::Value const_result;
    if (cmm::reflect_invoke(const_id, const_args, const_result) != cmm::Error::Success) return 13;
    const int* const_value = const_result.get_if<int>();
    if (!const_value || *const_value != 107 || const_probe.value != 7) return 14;

    cmm::Value rejected_result;
    if (cmm::reflect_invoke(mutable_id, const_args, rejected_result) != cmm::Error::ConstViolation) return 15;

    return 0;
}
