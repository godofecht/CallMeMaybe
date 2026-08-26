#ifndef CALLMEMAYBE_STATIC_SINGLE_STATIC_MEMBER_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_SINGLE_STATIC_MEMBER_REGISTRY_HPP

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
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_registry_view.hpp"
#include "cmm/detail/static_type_registry.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
struct SingleStaticMemberMetadata
{
    inline static constexpr auto reflected_member = []() consteval
    {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));
        std::meta::info result{};
        std::size_t count = 0;
        template for (constexpr std::meta::info member : members)
        {
            if (cmm::is_reflectable(member) && std::meta::is_static_member(member) &&
                !std::meta::is_function(member))
            {
                result = member;
                ++count;
            }
        }
        if (count != 1) throw "single-static-member registry requires exactly one reflected static data member";
        return result;
    }();

    inline static constexpr cmm::info member_id = cmm::detail::hash_entity(reflected_member);
    inline static constexpr std::array<cmm::info, 1> member_ids{{member_id}};
    inline static constexpr std::array<std::pair<std::string_view, cmm::info>, 1> member_names{{
        {std::meta::identifier_of(reflected_member), member_id}
    }};
};

template <std::meta::info ClassRefl>
consteval auto make_single_static_member_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    constexpr std::meta::info member = SingleStaticMemberMetadata<ClassRefl>::reflected_member;
    constexpr std::meta::info MemberTypeRefl = std::meta::type_of(member);
    using ClassT = typename[:ClassRefl:];
    using MemberT = std::remove_cvref_t<typename[:MemberTypeRefl:]>;

    const cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    const cmm::info member_id = cmm::detail::hash_entity(member);
    const cmm::info member_type_id = cmm::detail::hash_entity(MemberTypeRefl);
    constexpr bool member_is_const = std::meta::is_const_type(MemberTypeRefl);

    Class cls(std::meta::display_string_of(ClassRefl));
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    cls.set_flags(make_static_type_flags<ClassRefl>());
    cls.set_members(SingleStaticMemberMetadata<ClassRefl>::member_ids);
    cls.set_static_data_members(SingleStaticMemberMetadata<ClassRefl>::member_ids);
    cls.set_member_names(SingleStaticMemberMetadata<ClassRefl>::member_names);

    DataMember dm(std::meta::identifier_of(member), true);
    dm.set_display_name(std::meta::display_string_of(member));
    dm.set_type_id(member_type_id);
    dm.set_parent_id(class_id);
    dm.set_is_const(member_is_const);
    dm.set_address(const_cast<void*>(static_cast<const void*>(&[:member:])));
    dm.set_static_getter_thunk(&StaticThunks<MemberT>::get);
    if constexpr (member_is_const)
    {
        dm.set_static_ref_getter_thunk(&StaticThunks<MemberT>::get_cref);
    }
    else
    {
        dm.set_static_ref_getter_thunk(&StaticThunks<MemberT>::get_ref);
        dm.set_static_setter_thunk(&StaticThunks<MemberT>::set);
    }

    Type type(std::meta::display_string_of(MemberTypeRefl));
    if constexpr (!std::is_reference_v<typename[:MemberTypeRefl:]> &&
                  !std::is_void_v<typename[:MemberTypeRefl:]> &&
                  !std::is_function_v<typename[:MemberTypeRefl:]>)
    {
        type.set_size(sizeof(typename[:MemberTypeRefl:]));
        type.set_alignment(alignof(typename[:MemberTypeRefl:]));
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

#endif // CALLMEMAYBE_STATIC_SINGLE_STATIC_MEMBER_REGISTRY_HPP
