#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

#include "cmm/annotations.hpp"
#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/data_member.hpp"
#include "cmm/detail/entities/entity.hpp"
#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/enumerator.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/entities/variable.hpp"
#include "cmm/detail/static_class_metadata.hpp"
#include "cmm/meta.hpp"

struct StaticMetadataBase {
    int base_value = 1;
};

struct StaticMetadataProbe : StaticMetadataBase {
    [[=cmm::reflectable]] StaticMetadataProbe() = default;
    [[=cmm::reflectable]] int value = 2;
    [[=cmm::reflectable]] static int shared;
    [[=cmm::reflectable]] int call(int input) { return input + value; }
    [[=cmm::reflectable]] double call(double input) { return input + value; }
    ~StaticMetadataProbe() = default;
};

int StaticMetadataProbe::shared = 3;

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

    const std::array<cmm::info, 4> class_members{201, 202, 203, 204};
    const std::array<cmm::info, 1> class_nonstatic{201};
    const std::array<cmm::info, 1> class_static{202};
    const std::array<cmm::info, 1> class_functions{203};
    const std::array<cmm::info, 1> class_constructors{204};
    const std::array<cmm::info, 2> class_bases{205, 206};
    const std::array<std::pair<std::string_view, cmm::info>, 3> class_names{{
        {"value", 201},
        {"call", cmm::invalid_info},
        {"build", 204},
    }};

    cmm::detail::Class class_type("Widget");
    class_type.set_members(class_members);
    class_type.set_nonstatic_data_members(class_nonstatic);
    class_type.set_static_data_members(class_static);
    class_type.set_functions(class_functions);
    class_type.set_constructors(class_constructors);
    class_type.set_bases(class_bases);
    class_type.set_destructor(207);
    class_type.set_member_names(class_names);
    if (!class_type.flags().is_class) return false;
    if (class_type.members().size() != 4 || class_type.members()[2] != 203) return false;
    if (class_type.nonstatic_data_members()[0] != 201 || class_type.static_data_members()[0] != 202) return false;
    if (class_type.functions()[0] != 203 || class_type.constructors()[0] != 204) return false;
    if (class_type.bases().size() != 2 || class_type.destructor() != 207) return false;
    if (class_type.get_member_by_name("value") != 201) return false;
    if (class_type.get_member_by_name("call") != cmm::invalid_info) return false;
    if (class_type.get_member_by_name("missing") != cmm::invalid_info) return false;

    cmm::detail::Class generated("StaticMetadataProbe");
    cmm::detail::StaticClassMetadata<^^StaticMetadataProbe>::apply(generated);
    if (generated.nonstatic_data_members().size() != 1) return false;
    if (generated.static_data_members().size() != 1) return false;
    if (generated.functions().size() != 2) return false;
    if (generated.constructors().size() != 1) return false;
    if (generated.bases().size() != 1) return false;
    if (generated.destructor() == cmm::invalid_info) return false;
    if (generated.get_member_by_name("value") != cmm::get_id<^^StaticMetadataProbe::value>()) return false;
    if (generated.get_member_by_name("shared") != cmm::get_id<^^StaticMetadataProbe::shared>()) return false;
    if (generated.get_member_by_name("call") != cmm::invalid_info) return false;
    if (generated.bases()[0] != cmm::get_id<^^StaticMetadataBase>()) return false;

    return true;
}

static_assert(constexpr_metadata_roundtrip());

int main()
{
    return constexpr_metadata_roundtrip() ? 0 : 1;
}
