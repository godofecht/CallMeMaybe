#ifndef CALLMEMAYBE_STATIC_INVOKE_HPP
#define CALLMEMAYBE_STATIC_INVOKE_HPP

#include <array>
#include <cstdlib>
#include <type_traits>
#include <utility>

#include "cmm/static_meta.hpp"

namespace cmm {
namespace detail {

template <typename T>
Value make_static_argument_value(T&& value)
{
    using Raw = std::remove_reference_t<T>;
    if constexpr (std::is_lvalue_reference_v<T&&>)
    {
        if constexpr (std::is_const_v<Raw>) return Value::cref(value);
        return Value::ref(value);
    }
    return Value(std::forward<T>(value));
}

} // namespace detail

template <typename Ret = Value, typename... Args>
inline decltype(auto) invoke(cmm::info target, Args&&... args)
{
    Value result;
    cmm::Error err;

    if constexpr (sizeof...(Args) > 0)
    {
        std::array<Value, sizeof...(Args)> vals{
            detail::make_static_argument_value(std::forward<Args>(args))...
        };
        err = reflect_invoke(target, vals, result);
    }
    else
    {
        err = reflect_invoke(target, {}, result);
    }

    if (err != cmm::Error::Success) std::abort();

    if constexpr (std::is_same_v<Ret, Value>)
    {
        return Value(std::move(result));
    }
    else if constexpr (std::is_void_v<Ret>)
    {
        return;
    }
    else if constexpr (std::is_reference_v<Ret>)
    {
        using Base = std::remove_reference_t<Ret>;
        return static_cast<Ret>(result.template get<Base>());
    }
    else
    {
        return Ret(std::move(result.template get<Ret>()));
    }
}

} // namespace cmm

#endif // CALLMEMAYBE_STATIC_INVOKE_HPP
