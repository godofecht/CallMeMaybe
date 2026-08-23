#ifndef CALLMEMAYBE_META_HPP
#define CALLMEMAYBE_META_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <meta>
#include "cmm/error.hpp"
#include "cmm/info.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/registry.hpp"
#include "cmm/value.hpp"

namespace cmm {

template <std::meta::info EntityRefl>
consteval cmm::info get_id() {
    return detail::hash_entity(EntityRefl);
}

template <std::meta::info EntityRefl>
inline cmm::Error register_rrefl() {
    return detail::Registry::instance().register_entity<EntityRefl>();
}

inline cmm::info reflect_name(std::string_view name) {
    return detail::Registry::instance().get_id_by_name(name);
}

namespace detail {
    inline bool valid(cmm::info i) {
        return i != invalid_info && Registry::instance().contains(i);
    }

    template <typename Visitor>
    inline auto visit_entity(cmm::info i, Visitor&& v) {
        return std::visit(std::forward<Visitor>(v), Registry::instance().get_entity(i));
    }

    template <typename T>
    Value make_argument_value(T&& value) {
        using Raw = std::remove_reference_t<T>;
        if constexpr (std::is_lvalue_reference_v<T&&>) {
            if constexpr (std::is_const_v<Raw>) return Value::cref(value);
            return Value::ref(value);
        }
        return Value(std::forward<T>(value));
    }
} // namespace detail

inline std::string_view identifier_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    return detail::Registry::instance().get_entity_name(i);
}

inline std::string_view display_string_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    return detail::Registry::instance().get_entity_display_name(i);
}

inline cmm::info type_of(cmm::info i) {
    if (!detail::valid(i)) return invalid_info;
    return detail::visit_entity(i, [i](auto&& arg) -> cmm::info {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, detail::DataMember> ||
                      std::is_same_v<T, detail::Parameter> ||
                      std::is_same_v<T, detail::Variable> ||
                      std::is_same_v<T, detail::Base>) {
            return arg.type_id();
        } else if constexpr (std::is_same_v<T, detail::Enumerator>) {
            return arg.parent_id();
        } else if constexpr (std::is_base_of_v<detail::Type, T>) {
            return i;
        }
        return invalid_info;
    });
}

inline cmm::info parent_of(cmm::info i) {
    if (!detail::valid(i)) return invalid_info;
    return detail::visit_entity(i, [](auto&& arg) -> cmm::info {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, detail::DataMember> ||
                      std::is_same_v<T, detail::Function> ||
                      std::is_same_v<T, detail::Enumerator> ||
                      std::is_same_v<T, detail::Parameter> ||
                      std::is_same_v<T, detail::Base>) {
            return arg.parent_id();
        }
        return invalid_info;
    });
}

inline cmm::info underlying_type(cmm::info i) {
    if (!detail::valid(i)) return invalid_info;
    return detail::visit_entity(i, [](auto&& arg) -> cmm::info {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_base_of_v<detail::Type, T>) return arg.underlying_type_id();
        return invalid_info;
    });
}

inline std::span<const cmm::info> members_view_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* cls = std::get_if<detail::Class>(&entity)) return cls->members();
    return {};
}

inline std::vector<cmm::info> members_of(cmm::info i) {
    auto view = members_view_of(i);
    return {view.begin(), view.end()};
}

inline std::span<const cmm::info> nonstatic_data_members_view_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* cls = std::get_if<detail::Class>(&entity)) return cls->nonstatic_data_members();
    return {};
}

inline std::vector<cmm::info> nonstatic_data_members_of(cmm::info i) {
    auto view = nonstatic_data_members_view_of(i);
    return {view.begin(), view.end()};
}

inline std::span<const cmm::info> static_data_members_view_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* cls = std::get_if<detail::Class>(&entity)) return cls->static_data_members();
    return {};
}

inline std::vector<cmm::info> static_data_members_of(cmm::info i) {
    auto view = static_data_members_view_of(i);
    return {view.begin(), view.end()};
}

inline std::span<const cmm::info> bases_view_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* cls = std::get_if<detail::Class>(&entity)) return cls->bases();
    return {};
}

inline std::vector<cmm::info> bases_of(cmm::info i) {
    auto view = bases_view_of(i);
    return {view.begin(), view.end()};
}

inline bool is_base(cmm::info i) {
    if (!detail::valid(i)) return false;
    return std::holds_alternative<detail::Base>(detail::Registry::instance().get_entity(i));
}

inline bool is_virtual_base(cmm::info i) {
    if (!is_base(i)) return false;
    return std::get<detail::Base>(detail::Registry::instance().get_entity(i)).is_virtual();
}

inline bool is_public_base(cmm::info i) {
    if (!is_base(i)) return false;
    return std::get<detail::Base>(detail::Registry::instance().get_entity(i)).access() == detail::Access::Public;
}

