#ifndef CALLMEMAYBE_STATIC_LIFECYCLE_HPP
#define CALLMEMAYBE_STATIC_LIFECYCLE_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <type_traits>

#include "cmm/static_meta.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/hash/info_hash.hpp"

namespace cmm {
namespace lookup {

template <typename... Args>
inline cmm::info get_constructor(cmm::info class_id)
{
    if (!detail::static_valid(class_id)) return invalid_info;

    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(class_id);
    if (!cls) return invalid_info;

    constexpr std::size_t N = sizeof...(Args);
    const std::array<cmm::info, N> expected{
        detail::hash_entity(std::meta::remove_cvref(^^Args))...
    };

    for (cmm::info candidate : cls->constructors())
    {
        const auto* function = detail::active_static_registry().try_get_as<detail::Function>(candidate);
        if (!function || !function->is_constructor()) continue;

        const auto params = function->parameter_ids();
        if (params.size() != N) continue;

        bool match = true;
        for (std::size_t i = 0; i < N; ++i)
        {
            const auto* parameter = detail::active_static_registry().try_get_as<detail::Parameter>(params[i]);
            if (!parameter || parameter->decayed_type_id() != expected[i])
            {
                match = false;
                break;
            }
        }

        if (match) return candidate;
    }

    return invalid_info;
}

inline cmm::info get_destructor(cmm::info class_id)
{
    if (!detail::static_valid(class_id)) return invalid_info;

    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(class_id);
    if (!cls) return invalid_info;

    const cmm::info destructor = cls->destructor();
    if (destructor == invalid_info) return invalid_info;

    const auto* function = detail::active_static_registry().try_get_as<detail::Function>(destructor);
    return function && function->is_destructor() ? destructor : invalid_info;
}

} // namespace lookup
} // namespace cmm

#endif // CALLMEMAYBE_STATIC_LIFECYCLE_HPP
