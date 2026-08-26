#ifndef CALLMEMAYBE_STATIC_META_HPP
#define CALLMEMAYBE_STATIC_META_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "cmm/info.hpp"
#include "cmm/value.hpp"
#include "cmm/detail/static_active_registry.hpp"

namespace cmm {

inline cmm::info reflect_name(std::string_view name)
{
    return detail::active_static_registry().get_id_by_name(name);
}

namespace detail {

inline bool static_valid(cmm::info i)
{
    return i != invalid_info && active_static_registry().contains(i);
}

template <typename Visitor>
inline auto visit_static_entity(cmm::info i, Visitor&& visitor)
{
    return std::visit(std::forward<Visitor>(visitor), active_static_registry().get_entity(i));
}

} // namespace detail

inline std::string_view identifier_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    return detail::active_static_registry().get_entity_name(i);
}

inline std::string_view display_string_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    return detail::active_static_registry().get_entity_display_name(i);
}

inline cmm::info type_of(cmm::info i)
{
    if (!detail::static_valid(i)) return invalid_info;
    return detail::visit_static_entity(i, [i](const auto& arg) -> cmm::info
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, detail::DataMember> ||
                      std::is_same_v<T, detail::Parameter> ||
                      std::is_same_v<T, detail::Variable> ||
                      std::is_same_v<T, detail::Base>)
        {
            return arg.type_id();
        }
        else if constexpr (std::is_same_v<T, detail::Enumerator>)
        {
            return arg.parent_id();
        }
        else if constexpr (std::is_base_of_v<detail::Type, T>)
        {
            return i;
        }
        return invalid_info;
    });
}

inline cmm::info parent_of(cmm::info i)
{
    if (!detail::static_valid(i)) return invalid_info;
    return detail::visit_static_entity(i, [](const auto& arg) -> cmm::info
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, detail::DataMember> ||
                      std::is_same_v<T, detail::Function> ||
                      std::is_same_v<T, detail::Enumerator> ||
                      std::is_same_v<T, detail::Parameter> ||
                      std::is_same_v<T, detail::Base>)
        {
            return arg.parent_id();
        }
        return invalid_info;
    });
}

inline cmm::info underlying_type(cmm::info i)
{
    if (!detail::static_valid(i)) return invalid_info;
    return detail::visit_static_entity(i, [](const auto& arg) -> cmm::info
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_base_of_v<detail::Type, T>) return arg.underlying_type_id();
        return invalid_info;
    });
}

inline std::span<const cmm::info> members_view_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(i);
    return cls ? cls->members() : std::span<const cmm::info>{};
}

inline std::vector<cmm::info> members_of(cmm::info i)
{
    const auto view = members_view_of(i);
    return {view.begin(), view.end()};
}

inline std::span<const cmm::info> nonstatic_data_members_view_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(i);
    return cls ? cls->nonstatic_data_members() : std::span<const cmm::info>{};
}

inline std::vector<cmm::info> nonstatic_data_members_of(cmm::info i)
{
    const auto view = nonstatic_data_members_view_of(i);
    return {view.begin(), view.end()};
}

inline std::span<const cmm::info> static_data_members_view_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(i);
    return cls ? cls->static_data_members() : std::span<const cmm::info>{};
}

inline std::vector<cmm::info> static_data_members_of(cmm::info i)
{
    const auto view = static_data_members_view_of(i);
    return {view.begin(), view.end()};
}

inline std::span<const cmm::info> bases_view_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(i);
    return cls ? cls->bases() : std::span<const cmm::info>{};
}

inline std::vector<cmm::info> bases_of(cmm::info i)
{
    const auto view = bases_view_of(i);
    return {view.begin(), view.end()};
}

inline bool is_base(cmm::info i)
{
    if (!detail::static_valid(i)) return false;
    return detail::active_static_registry().try_get_as<detail::Base>(i) != nullptr;
}