inline bool is_protected_base(cmm::info i) {
    if (!is_base(i)) return false;
    return std::get<detail::Base>(detail::Registry::instance().get_entity(i)).access() == detail::Access::Protected;
}

inline bool is_private_base(cmm::info i) {
    if (!is_base(i)) return false;
    return std::get<detail::Base>(detail::Registry::instance().get_entity(i)).access() == detail::Access::Private;
}

inline std::vector<cmm::info> enumerators_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* e = std::get_if<detail::Enum>(&entity)) {
        std::vector<cmm::info> result;
        result.reserve(e->enumerators().size());
        for (const auto& entry : e->enumerators()) result.push_back(entry.entity_id);
        return result;
    }
    return {};
}

inline const void* const_address_of(cmm::info i) {
    if (!detail::valid(i)) return nullptr;
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* var = std::get_if<detail::Variable>(&entity)) return var->address();
    return nullptr;
}

inline void* address_of(cmm::info i) {
    if (!detail::valid(i)) return nullptr;
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* var = std::get_if<detail::Variable>(&entity)) return var->mutable_address();
    return nullptr;
}

inline cmm::Error try_value_of(cmm::info i, std::int64_t& out) {
    if (!detail::valid(i)) return cmm::Error::EntityNotFound;
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* enumerator = std::get_if<detail::Enumerator>(&entity)) {
        out = enumerator->value();
        return cmm::Error::Success;
    }
    return cmm::Error::TypeMismatch;
}

inline std::int64_t value_of(cmm::info i) {
    std::int64_t value = 0;
    (void)try_value_of(i, value);
    return value;
}

inline std::span<const cmm::info> parameters_view_of(cmm::info i) {
    if (!detail::valid(i)) return {};
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* func = std::get_if<detail::Function>(&entity)) return func->parameter_ids();
    return {};
}

inline std::vector<cmm::info> parameters_of(cmm::info i) {
    auto view = parameters_view_of(i);
    return {view.begin(), view.end()};
}

inline cmm::info return_type_of(cmm::info i) {
    if (!detail::valid(i)) return invalid_info;
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* func = std::get_if<detail::Function>(&entity)) return func->return_type_id();
    return invalid_info;
}

inline std::size_t size_of(cmm::info i) {
    if (!detail::valid(i)) return 0;
    return detail::visit_entity(i, [](auto&& arg) -> std::size_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_base_of_v<detail::Type, T>) return arg.size();
        return 0;
    });
}

inline std::size_t alignment_of(cmm::info i) {
    if (!detail::valid(i)) return 0;
    return detail::visit_entity(i, [](auto&& arg) -> std::size_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_base_of_v<detail::Type, T>) return arg.alignment();
        return 0;
    });
}

inline cmm::Error try_offset_of(cmm::info i, std::size_t& out) {
    if (!detail::valid(i)) return cmm::Error::EntityNotFound;
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* dm = std::get_if<detail::DataMember>(&entity)) {
        out = static_cast<std::size_t>(dm->offset_bytes());
        return cmm::Error::Success;
    }
    return cmm::Error::TypeMismatch;
}

inline std::size_t offset_of(cmm::info i) {
    std::size_t value = 0;
    (void)try_offset_of(i, value);
    return value;
}

inline std::size_t bit_offset_of(cmm::info i) {
    if (!detail::valid(i)) return 0;
    auto& entity = detail::Registry::instance().get_entity(i);
    if (auto* dm = std::get_if<detail::DataMember>(&entity)) {
        return static_cast<std::size_t>(dm->offset_bits());
    }
    return 0;
}

inline bool is_function(cmm::info i) {
    if (!detail::valid(i)) return false;
    return std::holds_alternative<detail::Function>(detail::Registry::instance().get_entity(i));
}

inline bool is_constructor(cmm::info i) {
    if (!is_function(i)) return false;
    return std::get<detail::Function>(detail::Registry::instance().get_entity(i)).is_constructor();
}

inline bool is_destructor(cmm::info i) {
    if (!is_function(i)) return false;
    return std::get<detail::Function>(detail::Registry::instance().get_entity(i)).is_destructor();
}

inline bool is_const_member_function(cmm::info i) {
    if (!is_function(i)) return false;
    return std::get<detail::Function>(detail::Registry::instance().get_entity(i)).is_const_member_function();
}

inline bool is_static_member(cmm::info i) {
    if (!detail::valid(i)) return false;
    return detail::visit_entity(i, [](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, detail::Function>) return arg.is_static_function();
        if constexpr (std::is_same_v<T, detail::DataMember>) return arg.is_static();
        return false;
    });
}

inline bool is_nonstatic_data_member(cmm::info i) {
    if (!detail::valid(i)) return false;
    auto& e = detail::Registry::instance().get_entity(i);
    if (auto* dm = std::get_if<detail::DataMember>(&e)) return !dm->is_static();
    return false;
}

