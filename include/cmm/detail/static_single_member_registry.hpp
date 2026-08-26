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
#include "cmm/detail/static_registry_view.hpp"
#include "cmm/detail/static_type_registry.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
struct SingleMemberStaticMetadata
{
    inline static constexpr auto reflected_member = []() consteval
    {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));
        std::meta::info result{};
        std::size_t count = 0;
        template for (constexpr std::meta::info member : members)
        {
            if (cmm::is_reflectable(member) && std::meta::is_nonstatic_data_member(member))
            {
                result = member;
                ++count;
            }
        }
        if (count != 1) throw "single-member static registry requires exactly one reflected nonstatic data member";
        return result;
    }();

    inline static constexpr cmm::info member_id = cmm::detail::hash_entity(reflected_member);
    inline static constexpr std::array<cmm::info, 1> member_ids{{member_id}};
    inline static constexpr std::array<std::pair<std::string_view, cmm::info>, 1> member_names{{
        {std::meta::identifier_of(reflected_member), member_id}
    }};
};

template <std::meta::info ClassRefl>
consteval auto make_single_member_static_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    constexpr std::meta::info member = SingleMemberStaticMetadata<ClassRefl>::reflected_member;
    constexpr std::meta::info MemberTypeRefl = std::meta::type_of(member);
    using ClassT = typename[:ClassRefl:];
    using MemberT = typename[:MemberTypeRefl:];

    const cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    const cmm::info member_id = cmm::detail::hash_entity(member);
    const cmm::info member_type_id = cmm::detail::hash_entity(MemberTypeRefl);

    Class cls(std::meta::display_string_of(ClassRefl));
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    cls.set_flags(make_static_type_flags<ClassRefl>());
    cls.set_members(SingleMemberStaticMetadata<ClassRefl>::member_ids);
    cls.set_nonstatic_data_members(SingleMemberStaticMetadata<ClassRefl>::member_ids);
    cls.set_member_names(SingleMemberStaticMetadata<ClassRefl>::member_names);

    DataMember dm(std::meta::identifier_of(member), false);
    dm.set_display_name(std::meta::display_string_of(member));
    dm.set_type_id(member_type_id);
    dm.set_parent_id(class_id);
    dm.set_offset_bytes(std::meta::offset_of(member).bytes);
    dm.set_offset_bits(std::meta::offset_of(member).bits);
    dm.set_is_bit_field(std::meta::is_bit_field(member));
    dm.set_is_const(std::meta::is_const_type(MemberTypeRefl));

    Type type(std::meta::display_string_of(MemberTypeRefl));
    if constexpr (!std::is_reference_v<MemberT> && !std::is_void_v<MemberT> && !std::is_function_v<MemberT>)
    {
        type.set_size(sizeof(MemberT));
        type.set_alignment(alignof(MemberT));
    }
    type.set_flags(make_static_type_flags<MemberTypeRefl>());

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 3> entities{{
        {class_id, cls},
        {member_id, dm},
        {member_type_id, type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_SINGLE_MEMBER_REGISTRY_HPP
