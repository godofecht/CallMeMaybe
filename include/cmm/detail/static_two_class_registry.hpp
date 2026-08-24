#ifndef CALLMEMAYBE_STATIC_TWO_CLASS_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_TWO_CLASS_REGISTRY_HPP

#include <array>
#include <meta>
#include <type_traits>
#include <utility>

#include "cmm/detail/static_single_member_registry.hpp"

namespace cmm::detail {

template <std::meta::info FirstClassRefl, std::meta::info SecondClassRefl>
consteval auto make_two_class_static_registry()
{
    static_assert(std::meta::is_class_type(FirstClassRefl) || std::meta::is_union_type(FirstClassRefl));
    static_assert(std::meta::is_class_type(SecondClassRefl) || std::meta::is_union_type(SecondClassRefl));

    constexpr std::meta::info first_member = SingleMemberStaticMetadata<FirstClassRefl>::reflected_member;
    constexpr std::meta::info second_member = SingleMemberStaticMetadata<SecondClassRefl>::reflected_member;
    constexpr std::meta::info FirstMemberTypeRefl = std::meta::type_of(first_member);
    constexpr std::meta::info SecondMemberTypeRefl = std::meta::type_of(second_member);
    static_assert(cmm::detail::hash_entity(FirstMemberTypeRefl) == cmm::detail::hash_entity(SecondMemberTypeRefl),
                  "two-class static registry currently requires the reflected members to share one type");

    using FirstClassT = typename[:FirstClassRefl:];
    using SecondClassT = typename[:SecondClassRefl:];
    using MemberT = typename[:FirstMemberTypeRefl:];

    const cmm::info first_class_id = cmm::detail::hash_entity(FirstClassRefl);
    const cmm::info second_class_id = cmm::detail::hash_entity(SecondClassRefl);
    const cmm::info first_member_id = cmm::detail::hash_entity(first_member);
    const cmm::info second_member_id = cmm::detail::hash_entity(second_member);
    const cmm::info member_type_id = cmm::detail::hash_entity(FirstMemberTypeRefl);

    Class first_class(std::meta::display_string_of(FirstClassRefl));
    first_class.set_size(sizeof(FirstClassT));
    first_class.set_alignment(alignof(FirstClassT));
    first_class.set_members(SingleMemberStaticMetadata<FirstClassRefl>::member_ids);
    first_class.set_nonstatic_data_members(SingleMemberStaticMetadata<FirstClassRefl>::member_ids);
    first_class.set_member_names(SingleMemberStaticMetadata<FirstClassRefl>::member_names);

    DataMember first_dm(std::meta::identifier_of(first_member), false);
    first_dm.set_display_name(std::meta::display_string_of(first_member));
    first_dm.set_type_id(member_type_id);
    first_dm.set_parent_id(first_class_id);
    first_dm.set_offset_bytes(std::meta::offset_of(first_member).bytes);
    first_dm.set_offset_bits(std::meta::offset_of(first_member).bits);
    first_dm.set_is_bit_field(std::meta::is_bit_field(first_member));
    first_dm.set_is_const(std::meta::is_const_type(FirstMemberTypeRefl));

    Class second_class(std::meta::display_string_of(SecondClassRefl));
    second_class.set_size(sizeof(SecondClassT));
    second_class.set_alignment(alignof(SecondClassT));
    second_class.set_members(SingleMemberStaticMetadata<SecondClassRefl>::member_ids);
    second_class.set_nonstatic_data_members(SingleMemberStaticMetadata<SecondClassRefl>::member_ids);
    second_class.set_member_names(SingleMemberStaticMetadata<SecondClassRefl>::member_names);

    DataMember second_dm(std::meta::identifier_of(second_member), false);
    second_dm.set_display_name(std::meta::display_string_of(second_member));
    second_dm.set_type_id(member_type_id);
    second_dm.set_parent_id(second_class_id);
    second_dm.set_offset_bytes(std::meta::offset_of(second_member).bytes);
    second_dm.set_offset_bits(std::meta::offset_of(second_member).bits);
    second_dm.set_is_bit_field(std::meta::is_bit_field(second_member));
    second_dm.set_is_const(std::meta::is_const_type(SecondMemberTypeRefl));

    Type member_type(std::meta::display_string_of(FirstMemberTypeRefl));
    if constexpr (!std::is_reference_v<MemberT> && !std::is_void_v<MemberT> && !std::is_function_v<MemberT>)
    {
        member_type.set_size(sizeof(MemberT));
        member_type.set_alignment(alignof(MemberT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 5> entities{{
        {first_class_id, first_class},
        {first_member_id, first_dm},
        {second_class_id, second_class},
        {second_member_id, second_dm},
        {member_type_id, member_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_TWO_CLASS_REGISTRY_HPP