inline bool is_const_data_member(cmm::info i) {
    if (!detail::valid(i)) return false;
    auto& e = detail::Registry::instance().get_entity(i);
    if (auto* dm = std::get_if<detail::DataMember>(&e)) return dm->is_const();
    return false;
}

inline bool is_enumerator(cmm::info i) {
    if (!detail::valid(i)) return false;
    return std::holds_alternative<detail::Enumerator>(detail::Registry::instance().get_entity(i));
}

#define CMM_DEFINE_TYPE_PREDICATE(name, flag)                          \
    inline bool name(cmm::info i) {                                    \
        if (!detail::valid(i)) return false;                           \
        return detail::visit_entity(i, [](auto&& arg) -> bool {        \
            using T = std::decay_t<decltype(arg)>;                     \
            if constexpr (std::is_base_of_v<detail::Type, T>) {        \
                return arg.flags().flag;                               \
            }                                                          \
            return false;                                              \
        });                                                            \
    }

CMM_DEFINE_TYPE_PREDICATE(is_void_type,             is_void)
CMM_DEFINE_TYPE_PREDICATE(is_null_pointer_type,     is_null_pointer)
CMM_DEFINE_TYPE_PREDICATE(is_integral_type,         is_integral)
CMM_DEFINE_TYPE_PREDICATE(is_floating_point_type,   is_floating_point)
CMM_DEFINE_TYPE_PREDICATE(is_arithmetic_type,       is_arithmetic)
CMM_DEFINE_TYPE_PREDICATE(is_fundamental_type,      is_fundamental)
CMM_DEFINE_TYPE_PREDICATE(is_pointer_type,          is_pointer)
CMM_DEFINE_TYPE_PREDICATE(is_lvalue_reference_type, is_lvalue_reference)
CMM_DEFINE_TYPE_PREDICATE(is_rvalue_reference_type, is_rvalue_reference)
CMM_DEFINE_TYPE_PREDICATE(is_reference_type,        is_reference)
CMM_DEFINE_TYPE_PREDICATE(is_class_type,            is_class)
CMM_DEFINE_TYPE_PREDICATE(is_union_type,            is_union)
CMM_DEFINE_TYPE_PREDICATE(is_enum_type,             is_enum)
CMM_DEFINE_TYPE_PREDICATE(is_scoped_enum_type,      is_scoped_enum)
CMM_DEFINE_TYPE_PREDICATE(is_array_type,            is_array)
CMM_DEFINE_TYPE_PREDICATE(is_function_type,         is_function_type)
CMM_DEFINE_TYPE_PREDICATE(is_const_type,            is_const)
CMM_DEFINE_TYPE_PREDICATE(is_volatile_type,         is_volatile)
CMM_DEFINE_TYPE_PREDICATE(is_signed_type,           is_signed)
CMM_DEFINE_TYPE_PREDICATE(is_unsigned_type,         is_unsigned)

#undef CMM_DEFINE_TYPE_PREDICATE

inline cmm::Error reflect_invoke(cmm::info target, std::span<Value> args, Value& out) {
    if (!detail::valid(target)) return cmm::Error::EntityNotFound;

    auto& entity = detail::Registry::instance().get_entity(target);
    auto* func = std::get_if<detail::Function>(&entity);
    if (!func) return cmm::Error::NotInvocable;

    if (func->is_member_function() && !func->is_static_function() &&
        !func->is_constructor() && !func->is_destructor()) {
        if (args.empty()) return cmm::Error::InvalidArgumentCount;
        if (args[0].pointee_type_id() == cmm::invalid_info) return cmm::Error::InvalidArgumentType;
        if (args[0].pointee_is_const() && !func->is_const_member_function()) {
            return cmm::Error::ConstViolation;
        }

        const void* instance = args[0].object_pointer();
        if (!instance) return cmm::Error::NullValue;

        const void* adjusted = nullptr;
        cmm::Error adjust = detail::Registry::instance().adjust_instance_pointer(
            args[0].pointee_type_id(), func->parent_id(), instance, adjusted);
        if (adjust != cmm::Error::Success) return adjust;

        return func->invoke(args, out, adjusted);
    }

    return func->invoke(args, out);
}

template <typename Ret = Value, typename... Args>
inline decltype(auto) invoke(cmm::info target, Args&&... args) {
    Value result;
    cmm::Error err;

    if constexpr (sizeof...(Args) > 0) {
        std::array<Value, sizeof...(Args)> vals{
            detail::make_argument_value(std::forward<Args>(args))...
        };
        err = reflect_invoke(target, vals, result);
    } else {
        err = reflect_invoke(target, {}, result);
    }

    if (err != cmm::Error::Success) std::abort();

    if constexpr (std::is_same_v<Ret, Value>) {
        return Value(std::move(result));
    } else if constexpr (std::is_void_v<Ret>) {
        return;
    } else if constexpr (std::is_reference_v<Ret>) {
        using Base = std::remove_reference_t<Ret>;
        return static_cast<Ret>(result.template get<Base>());
    } else {
        return Ret(std::move(result.template get<Ret>()));
    }
}

