#include <array>
#include <string_view>
#include <utility>

#include "cmm/detail/static_active_registry.hpp"

consteval auto make_declared_registry()
{
    using V = cmm::detail::RegistryEntityVariant;
    std::array<std::pair<cmm::info, V>, 3> entities{{
        {30, cmm::detail::Type("gamma")},
        {10, cmm::detail::Type("alpha")},
        {20, cmm::detail::Type("beta")},
    }};
    return cmm::detail::make_static_registry_data(entities);
}

CMM_USE_STATIC_REGISTRY(make_declared_registry())

int main()
{
    const cmm::detail::RegistryView& registry = cmm::detail::active_static_registry();
    if (registry.entity_count() != 3) return 1;
    if (registry.get_id_by_name("alpha") != 10) return 2;
    if (registry.get_id_by_name("beta") != 20) return 3;
    if (registry.get_id_by_name("gamma") != 30) return 4;
    if (registry.get_entity_name(20) != std::string_view("beta")) return 5;
    if (registry.contains(40)) return 6;
    return 0;
}
