#include <cstdint>
#include <string_view>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_enum_registry.hpp"
#include "cmm/static_meta.hpp"

enum class StaticHighBit : std::uint64_t
{
    Zero = 0,
    High = 0x8000000000000000ULL,
    Max = 0xffffffffffffffffULL
};

CMM_USE_STATIC_REGISTRY((cmm::detail::make_enum_static_registry<^^StaticHighBit>()))

int main()
{
    const cmm::info enum_id = cmm::detail::hash_entity(^^StaticHighBit);
    const cmm::info underlying_id = cmm::detail::hash_entity(^^std::uint64_t);
    const auto enumerators = cmm::enumerators_of(enum_id);

    if (cmm::reflect_name("StaticHighBit") != enum_id) return 1;
    if (cmm::underlying_type(enum_id) != underlying_id) return 2;
    if (cmm::size_of(enum_id) != sizeof(StaticHighBit)) return 3;
    if (cmm::alignment_of(enum_id) != alignof(StaticHighBit)) return 4;
    if (enumerators.size() != 3) return 5;

    const cmm::info zero_id = cmm::reflect_name("Zero");
    const cmm::info high_id = cmm::reflect_name("High");
    const cmm::info max_id = cmm::reflect_name("Max");
    if (zero_id == cmm::invalid_info || high_id == cmm::invalid_info || max_id == cmm::invalid_info) return 6;

    if (cmm::parent_of(zero_id) != enum_id || cmm::type_of(zero_id) != enum_id) return 7;
    if (cmm::parent_of(high_id) != enum_id || cmm::type_of(high_id) != enum_id) return 8;
    if (cmm::parent_of(max_id) != enum_id || cmm::type_of(max_id) != enum_id) return 9;

    if (static_cast<std::uint64_t>(cmm::value_of(zero_id)) != 0ULL) return 10;
    if (static_cast<std::uint64_t>(cmm::value_of(high_id)) != 0x8000000000000000ULL) return 11;
    if (static_cast<std::uint64_t>(cmm::value_of(max_id)) != 0xffffffffffffffffULL) return 12;

    const auto* high = cmm::detail::active_static_registry().try_get_as<cmm::detail::Enumerator>(high_id);
    const auto* max = cmm::detail::active_static_registry().try_get_as<cmm::detail::Enumerator>(max_id);
    if (!high || !max) return 13;
    if (high->is_signed() || max->is_signed()) return 14;
    if (high->value_bits() != 0x8000000000000000ULL) return 15;
    if (max->value_bits() != 0xffffffffffffffffULL) return 16;

    if (cmm::lookup::enum_to_string(enum_id, cmm::value_of(high_id)) != std::string_view("High")) return 17;
    std::int64_t parsed = 0;
    if (!cmm::lookup::string_to_enum(enum_id, "Max", parsed)) return 18;
    if (static_cast<std::uint64_t>(parsed) != 0xffffffffffffffffULL) return 19;

    return 0;
}