namespace lookup {

inline cmm::info get_member(cmm::info class_id, std::string_view name) {
    if (!detail::valid(class_id)) return invalid_info;
    auto& entity = detail::Registry::instance().get_entity(class_id);
    if (auto* cls = std::get_if<detail::Class>(&entity)) return cls->get_member_by_name(name);
    return invalid_info;
}

template <typename... Args>
inline cmm::info get_constructor(cmm::info class_id) {
    constexpr std::size_t N = sizeof...(Args);
    const cmm::info expected[] = { detail::hash_entity(std::meta::remove_cvref(^^Args))... };

    for (cmm::info m : members_view_of(class_id)) {
        if (!is_constructor(m)) continue;
        auto params = parameters_view_of(m);
        if (params.size() != N) continue;

        bool match = true;
        for (std::size_t i = 0; i < N; ++i) {
            auto param_decayed = detail::visit_entity(params[i], [](auto&& arg) -> cmm::info {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, detail::Parameter>) return arg.decayed_type_id();
                return invalid_info;
            });
            if (param_decayed != expected[i]) {
                match = false;
                break;
            }
        }
        if (match) return m;
    }
    return invalid_info;
}

inline cmm::info get_destructor(cmm::info class_id) {
    if (!detail::valid(class_id)) return invalid_info;
    auto& entity = detail::Registry::instance().get_entity(class_id);
    if (auto* cls = std::get_if<detail::Class>(&entity)) return cls->destructor();
    return invalid_info;
}

inline std::string_view enum_to_string(cmm::info enum_type_id, std::int64_t value) {
    if (!detail::valid(enum_type_id)) return {};
    auto& entity = detail::Registry::instance().get_entity(enum_type_id);
    if (auto* e = std::get_if<detail::Enum>(&entity)) return e->get_name_by_value(value);
    return {};
}

inline bool string_to_enum(cmm::info enum_type_id, std::string_view name, std::int64_t& out_value) {
    if (!detail::valid(enum_type_id)) return false;
    auto& entity = detail::Registry::instance().get_entity(enum_type_id);
    if (auto* e = std::get_if<detail::Enum>(&entity)) return e->get_value_by_name(name, out_value);
    return false;
}

} // namespace lookup

class DynamicObject {
public:
    DynamicObject() = default;

    DynamicObject(Value pointer, cmm::info destructor)
        : pointer_(std::move(pointer)), destructor_(destructor) {}

    DynamicObject(const DynamicObject&) = delete;
    DynamicObject& operator=(const DynamicObject&) = delete;

    DynamicObject(DynamicObject&& other) noexcept
        : pointer_(std::move(other.pointer_)), destructor_(other.destructor_) {
        other.destructor_ = cmm::invalid_info;
    }

    DynamicObject& operator=(DynamicObject&& other) noexcept {
        if (this != &other) {
            destroy();
            pointer_ = std::move(other.pointer_);
            destructor_ = other.destructor_;
            other.destructor_ = cmm::invalid_info;
        }
        return *this;
    }

    ~DynamicObject() { destroy(); }

    explicit operator bool() const { return pointer_.has_value(); }

    template <typename T>
    T* get() {
        if (!pointer_.has_value()) return nullptr;
        return pointer_.template get<T*>();
    }

    template <typename T>
    T* release() {
        T* result = get<T>();
        pointer_ = Value{};
        destructor_ = cmm::invalid_info;
        return result;
    }

private:
    void destroy() noexcept {
        if (!pointer_.has_value() || destructor_ == cmm::invalid_info) return;

        Value ignored;
        std::span<Value> args(&pointer_, 1);
        if (reflect_invoke(destructor_, args, ignored) != cmm::Error::Success) std::abort();
        pointer_ = Value{};
        destructor_ = cmm::invalid_info;
    }

    Value pointer_;
    cmm::info destructor_{cmm::invalid_info};
};

template <typename... Args>
DynamicObject construct(cmm::info class_id, Args&&... args) {
    cmm::info constructor = lookup::get_constructor<Args...>(class_id);
    cmm::info destructor = lookup::get_destructor(class_id);
    if (constructor == cmm::invalid_info || destructor == cmm::invalid_info) std::abort();

    Value pointer = invoke(constructor, std::forward<Args>(args)...);
    return DynamicObject(std::move(pointer), destructor);
}

} // namespace cmm

#endif // CALLMEMAYBE_META_HPP
