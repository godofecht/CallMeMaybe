#ifndef CALLMEMAYBE_STATIC_ENUM_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_ENUM_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <type_traits>
#include <utility>

#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/enumerator.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_function_enum_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info EnumRefl>
consteval auto make_enum_static_registry()
{
    static_assert(std::meta::is_enum_type(EnumRefl));

    static constexpr auto enumerators =
        std::define_static_array(std::meta::enumerators_of(EnumRefl));
    constexpr std::meta::info UnderlyingRefl = std::meta::underlying_type(EnumRefl);

    using EnumT = typename[:EnumRefl:];
    using UnderlyingT = typename[:UnderlyingRefl:];

    const cmm::info enum_id = cmm::detail::hash_entity(EnumRefl);
    const cmm::info underlying_id = cmm::detail::hash_entity(UnderlyingRefl);

    Enum enumeration(std::meta::identifier_of(EnumRefl));
    enumeration.set_display_name(std::meta::display_string_of(EnumRefl));
    enumeration.set_size(sizeof(EnumT));
    enumeration.set_alignment(alignof(EnumT));
    enumeration.set_underlying_type_id(underlying_id);
    enumeration.set_enumerators(StaticEnumMetadata<EnumRefl>::entries);

    Type underlying(std::meta::display_string_of(UnderlyingRefl));
    underlying.set_size(sizeof(UnderlyingT));
    underlying.set_alignment(alignof(UnderlyingT));

    std::array<std::pair<cmm::info, RegistryEntityVariant>, enumerators.size() + 2> entities{};
    entities[0] = {enum_id, enumeration};
    entities[1] = {underlying_id, underlying};

    std::size_t index = 0;
    template for (constexpr std::meta::info enumerator : enumerators)
    {
        const auto& entry = StaticEnumMetadata<EnumRefl>::entries[index];
        Enumerator value(entry.name, entry.value_bits, entry.is_signed);
        value.set_display_name(std::meta::display_string_of(enumerator));
        value.set_parent_id(enum_id);
        entities[index + 2] = {entry.entity_id, value};
        ++index;
    }

    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_ENUM_REGISTRY_HPP
