#ifndef CALLMEMAYBE_STATIC_AMBIGUOUS_DIAMOND_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_AMBIGUOUS_DIAMOND_REGISTRY_HPP

#include <array>
#include <meta>

#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_class_metadata.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info DiamondRefl>
struct AmbiguousDiamondStaticMetadata
{
    inline static constexpr auto diamond_bases = []() consteval
    {
        static constexpr auto bases = std::define_static_array(
            std::meta::bases_of(DiamondRefl, std::meta::access_context::unchecked()));
        if (bases.size() != 2) throw "ambiguous-diamond registry requires two direct bases";
        return bases;
    }();

    inline static constexpr std::meta::info left_base = diamond_bases[0];
    inline static constexpr std::meta::info right_base = diamond_bases[1];
    inline static constexpr std::meta::info left_type = std::meta::type_of(left_base);
    inline static constexpr std::meta::info right_type = std::meta::type_of(right_base);

    inline static constexpr auto left_bases = []() consteval
    {
        static constexpr auto bases = std::define_static_array(
            std::meta::bases_of(left_type, std::meta::access_context::unchecked()));
        if (bases.size() != 1) throw "left branch must have exactly one direct base";
        return bases;
    }();

    inline static constexpr auto right_bases = []() consteval
    {
        static constexpr auto bases = std::define_static_array(
            std::meta::bases_of(right_type, std::meta::access_context::unchecked()));
        if (bases.size() != 1) throw "right branch must have exactly one direct base";
        return bases;
    }();

    inline static constexpr std::meta::info left_root_base = left_bases[0];
    inline static constexpr std::meta::info right_root_base = right_bases[0];
    inline static constexpr std::meta::info root_type = std::meta::type_of(left_root_base);

    static_assert(std::meta::type_of(right_root_base) == root_type,
                  "both branches must converge on the same root type");
};

template <std::meta::info DiamondRefl>
consteval auto make_ambiguous_diamond_static_registry()
{
    static_assert(std::meta::is_class_type(DiamondRefl));

    using Metadata = AmbiguousDiamondStaticMetadata<DiamondRefl>;
    constexpr std::meta::info LeftRefl = Metadata::left_type;
    constexpr std::meta::info RightRefl = Metadata::right_type;
    constexpr std::meta::info RootRefl = Metadata::root_type;

    using DiamondT = typename[:DiamondRefl:];
    using LeftT = typename[:LeftRefl:];
    using RightT = typename[:RightRefl:];
    using RootT = typename[:RootRefl:];

    const cmm::info diamond_id = cmm::detail::hash_entity(DiamondRefl);
    const cmm::info left_id = cmm::detail::hash_entity(LeftRefl);
    const cmm::info right_id = cmm::detail::hash_entity(RightRefl);
    const cmm::info root_id = cmm::detail::hash_entity(RootRefl);

    const cmm::info diamond_left_id = cmm::detail::hash_entity(Metadata::left_base);
    const cmm::info diamond_right_id = cmm::detail::hash_entity(Metadata::right_base);
    const cmm::info left_root_id = cmm::detail::hash_entity(Metadata::left_root_base);
    const cmm::info right_root_id = cmm::detail::hash_entity(Metadata::right_root_base);

    Class diamond(std::meta::display_string_of(DiamondRefl));
    diamond.set_size(sizeof(DiamondT));
    diamond.set_alignment(alignof(DiamondT));
    diamond.set_bases(StaticClassMetadata<DiamondRefl>::base_ids);

    Class left(std::meta::display_string_of(LeftRefl));
    left.set_size(sizeof(LeftT));
    left.set_alignment(alignof(LeftT));
    left.set_bases(StaticClassMetadata<LeftRefl>::base_ids);

    Class right(std::meta::display_string_of(RightRefl));
    right.set_size(sizeof(RightT));
    right.set_alignment(alignof(RightT));
    right.set_bases(StaticClassMetadata<RightRefl>::base_ids);

    Class root(std::meta::display_string_of(RootRefl));
    root.set_size(sizeof(RootT));
    root.set_alignment(alignof(RootT));

    Base diamond_left(std::meta::display_string_of(Metadata::left_base), left_id, diamond_id);
    diamond_left.set_access(Access::Public);
    diamond_left.set_upcast_thunk(+[](const void* instance) -> const void*
    {
        return static_cast<const LeftT*>(static_cast<const DiamondT*>(instance));
    });

    Base diamond_right(std::meta::display_string_of(Metadata::right_base), right_id, diamond_id);
    diamond_right.set_access(Access::Public);
    diamond_right.set_upcast_thunk(+[](const void* instance) -> const void*
    {
        return static_cast<const RightT*>(static_cast<const DiamondT*>(instance));
    });

    Base left_root(std::meta::display_string_of(Metadata::left_root_base), root_id, left_id);
    left_root.set_access(Access::Public);
    left_root.set_upcast_thunk(+[](const void* instance) -> const void*
    {
        return static_cast<const RootT*>(static_cast<const LeftT*>(instance));
    });

    Base right_root(std::meta::display_string_of(Metadata::right_root_base), root_id, right_id);
    right_root.set_access(Access::Public);
    right_root.set_upcast_thunk(+[](const void* instance) -> const void*
    {
        return static_cast<const RootT*>(static_cast<const RightT*>(instance));
    });

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 8> entities{{
        {diamond_id, diamond},
        {left_id, left},
        {right_id, right},
        {root_id, root},
        {diamond_left_id, diamond_left},
        {diamond_right_id, diamond_right},
        {left_root_id, left_root},
        {right_root_id, right_root},
    }};

    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_AMBIGUOUS_DIAMOND_REGISTRY_HPP
