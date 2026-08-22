#ifndef CALLMEMAYBE_THUNK_HPP
#define CALLMEMAYBE_THUNK_HPP

#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include <meta>

#include "cmm/error.hpp"
#include "cmm/info.hpp"
#include "cmm/value.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/hash/info_hash.hpp"

namespace cmm {
namespace detail {

constexpr bool is_compatible_argument(
    cmm::info arg_base_id, cmm::Value::Policy arg_policy,
    cmm::info param_base_id, bool param_is_ref, bool param_is_const,
    bool param_is_rvalue_ref)
{
    if (arg_base_id != param_base_id) return false;
    if (!param_is_ref) return true;
    if (param_is_const) return true;

    if (param_is_rvalue_ref) {
        return arg_policy == cmm::Value::Policy::Owned ||
               arg_policy == cmm::Value::Policy::RvalueRef;
    }
    return arg_policy == cmm::Value::Policy::MutRef;
}

template <std::meta::info ParamTypeRefl>
constexpr bool is_argument_valid(const cmm::Value& arg) {
    constexpr cmm::info p_base_id = cmm::detail::hash_entity(std::meta::decay(ParamTypeRefl));
    constexpr bool p_is_ref = std::meta::is_reference_type(ParamTypeRefl);
    constexpr bool p_is_const = std::meta::is_const_type(std::meta::remove_reference(ParamTypeRefl));
    constexpr bool p_is_rvalue_ref = std::meta::is_rvalue_reference_type(ParamTypeRefl);

    return is_compatible_argument(arg.type_id(), arg.policy(), p_base_id,
                                  p_is_ref, p_is_const, p_is_rvalue_ref);
}

template <std::meta::info ParamTypeRefl>
decltype(auto) extract_argument(cmm::Value& value) {
    using Param = typename[:ParamTypeRefl:];
    using Base = std::remove_cvref_t<Param>;

    const Base* ptr = static_cast<const Base*>(value.data());

    if constexpr (std::is_lvalue_reference_v<Param>) {
        if constexpr (std::is_const_v<std::remove_reference_t<Param>>) {
            return static_cast<Param>(*ptr);
        } else {
            return static_cast<Param>(*const_cast<Base*>(ptr));
        }
    } else if constexpr (std::is_rvalue_reference_v<Param>) {
        return static_cast<Param>(std::move(*const_cast<Base*>(ptr)));
    } else {
        return static_cast<Param>(*ptr);
    }
}

template <typename T>
struct PropertyThunks {
    static Value get(const void* inst, std::ptrdiff_t offset) {
        return Value(*reinterpret_cast<const T*>(static_cast<const char*>(inst) + offset));
    }
    static Value get_ref(void* inst, std::ptrdiff_t offset) {
        return Value::ref(*reinterpret_cast<T*>(static_cast<char*>(inst) + offset));
    }
    static Value get_cref(void* inst, std::ptrdiff_t offset) {
        return Value::cref(*reinterpret_cast<const T*>(static_cast<const char*>(inst) + offset));
    }
    static cmm::Error set(void* inst, std::ptrdiff_t offset, const Value& val) {
        const T* typed = val.get_if<T>();
        if (!typed) return cmm::Error::TypeMismatch;
        *reinterpret_cast<T*>(static_cast<char*>(inst) + offset) = *typed;
        return cmm::Error::Success;
    }
};

template <typename T>
struct StaticThunks {
    static Value get(const void* address) {
        return Value(*reinterpret_cast<const T*>(address));
    }
    static Value get_ref(void* address) {
        return Value::ref(*reinterpret_cast<T*>(address));
    }
    static Value get_cref(void* address) {
        return Value::cref(*reinterpret_cast<const T*>(address));
    }
    static cmm::Error set(void* address, const Value& val) {
        const T* typed = val.get_if<T>();
        if (!typed) return cmm::Error::TypeMismatch;
        *reinterpret_cast<T*>(address) = *typed;
        return cmm::Error::Success;
    }
};

template <std::meta::info FuncRefl>
InvokerFn create_thunk() {
    return [](std::span<Value> args, Value& out) -> cmm::Error {
        static constexpr auto params = std::define_static_array(std::meta::parameters_of(FuncRefl));
        constexpr std::size_t num_params = params.size();
        constexpr bool is_member = std::meta::is_class_member(FuncRefl) &&
                                   !std::meta::is_static_member(FuncRefl);
        constexpr std::size_t arg_offset = is_member ? 1 : 0;

        if (args.size() != num_params + arg_offset) {
            return cmm::Error::InvalidArgumentCount;
        }

        if constexpr (is_member) {
            constexpr cmm::info expected_instance_type = cmm::detail::hash_entity(
                std::meta::add_pointer(std::meta::parent_of(FuncRefl))
            );
            if (args[0].type_id() != expected_instance_type) {
                return cmm::Error::InvalidArgumentType;
            }

            auto* instance_ptr = *static_cast<typename[:std::meta::parent_of(FuncRefl):]* const*>(args[0].data());
            if (!instance_ptr) {
                return cmm::Error::NullValue;
            }
        }

        return []<std::size_t... Is>(std::span<Value> args, Value& out,
                                     std::index_sequence<Is...>) -> cmm::Error {
            bool args_valid = (is_argument_valid<std::meta::type_of(params[Is])>(
                                   args[Is + arg_offset]) && ...);
            if (!args_valid) {
                return cmm::Error::InvalidArgumentType;
            }

            auto do_invoke = [&]() -> decltype(auto) {
                if constexpr (is_member) {
                    using ClassType = typename[:std::meta::parent_of(FuncRefl):];
                    auto* instance_ptr = *static_cast<ClassType* const*>(args[0].data());
                    return std::invoke(&[:FuncRefl:], instance_ptr,
                        extract_argument<std::meta::type_of(params[Is])>(args[Is + 1])...);
                } else {
                    return std::invoke([:FuncRefl:],
                        extract_argument<std::meta::type_of(params[Is])>(args[Is])...);
                }
            };

            using ReturnType = typename[:std::meta::return_type_of(FuncRefl):];
            if constexpr (std::is_void_v<ReturnType>) {
                do_invoke();
                out = Value{};
            } else if constexpr (std::is_lvalue_reference_v<ReturnType>) {
                auto&& result = do_invoke();
                if constexpr (std::is_const_v<std::remove_reference_t<ReturnType>>) {
                    out = Value::cref(result);
                } else {
                    out = Value::ref(result);
                }
            } else if constexpr (std::is_rvalue_reference_v<ReturnType>) {
                auto&& result = do_invoke();
                out = Value::rref(std::move(result));
            } else {
                out = Value(do_invoke());
            }

            return cmm::Error::Success;
        }(args, out, std::make_index_sequence<num_params>{});
    };
}

template <std::meta::info ConstructorRefl>
InvokerFn create_constructor_thunk() {
    return [](std::span<Value> args, Value& out) -> cmm::Error {
        static constexpr auto params = std::define_static_array(std::meta::parameters_of(ConstructorRefl));
        constexpr std::size_t num_params = params.size();

        if (args.size() != num_params) {
            return cmm::Error::InvalidArgumentCount;
        }

        return []<std::size_t... Is>(std::span<Value> args, Value& out,
                                     std::index_sequence<Is...>) -> cmm::Error {
            bool args_valid = (is_argument_valid<std::meta::type_of(params[Is])>(args[Is]) && ...);
            if (!args_valid) {
                return cmm::Error::InvalidArgumentType;
            }

            using ClassType = typename[:std::meta::parent_of(ConstructorRefl):];
            out = Value(new ClassType(
                extract_argument<std::meta::type_of(params[Is])>(args[Is])...));
            return cmm::Error::Success;
        }(args, out, std::make_index_sequence<num_params>{});
    };
}

template <std::meta::info DestructorRefl>
InvokerFn create_destructor_thunk() {
    return [](std::span<Value> args, Value& out) -> cmm::Error {
        if (args.size() != 1) {
            return cmm::Error::InvalidArgumentCount;
        }

        using ClassType = typename[:std::meta::parent_of(DestructorRefl):];
        constexpr cmm::info expected_type = cmm::detail::hash_entity(
            std::meta::add_pointer(std::meta::parent_of(DestructorRefl))
        );
        if (args[0].type_id() != expected_type) {
            return cmm::Error::InvalidArgumentType;
        }

        auto* instance_ptr = *static_cast<ClassType* const*>(args[0].data());
        if (!instance_ptr) {
            return cmm::Error::NullValue;
        }

        delete instance_ptr;
        out = Value{};
        return cmm::Error::Success;
    };
}

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_THUNK_HPP
