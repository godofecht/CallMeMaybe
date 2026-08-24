#ifndef CALLMEMAYBE_STATIC_SINGLE_MEMBER_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_SINGLE_MEMBER_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "cmm/annotations.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/data_member.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_class_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
consteval auto make_single_member_static_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    static constexpr auto members = std::define_static_array(
        std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));

    std::size_t reflected_data_members = 0;
    template for (constexpr std::meta::info member : members)
    {
        if (cmm::is_reflectable(member) && std::meta::is_nonstatic_data_member(member))
            ++reflected_data_members;
    }
    if (reflected_data_members != 1) throw "single-member static registry requires exactly one reflected nonstatic data member";

    RegistryEntityVariant class_entity;
    RegistryEntityVariant member_entity;
    RegistryEntityVariant type_entity;
    cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    cmm::info member_id = cmm::invalid_info;
    cmm::info member_type_id = cmm::invalid_info;

    Class cls(std::meta::display_string_of(ClassRefl));
    using ClassT = typename[:ClassRefl:];
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    StaticClassMetadata<ClassRefl>::apply(cls);
    class_entity = cls;

    template for (constexpr std::meta::info member : members)
    {
        if constexpr (cmm::is_reflectable(member) && std::meta::is_nonstatic_data_member(member))
        {
            constexpr std::meta::info MemberTypeRefl = std::meta::type_of(member);
            using MemberT = typename[:MemberTypeRefl:];

            member_id = cmm::detail::hash_entity(member);
            member_type_id = cmm::detail::hash_entity(MemberTypeRefl);

            DataMember dm(std::meta::identifier_of(member), false);
            dm.set_display_name(std::meta::display_string_of(member));
            dm.set_type_id(member_type_id);
            dm.set_parent_id(class_id);
            dm.set_offset_bytes(std::meta::offset_of(member).bytes);
            dm.set_offset_bits(std::meta::offset_of(member).bits);
            dm.set_is_bit_field(std::meta::is_bit_field(member));
            dm.set_is_const(std::meta::is_const_type(MemberTypeRefl));
            member_entity = dm;

            Type type(std::meta::display_string_of(MemberTypeRefl));
            if constexpr (!std::is_reference_v<MemberT> && !std::is_void_v<MemberT> && !std::is_function_v<MemberT>)
            {
                type.set_size(sizeof(MemberT));
                type.set_alignment(alignof(MemberT));
            }
            type_entity = type;
        }
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 3> entities{{
        {class_id, class_entity},
        {member_id, member_entity},
        {member_type_id, type_entity},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_SINGLE_MEMBER_REGISTRY_HPP
