#ifndef CALLMEMAYBE_STATIC_ACTIVE_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_ACTIVE_REGISTRY_HPP

#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

extern const RegistryView g_active_static_registry;

inline const RegistryView& active_static_registry() noexcept
{
    return g_active_static_registry;
}

} // namespace cmm::detail

#define CMM_USE_STATIC_REGISTRY(DataExpression)                                      \
    namespace cmm::detail {                                                         \
    inline constexpr auto g_active_static_registry_data = (DataExpression);          \
    constinit const RegistryView g_active_static_registry{g_active_static_registry_data}; \
    }

#endif // CALLMEMAYBE_STATIC_ACTIVE_REGISTRY_HPP
