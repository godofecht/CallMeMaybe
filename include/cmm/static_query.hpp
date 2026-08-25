#ifndef CALLMEMAYBE_STATIC_QUERY_HPP
#define CALLMEMAYBE_STATIC_QUERY_HPP

#include <type_traits>

#include "cmm/static_meta.hpp"

namespace cmm {

inline const void* const_address_of(cmm::info i)
{
    if (!detail::static_valid(i)) return nullptr;
    const auto* variable = detail::active_static_registry().try_get_as<detail::Variable>(i);
    return variable ? variable->address() : nullptr;
}

inline void* address_of(cmm::info i)
{
    if (!detail::static_valid(i)) return nullptr;
    const auto* variable = detail::active_static_registry().try_get_as<detail::Variable>(i);
    return variable ? variable->mutable_address() : nullptr;
}

inline cmm::info return_type_of(cmm::info i)
{
    if (!detail::static_valid(i)) return invalid_info;
    const auto* function = detail::active_static_registry().try_get_as<detail::Function>(i);
    return function ? function->return_type_id() : invalid_info;
}

inline bool is_function(cmm::info i)
{
    if (!detail::static_valid(i)) return false;
    return detail::active_static_registry().try_get_as<detail::Function>(i) != nullptr;
}

inline bool is_constructor(cmm::info i)
{
    const auto* function = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Function>(i)
        : nullptr;
    return function && function->is_constructor();
}

inline bool is_destructor(cmm::info i)
{
    const auto* function = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Function>(i)
        : nullptr;
    return function && function->is_destructor();
}

inline bool is_const_member_function(cmm::info i)
{
    const auto* function = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Function>(i)
        : nullptr;
    return function && function->is_const_member_function();
}

inline bool is_static_member(cmm::info i)
{
    if (!detail::static_valid(i)) return false;
    return detail::visit_static_entity(i, [](const auto& arg) -> bool
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, detail::Function>) return arg.is_static_function();
        if constexpr (std::is_same_v<T, detail::DataMember>) return arg.is_static();
        return false;
    });
}

inline bool is_nonstatic_data_member(cmm::info i)
{
    if (!detail::static_valid(i)) return false;
    const auto* member = detail::active_static_registry().try_get_as<detail::DataMember>(i);
    return member && !member->is_static();
}

inline bool is_const_data_member(cmm::info i)
{
    if (!detail::static_valid(i)) return false;
    const auto* member = detail::active_static_registry().try_get_as<detail::DataMember>(i);
    return member && member->is_const();
}

inline bool is_enumerator(cmm::info i)
{
    if (!detail::static_valid(i)) return false;
    return detail::active_static_registry().try_get_as<detail::Enumerator>(i) != nullptr;
}

} // namespace cmm

#endif // CALLMEMAYBE_STATIC_QUERY_HPP