inline bool is_virtual_base(cmm::info i)
{
    const auto* base = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Base>(i)
        : nullptr;
    return base && base->is_virtual();
}

inline bool is_public_base(cmm::info i)
{
    const auto* base = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Base>(i)
        : nullptr;
    return base && base->access() == detail::Access::Public;
}

inline bool is_protected_base(cmm::info i)
{
    const auto* base = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Base>(i)
        : nullptr;
    return base && base->access() == detail::Access::Protected;
}

inline bool is_private_base(cmm::info i)
{
    const auto* base = detail::static_valid(i)
        ? detail::active_static_registry().try_get_as<detail::Base>(i)
        : nullptr;
    return base && base->access() == detail::Access::Private;
}

inline std::vector<cmm::info> enumerators_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    const auto* enumeration = detail::active_static_registry().try_get_as<detail::Enum>(i);
    if (!enumeration) return {};

    std::vector<cmm::info> result;
    result.reserve(enumeration->enumerators().size());
    for (const auto& entry : enumeration->enumerators()) result.push_back(entry.entity_id);
    return result;
}

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

inline cmm::Error try_value_of(cmm::info i, std::int64_t& out)
{
    if (!detail::static_valid(i)) return cmm::Error::EntityNotFound;
    const auto* enumerator = detail::active_static_registry().try_get_as<detail::Enumerator>(i);
    if (!enumerator) return cmm::Error::TypeMismatch;
    out = enumerator->value();
    return cmm::Error::Success;
}

inline std::int64_t value_of(cmm::info i)
{
    std::int64_t value = 0;
    (void)try_value_of(i, value);
    return value;
}

inline std::span<const cmm::info> parameters_view_of(cmm::info i)
{
    if (!detail::static_valid(i)) return {};
    const auto* function = detail::active_static_registry().try_get_as<detail::Function>(i);
    return function ? function->parameter_ids() : std::span<const cmm::info>{};
}

inline std::vector<cmm::info> parameters_of(cmm::info i)
{
    const auto view = parameters_view_of(i);
    return {view.begin(), view.end()};
}

inline cmm::info return_type_of(cmm::info i)
{
    if (!detail::static_valid(i)) return invalid_info;
    const auto* function = detail::active_static_registry().try_get_as<detail::Function>(i);
    return function ? function->return_type_id() : invalid_info;
}

inline cmm::Error reflect_invoke(cmm::info target, std::span<Value> args, Value& out)
{
    if (!detail::static_valid(target)) return cmm::Error::EntityNotFound;
    const auto* function = detail::active_static_registry().try_get_as<detail::Function>(target);
    if (!function) return cmm::Error::NotInvocable;

    if (function->is_member_function() && !function->is_static_function() &&
        !function->is_constructor() && !function->is_destructor())
    {
        if (args.empty()) return cmm::Error::InvalidArgumentCount;
        if (args[0].pointee_type_id() == cmm::invalid_info) return cmm::Error::InvalidArgumentType;
        if (args[0].pointee_is_const() && !function->is_const_member_function())
        {
            return cmm::Error::ConstViolation;
        }

        const void* instance = args[0].object_pointer();
        if (!instance) return cmm::Error::NullValue;

        const void* adjusted = nullptr;
        const cmm::Error adjust = detail::active_static_registry().adjust_instance_pointer(
            args[0].pointee_type_id(), function->parent_id(), instance, adjusted);
        if (adjust != cmm::Error::Success) return adjust;

        return function->invoke(args, out, adjusted);
    }

    return function->invoke(args, out);
}

inline std::size_t size_of(cmm::info i)
{
    if (!detail::static_valid(i)) return 0;
    return detail::visit_static_entity(i, [](const auto& arg) -> std::size_t
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_base_of_v<detail::Type, T>) return arg.size();
        return 0;
    });
}

inline std::size_t alignment_of(cmm::info i)
{
    if (!detail::static_valid(i)) return 0;
    return detail::visit_static_entity(i, [](const auto& arg) -> std::size_t
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_base_of_v<detail::Type, T>) return arg.alignment();
        return 0;
    });
}

