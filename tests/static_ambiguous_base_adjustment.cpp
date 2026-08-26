#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_ambiguous_diamond_registry.hpp"
#include "cmm/static_meta.hpp"

struct StaticDiamondRoot
{
    int value = 1;
};

struct StaticDiamondLeft : public StaticDiamondRoot
{
    int left = 2;
};

struct StaticDiamondRight : public StaticDiamondRoot
{
    int right = 3;
};

struct StaticDiamond : public StaticDiamondLeft, public StaticDiamondRight
{
    int leaf = 4;
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_ambiguous_diamond_static_registry<^^StaticDiamond>()))

int main()
{
    using Metadata = cmm::detail::AmbiguousDiamondStaticMetadata<^^StaticDiamond>;

    const cmm::info diamond_id = cmm::detail::hash_entity(^^StaticDiamond);
    const cmm::info left_id = cmm::detail::hash_entity(^^StaticDiamondLeft);
    const cmm::info right_id = cmm::detail::hash_entity(^^StaticDiamondRight);
    const cmm::info root_id = cmm::detail::hash_entity(^^StaticDiamondRoot);
    const cmm::info diamond_left_base_id = cmm::detail::hash_entity(Metadata::left_base);
    const cmm::info diamond_right_base_id = cmm::detail::hash_entity(Metadata::right_base);
    const cmm::info left_root_base_id = cmm::detail::hash_entity(Metadata::left_root_base);
    const cmm::info right_root_base_id = cmm::detail::hash_entity(Metadata::right_root_base);

    const auto diamond_bases = cmm::bases_view_of(diamond_id);
    if (diamond_bases.size() != 2) return 1;
    if (diamond_bases[0] != diamond_left_base_id) return 2;
    if (diamond_bases[1] != diamond_right_base_id) return 3;
    if (cmm::parent_of(diamond_left_base_id) != diamond_id) return 4;
    if (cmm::parent_of(diamond_right_base_id) != diamond_id) return 5;
    if (cmm::type_of(diamond_left_base_id) != left_id) return 6;
    if (cmm::type_of(diamond_right_base_id) != right_id) return 7;
    if (!cmm::is_public_base(diamond_left_base_id) || cmm::is_virtual_base(diamond_left_base_id)) return 8;
    if (!cmm::is_public_base(diamond_right_base_id) || cmm::is_virtual_base(diamond_right_base_id)) return 9;

    const auto left_bases = cmm::bases_view_of(left_id);
    const auto right_bases = cmm::bases_view_of(right_id);
    if (left_bases.size() != 1 || left_bases[0] != left_root_base_id) return 10;
    if (right_bases.size() != 1 || right_bases[0] != right_root_base_id) return 11;
    if (cmm::parent_of(left_root_base_id) != left_id) return 12;
    if (cmm::parent_of(right_root_base_id) != right_id) return 13;
    if (cmm::type_of(left_root_base_id) != root_id) return 14;
    if (cmm::type_of(right_root_base_id) != root_id) return 15;

    StaticDiamond diamond{};
    const StaticDiamondLeft* native_left = static_cast<const StaticDiamondLeft*>(&diamond);
    const StaticDiamondRight* native_right = static_cast<const StaticDiamondRight*>(&diamond);
    const StaticDiamondRoot* native_left_root = static_cast<const StaticDiamondRoot*>(native_left);
    const StaticDiamondRoot* native_right_root = static_cast<const StaticDiamondRoot*>(native_right);

    if (native_left_root == native_right_root) return 16;

    const void* adjusted = nullptr;
    if (cmm::detail::active_static_registry().adjust_instance_pointer(
            diamond_id, left_id, &diamond, adjusted) != cmm::Error::Success) return 17;
    if (adjusted != native_left) return 18;

    adjusted = nullptr;
    if (cmm::detail::active_static_registry().adjust_instance_pointer(
            diamond_id, right_id, &diamond, adjusted) != cmm::Error::Success) return 19;
    if (adjusted != native_right) return 20;

    adjusted = nullptr;
    if (cmm::detail::active_static_registry().adjust_instance_pointer(
            diamond_id, root_id, &diamond, adjusted) != cmm::Error::InvalidArgumentType) return 21;
    if (adjusted != nullptr) return 22;

    return 0;
}
