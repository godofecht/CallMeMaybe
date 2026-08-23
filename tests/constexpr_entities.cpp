#include <cstdint>

#include "cmm/detail/entities/entity.hpp"
#include "cmm/detail/entities/enumerator.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/entities/type.hpp"

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

    return true;
}

static_assert(constexpr_metadata_roundtrip());

int main()
{
    return constexpr_metadata_roundtrip() ? 0 : 1;
}