inline cmm::Error try_offset_of(cmm::info i, std::size_t& out)
{
    if (!detail::static_valid(i)) return cmm::Error::EntityNotFound;
    const auto* member = detail::active_static_registry().try_get_as<detail::DataMember>(i);
    if (!member) return cmm::Error::TypeMismatch;
    out = static_cast<std::size_t>(member->offset_bytes());
    return cmm::Error::Success;
}

inline std::size_t offset_of(cmm::info i)
{
    std::size_t value = 0;
    (void)try_offset_of(i, value);
    return value;
}

inline std::size_t bit_offset_of(cmm::info i)
{
    if (!detail::static_valid(i)) return 0;
    const auto* member = detail::active_static_registry().try_get_as<detail::DataMember>(i);
    return member ? static_cast<std::size_t>(member->offset_bits()) : 0;
}

inline bool is_function(cmm::info i)
{
    return detail::static_valid(i) &&
           detail::active_static_registry().try_get_as<detail::Function>(i) != nullptr;
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
    return detail::static_valid(i) &&
           detail::active_static_registry().try_get_as<detail::Enumerator>(i) != nullptr;
}

#define CMM_DEFINE_STATIC_TYPE_PREDICATE(name, flag)                  \
    inline bool name(cmm::info i)                                     \
    {                                                                 \
        if (!detail::static_valid(i)) return false;                   \
        return detail::visit_static_entity(i, [](const auto& arg) -> bool \
        {                                                             \
            using T = std::decay_t<decltype(arg)>;                    \
            if constexpr (std::is_base_of_v<detail::Type, T>)        \
            {                                                         \
                return arg.flags().flag;                              \
            }                                                         \
            return false;                                             \
        });                                                           \
    }

CMM_DEFINE_STATIC_TYPE_PREDICATE(is_void_type,             is_void)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_null_pointer_type,     is_null_pointer)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_integral_type,         is_integral)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_floating_point_type,   is_floating_point)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_arithmetic_type,       is_arithmetic)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_fundamental_type,      is_fundamental)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_pointer_type,          is_pointer)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_lvalue_reference_type, is_lvalue_reference)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_rvalue_reference_type, is_rvalue_reference)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_reference_type,        is_reference)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_class_type,            is_class)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_union_type,            is_union)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_enum_type,             is_enum)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_scoped_enum_type,      is_scoped_enum)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_array_type,            is_array)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_function_type,         is_function_type)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_const_type,            is_const)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_volatile_type,         is_volatile)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_signed_type,           is_signed)
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_unsigned_type,         is_unsigned)

#undef CMM_DEFINE_STATIC_TYPE_PREDICATE

namespace lookup {

inline cmm::info get_member(cmm::info class_id, std::string_view name)
{
    if (!detail::static_valid(class_id)) return invalid_info;
    const auto* cls = detail::active_static_registry().try_get_as<detail::Class>(class_id);
    return cls ? cls->get_member_by_name(name) : invalid_info;
}

inline std::string_view enum_to_string(cmm::info enum_type_id, std::int64_t value)
{
    if (!detail::static_valid(enum_type_id)) return {};
    const auto* enumeration = detail::active_static_registry().try_get_as<detail::Enum>(enum_type_id);
    return enumeration ? enumeration->get_name_by_value(value) : std::string_view{};
}

inline bool string_to_enum(cmm::info enum_type_id, std::string_view name, std::int64_t& out_value)
{
    if (!detail::static_valid(enum_type_id)) return false;
    const auto* enumeration = detail::active_static_registry().try_get_as<detail::Enum>(enum_type_id);
    return enumeration ? enumeration->get_value_by_name(name, out_value) : false;
}

} // namespace lookup

} // namespace cmm

#endif // CALLMEMAYBE_STATIC_META_HPP
