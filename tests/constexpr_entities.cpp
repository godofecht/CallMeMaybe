#include <array>
#include <cstdint>

#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/data_member.hpp"
#include "cmm/detail/entities/entity.hpp"
#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/enumerator.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/entities/variable.hpp"

constexpr bool constexpr_metadata_roundtrip()
{
    cmm::detail::Entity entity("entity");
    entity.set_display_name("qualified::entity");
    if (entity.name() != "entity" || entity.display_name() != "qualified::entity") return false;

    cmm::detail::Type type("int");
    cmm::detail::TypeFlags flags{};
    flags.is_integral = true;
    flags.is_arithmetic = true;
    flags.is_signed = true;
    type.set_size(4);
    type.set_alignment(4);
    type.set_flags(flags);
    type.set_underlying_type_id(17);
    type.set_array_extent(3);
    if (type.size() != 4 || type.alignment() != 4) return false;
    if (!type.flags().is_integral || !type.flags().is_signed) return false;
    if (type.underlying_type_id() != 17 || type.array_extent() != 3) return false;

    cmm::detail::Parameter parameter("value", 11, 22, 3);
    parameter.set_decayed_type_id(33);
    if (!parameter.has_identifier()) return false;
    if (parameter.type_id() != 11 || parameter.parent_id() != 22) return false;
    if (parameter.index() != 3 || parameter.decayed_type_id() != 33) return false;

    cmm::detail::Enumerator enumerator("High", UINT64_C(0x8000000000000000), false);
    enumerator.set_parent_id(44);
    if (enumerator.value_bits() != UINT64_C(0x8000000000000000)) return false;
    if (enumerator.is_signed() || enumerator.parent_id() != 44) return false;

    cmm::detail::DataMember data_member("field");
    data_member.set_type_id(55);
    data_member.set_parent_id(66);
    data_member.set_offset_bytes(8);
    data_member.set_offset_bits(64);
    data_member.set_is_const(true);
    data_member.set_is_bit_field(false);
    if (data_member.type_id() != 55 || data_member.parent_id() != 66) return false;
    if (data_member.offset_bytes() != 8 || data_member.offset_bits() != 64) return false;
    if (!data_member.is_const() || data_member.is_static() || data_member.is_bit_field()) return false;

    cmm::detail::Variable variable("global", 77);
    variable.set_is_const(true);
    if (variable.type_id() != 77 || !variable.is_const()) return false;
    if (variable.address() != nullptr || variable.mutable_address() != nullptr) return false;

    cmm::detail::Base base("Base", 88, 99);
    base.set_access(cmm::detail::Access::Protected);
    base.set_is_virtual(true);
    if (base.type_id() != 88 || base.parent_id() != 99) return false;
    if (base.access() != cmm::detail::Access::Protected || !base.is_virtual()) return false;
    if (base.is_runtime_accessible()) return false;

    const std::array<cmm::info, 2> parameter_ids{101, 102};
    cmm::detail::Function function("call", true, false);
    function.set_parent_id(103);
    function.set_return_type_id(104);
    function.set_is_const_member_function(true);
    function.set_parameter_ids(parameter_ids);
    if (!function.is_member_function() || function.is_static_function()) return false;
    if (!function.is_const_member_function()) return false;
    if (function.parent_id() != 103 || function.return_type_id() != 104) return false;
    if (function.parameter_ids().size() != 2) return false;
    if (function.parameter_ids()[0] != 101 || function.parameter_ids()[1] != 102) return false;

    const std::array<cmm::detail::Enum::Entry, 2> entries{{
        {"Zero", 0, false, 105},
        {"High", UINT64_C(0x8000000000000000), false, 106},
    }};
    cmm::detail::Enum enum_type("Wide");
    enum_type.set_enumerators(entries);
    std::uint64_t enum_bits = 0;
    bool enum_signed = true;
    if (!enum_type.get_value_bits_by_name("High", enum_bits, enum_signed)) return false;
    if (enum_bits != UINT64_C(0x8000000000000000) || enum_signed) return false;
    if (enum_type.get_name_by_value_bits(enum_bits) != "High") return false;
    if (enum_type.enumerators().size() != 2 || enum_type.enumerators()[1].entity_id != 106) return false;

    return true;
}

static_assert(constexpr_metadata_roundtrip());

int main()
{
    return constexpr_metadata_roundtrip() ? 0 : 1;
}
