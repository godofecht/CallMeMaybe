#ifndef CALLMEMAYBE_STATIC_MIXED_MEMBER_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_MIXED_MEMBER_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "cmm/annotations.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/data_member.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
struct MixedMemberStaticMetadata
{
    inline static constexpr auto reflected_members = []() consteval
    {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));
        std::array<std::meta::info, 3> result{};
        std::size_t count = 0;
        template for (constexpr std::meta::info member : members)
        {
            if (cmm::is_reflectable(member) &&
                (std::meta::is_nonstatic_data_member(member) ||
                 (std::meta::is_function(member) && !std::meta::is_static_member(member))))
            {
                if (count >= result.size()) throw "mixed-member static registry requires exactly three reflected members";
                result[count++] = member;
            }
        }
        if (count != result.size()) throw "mixed-member static registry requires exactly three reflected members";
        if (!std::meta::is_nonstatic_data_member(result[0]) ||
            !std::meta::is_function(result[1]) ||
            !std::meta::is_nonstatic_data_member(result[2]))
        {
            throw "mixed-member static registry requires data/function/data declaration order";
        }
        return result;
    }();

    inline static constexpr std::array<cmm::info, 3> member_ids{{
        cmm::detail::hash_entity(reflected_members[0]),
        cmm::detail::hash_entity(reflected_members[1]),
        cmm::detail::hash_entity(reflected_members[2]),
    }};

    inline static constexpr std::array<cmm::info, 2> data_member_ids{{
        member_ids[0], member_ids[2]
    }};

    inline static constexpr std::array<std::pair<std::string_view, cmm::info>, 3> member_names{{
        {std::meta::identifier_of(reflected_members[0]), member_ids[0]},
        {std::meta::identifier_of(reflected_members[1]), member_ids[1]},
        {std::meta::identifier_of(reflected_members[2]), member_ids[2]},
    }};
};

template <std::meta::info ClassRefl>
consteval auto make_mixed_member_static_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    constexpr std::meta::info first = MixedMemberStaticMetadata<ClassRefl>::reflected_members[0];
    constexpr std::meta::info middle = MixedMemberStaticMetadata<ClassRefl>::reflected_members[1];
    constexpr std::meta::info last = MixedMemberStaticMetadata<ClassRefl>::reflected_members[2];
    static constexpr auto middle_parameters = std::define_static_array(std::meta::parameters_of(middle));
    static_assert(middle_parameters.empty(), "mixed-member static registry currently requires a zero-argument method");

    constexpr std::meta::info DataTypeRefl = std::meta::type_of(first);
    constexpr std::meta::info LastTypeRefl = std::meta::type_of(last);
    constexpr std::meta::info ReturnTypeRefl = std::meta::return_type_of(middle);
    static_assert(DataTypeRefl == LastTypeRefl,
                  "mixed-member static registry currently requires matching data-member types");

    using ClassT = typename[:ClassRefl:];
    using DataT = typename[:DataTypeRefl:];
    using ReturnT = typename[:ReturnTypeRefl:];

    const cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    const cmm::info first_id = cmm::detail::hash_entity(first);
    const cmm::info middle_id = cmm::detail::hash_entity(middle);
    const cmm::info last_id = cmm::detail::hash_entity(last);
    const cmm::info data_type_id = cmm::detail::hash_entity(DataTypeRefl);
    const cmm::info return_type_id = cmm::detail::hash_entity(ReturnTypeRefl);

    Class cls(std::meta::display_string_of(ClassRefl));
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    cls.set_members(MixedMemberStaticMetadata<ClassRefl>::member_ids);
    cls.set_nonstatic_data_members(MixedMemberStaticMetadata<ClassRefl>::data_member_ids);
    cls.set_member_names(MixedMemberStaticMetadata<ClassRefl>::member_names);

    DataMember first_member(std::meta::identifier_of(first), false);
    first_member.set_display_name(std::meta::display_string_of(first));
    first_member.set_type_id(data_type_id);
    first_member.set_parent_id(class_id);
    first_member.set_offset_bytes(std::meta::offset_of(first).bytes);
    first_member.set_offset_bits(std::meta::offset_of(first).bits);
    first_member.set_is_bit_field(std::meta::is_bit_field(first));
    first_member.set_is_const(std::meta::is_const_type(DataTypeRefl));

    Function middle_member(std::meta::identifier_of(middle), true, false);
    middle_member.set_display_name(std::meta::display_string_of(middle));
    middle_member.set_parent_id(class_id);
    middle_member.set_return_type_id(return_type_id);
    middle_member.set_is_const_member_function(std::meta::is_const(middle));
    middle_member.set_thunk(cmm::detail::create_thunk<middle>());

    DataMember last_member(std::meta::identifier_of(last), false);
    last_member.set_display_name(std::meta::display_string_of(last));
    last_member.set_type_id(data_type_id);
    last_member.set_parent_id(class_id);
    last_member.set_offset_bytes(std::meta::offset_of(last).bytes);
    last_member.set_offset_bits(std::meta::offset_of(last).bits);
    last_member.set_is_bit_field(std::meta::is_bit_field(last));
    last_member.set_is_const(std::meta::is_const_type(LastTypeRefl));

    Type data_type(std::meta::display_string_of(DataTypeRefl));
    if constexpr (!std::is_reference_v<DataT> && !std::is_void_v<DataT> && !std::is_function_v<DataT>)
    {
        data_type.set_size(sizeof(DataT));
        data_type.set_alignment(alignof(DataT));
    }

    Type return_type(std::meta::display_string_of(ReturnTypeRefl));
    if constexpr (!std::is_reference_v<ReturnT> && !std::is_void_v<ReturnT> && !std::is_function_v<ReturnT>)
    {
        return_type.set_size(sizeof(ReturnT));
        return_type.set_alignment(alignof(ReturnT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 6> entities{{
        {class_id, cls},
        {first_id, first_member},
        {middle_id, middle_member},
        {last_id, last_member},
        {data_type_id, data_type},
        {return_type_id, return_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_MIXED_MEMBER_REGISTRY_HPP
