#include <array>
#include <utility>

#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/static_registry_view.hpp"

constexpr bool duplicate_id_detection_roundtrip()
{
    using V = cmm::detail::RegistryEntityVariant;

    const std::array<std::pair<cmm::info, V>, 3> unique{{
        {10, cmm::detail::Type("alpha")},
        {20, cmm::detail::Type("beta")},
        {30, cmm::detail::Type("gamma")},
    }};

    const std::array<std::pair<cmm::info, V>, 3> duplicate{{
        {10, cmm::detail::Type("alpha")},
        {20, cmm::detail::Type("beta")},
        {10, cmm::detail::Type("different-entity")},
    }};

    return !cmm::detail::static_registry_has_duplicate_ids(unique) &&
           cmm::detail::static_registry_has_duplicate_ids(duplicate);
}

static_assert(duplicate_id_detection_roundtrip());

int main()
{
    return duplicate_id_detection_roundtrip() ? 0 : 1;
}
