#ifndef CALLMEMAYBE_STATIC_LIFECYCLE_HPP
#define CALLMEMAYBE_STATIC_LIFECYCLE_HPP

#include <array>
#include <cstddef>
#include <cstdlib>
#include <meta>
#include <span>
#include <type_traits>
#include <utility>

#include "cmm/static_invoke.hpp"
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

class DynamicObject {
public:
    DynamicObject() = default;

    DynamicObject(Value pointer, cmm::info destructor)
        : pointer_(std::move(pointer)), destructor_(destructor) {}

    DynamicObject(const DynamicObject&) = delete;
    DynamicObject& operator=(const DynamicObject&) = delete;

    DynamicObject(DynamicObject&& other) noexcept
        : pointer_(std::move(other.pointer_)), destructor_(other.destructor_)
    {
        other.destructor_ = cmm::invalid_info;
    }

    DynamicObject& operator=(DynamicObject&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            pointer_ = std::move(other.pointer_);
            destructor_ = other.destructor_;
            other.destructor_ = cmm::invalid_info;
        }
        return *this;
    }

    ~DynamicObject()
    {
        destroy();
    }

    explicit operator bool() const
    {
        return pointer_.has_value();
    }

    template <typename T>
    T* get()
    {
        if (!pointer_.has_value()) return nullptr;
        return pointer_.template get<T*>();
    }

    template <typename T>
    T* release()
    {
        T* result = get<T>();
        pointer_ = Value{};
        destructor_ = cmm::invalid_info;
        return result;
    }

private:
    void destroy() noexcept
    {
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
DynamicObject construct(cmm::info class_id, Args&&... args)
{
    const cmm::info constructor = lookup::get_constructor<Args...>(class_id);
    const cmm::info destructor = lookup::get_destructor(class_id);
    if (constructor == cmm::invalid_info || destructor == cmm::invalid_info) std::abort();

    Value pointer = invoke(constructor, std::forward<Args>(args)...);
    return DynamicObject(std::move(pointer), destructor);
}

} // namespace cmm

#endif // CALLMEMAYBE_STATIC_LIFECYCLE_HPP
