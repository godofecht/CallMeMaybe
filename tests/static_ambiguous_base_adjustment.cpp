#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_ambiguous_diamond_registry.hpp"

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
    const cmm::info diamond_id = cmm::detail::hash_entity(^^StaticDiamond);
    const cmm::info left_id = cmm::detail::hash_entity(^^StaticDiamondLeft);
    const cmm::info right_id = cmm::detail::hash_entity(^^StaticDiamondRight);
    const cmm::info root_id = cmm::detail::hash_entity(^^StaticDiamondRoot);

    StaticDiamond diamond{};
    const StaticDiamondLeft* native_left = static_cast<const StaticDiamondLeft*>(&diamond);
    const StaticDiamondRight* native_right = static_cast<const StaticDiamondRight*>(&diamond);
    const StaticDiamondRoot* native_left_root = static_cast<const StaticDiamondRoot*>(native_left);
    const StaticDiamondRoot* native_right_root = static_cast<const StaticDiamondRoot*>(native_right);

    if (native_left_root == native_right_root) return 1;

    const void* adjusted = nullptr;
    if (cmm::detail::active_static_registry().adjust_instance_pointer(
            diamond_id, left_id, &diamond, adjusted) != cmm::Error::Success) return 2;
    if (adjusted != native_left) return 3;

    adjusted = nullptr;
    if (cmm::detail::active_static_registry().adjust_instance_pointer(
            diamond_id, right_id, &diamond, adjusted) != cmm::Error::Success) return 4;
    if (adjusted != native_right) return 5;

    adjusted = nullptr;
    if (cmm::detail::active_static_registry().adjust_instance_pointer(
            diamond_id, root_id, &diamond, adjusted) != cmm::Error::InvalidArgumentType) return 6;
    if (adjusted != nullptr) return 7;

    return 0;
}
