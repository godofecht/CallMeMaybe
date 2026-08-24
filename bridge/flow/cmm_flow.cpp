#include "cmm_flow.h"

#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "cmm/meta.hpp"

namespace {

using ByteSpan = std::span<const std::uint8_t>;

std::mutex object_mutex;
std::unordered_map<std::uint64_t, std::shared_ptr<cmm::Value>> objects;
std::atomic<std::uint64_t> next_object_handle{1};

template <typename T>
bool is_type(cmm::info type_id)
{
    return type_id == cmm::get_id<^^T>();
}

uint32_t width_kind(bool is_signed, std::size_t size)
{
    if (is_signed)
    {
        switch (size)
        {
            case 1: return CMM_FLOW_I8;
            case 2: return CMM_FLOW_I16;
            case 4: return CMM_FLOW_I32;
            case 8: return CMM_FLOW_I64;
            default: return CMM_FLOW_UNSUPPORTED;
        }
    }
    switch (size)
    {
        case 1: return CMM_FLOW_U8;
        case 2: return CMM_FLOW_U16;
        case 4: return CMM_FLOW_U32;
        case 8: return CMM_FLOW_U64;
        default: return CMM_FLOW_UNSUPPORTED;
    }
}

bool is_nested_borrow_target(cmm::info type_id)
{
    return cmm::is_pointer_type(type_id) || cmm::is_reference_type(type_id);
}

bool requires_instance(cmm::info function_id)
{
    return cmm::is_function(function_id) &&
           cmm::parent_of(function_id) != cmm::invalid_info &&
           !cmm::is_static_member(function_id) &&
           !cmm::is_constructor(function_id) &&
           !cmm::is_destructor(function_id);
}

std::uint64_t store_object(cmm::Value&& value)
{
    if (!value.object_pointer() || value.pointee_type_id() == cmm::invalid_info) return 0;
    const std::uint64_t handle = next_object_handle.fetch_add(1, std::memory_order_relaxed);
    auto stored = std::make_shared<cmm::Value>(std::move(value));
    std::lock_guard<std::mutex> lock(object_mutex);
    objects.emplace(handle, std::move(stored));
    return handle;
}

std::shared_ptr<cmm::Value> get_object(std::uint64_t handle)
{
    std::lock_guard<std::mutex> lock(object_mutex);
    const auto it = objects.find(handle);
    return it == objects.end() ? nullptr : it->second;
}

void erase_object(std::uint64_t handle)
{
    std::lock_guard<std::mutex> lock(object_mutex);
    objects.erase(handle);
}

uint32_t kind_of(cmm::info type_id)
{
    if (is_type<void>(type_id)) return CMM_FLOW_VOID;
    if (is_type<bool>(type_id) || is_type<const bool>(type_id)) return CMM_FLOW_BOOL;
    if (cmm::is_enum_type(type_id)) return kind_of(cmm::underlying_type(type_id));
    if (cmm::is_floating_point_type(type_id))
    {
        if (cmm::size_of(type_id) == sizeof(float)) return CMM_FLOW_F32;
        if (cmm::size_of(type_id) == sizeof(double)) return CMM_FLOW_F64;
        return CMM_FLOW_UNSUPPORTED;
    }
    if (is_type<const char*>(type_id)) return CMM_FLOW_STRING;
    if (is_type<ByteSpan>(type_id)) return CMM_FLOW_BYTES;
    if (cmm::is_lvalue_reference_type(type_id))
    {
        const cmm::info target = cmm::underlying_type(type_id);
        if (cmm::is_enum_type(target) || cmm::is_volatile_type(target) || is_nested_borrow_target(target)) return CMM_FLOW_UNSUPPORTED;
        return cmm::is_const_type(target) ? CMM_FLOW_CONST_REF : CMM_FLOW_MUT_REF;
    }
    if (cmm::is_rvalue_reference_type(type_id)) return CMM_FLOW_UNSUPPORTED;
    if (cmm::is_pointer_type(type_id))
    {
        const cmm::info target = cmm::underlying_type(type_id);
        if (cmm::is_enum_type(target) || cmm::is_volatile_type(target) || is_nested_borrow_target(target)) return CMM_FLOW_UNSUPPORTED;
        return cmm::is_const_type(target) ? CMM_FLOW_CONST_POINTER : CMM_FLOW_POINTER;
    }
    if (cmm::is_integral_type(type_id)) return width_kind(cmm::is_signed_type(type_id), cmm::size_of(type_id));
    return CMM_FLOW_UNSUPPORTED;
}

uint32_t pointee_kind(cmm::info type_id)
{
    if (!cmm::is_pointer_type(type_id) && !cmm::is_reference_type(type_id)) return CMM_FLOW_UNSUPPORTED;
    const cmm::info target = cmm::underlying_type(type_id);
    if (cmm::is_enum_type(target) || cmm::is_volatile_type(target) || is_nested_borrow_target(target)) return CMM_FLOW_UNSUPPORTED;
    return kind_of(target);
}

template <typename T>
cmm::Error decode_integer(const cmm_flow_value& input, uint32_t expected_kind, cmm::Value& output)
{
    if (input.kind != expected_kind) return cmm::Error::InvalidArgumentType;
    output = cmm::Value(static_cast<T>(input.bits));
    return cmm::Error::Success;
}

cmm::Error decode_enum_underlying(cmm::info enum_type, const cmm_flow_value& input, cmm::Value& output)
{
    const cmm::info underlying = cmm::underlying_type(enum_type);
    if (is_type<signed char>(underlying)) return decode_integer<signed char>(input, CMM_FLOW_I8, output);
    if (is_type<unsigned char>(underlying)) return decode_integer<unsigned char>(input, CMM_FLOW_U8, output);
    if (is_type<short>(underlying)) return decode_integer<short>(input, width_kind(true, sizeof(short)), output);
    if (is_type<unsigned short>(underlying)) return decode_integer<unsigned short>(input, width_kind(false, sizeof(unsigned short)), output);
    if (is_type<int>(underlying)) return decode_integer<int>(input, width_kind(true, sizeof(int)), output);
    if (is_type<unsigned int>(underlying)) return decode_integer<unsigned int>(input, width_kind(false, sizeof(unsigned int)), output);
    if (is_type<long>(underlying)) return decode_integer<long>(input, width_kind(true, sizeof(long)), output);
    if (is_type<unsigned long>(underlying)) return decode_integer<unsigned long>(input, width_kind(false, sizeof(unsigned long)), output);
    if (is_type<long long>(underlying)) return decode_integer<long long>(input, width_kind(true, sizeof(long long)), output);
    if (is_type<unsigned long long>(underlying)) return decode_integer<unsigned long long>(input, width_kind(false, sizeof(unsigned long long)), output);
    return cmm::Error::InvalidArgumentType;
}

template <typename T>
cmm::Error decode_scalar_borrow(cmm::info expected_type, const cmm_flow_value& input, cmm::Value& output)
{
    const auto address = static_cast<std::uintptr_t>(input.bits);
    if (cmm::is_pointer_type(expected_type))
    {
        const cmm::info target = cmm::underlying_type(expected_type);
        if (cmm::is_const_type(target)) output = cmm::Value(reinterpret_cast<const T*>(address));
        else output = cmm::Value(reinterpret_cast<T*>(address));
        return cmm::Error::Success;
    }
    if (!cmm::is_lvalue_reference_type(expected_type)) return cmm::Error::InvalidArgumentType;
    if (address == 0) return cmm::Error::NullValue;

    T* value = reinterpret_cast<T*>(address);
    const cmm::info target = cmm::underlying_type(expected_type);
    if (cmm::is_const_type(target)) output = cmm::Value::cref(*value);
    else output = cmm::Value::ref(*value);
    return cmm::Error::Success;
}

cmm::Error decode_borrow(cmm::info expected_type, const cmm_flow_value& input, cmm::Value& output)
{
    switch (pointee_kind(expected_type))
    {
        case CMM_FLOW_BOOL: return decode_scalar_borrow<bool>(expected_type, input, output);
        case CMM_FLOW_I8: return decode_scalar_borrow<std::int8_t>(expected_type, input, output);
        case CMM_FLOW_U8: return decode_scalar_borrow<std::uint8_t>(expected_type, input, output);
        case CMM_FLOW_I16: return decode_scalar_borrow<std::int16_t>(expected_type, input, output);
        case CMM_FLOW_U16: return decode_scalar_borrow<std::uint16_t>(expected_type, input, output);
        case CMM_FLOW_I32: return decode_scalar_borrow<std::int32_t>(expected_type, input, output);
        case CMM_FLOW_U32: return decode_scalar_borrow<std::uint32_t>(expected_type, input, output);
        case CMM_FLOW_I64: return decode_scalar_borrow<std::int64_t>(expected_type, input, output);
        case CMM_FLOW_U64: return decode_scalar_borrow<std::uint64_t>(expected_type, input, output);
        case CMM_FLOW_F32: return decode_scalar_borrow<float>(expected_type, input, output);
        case CMM_FLOW_F64: return decode_scalar_borrow<double>(expected_type, input, output);
        default: return cmm::Error::InvalidArgumentType;
    }
}

cmm::Error decode_value(cmm::info expected_type, const cmm_flow_value& input, cmm::Value& output)
{
    const uint32_t expected_kind = kind_of(expected_type);
    if (input.kind != expected_kind) return cmm::Error::InvalidArgumentType;

    if (cmm::is_enum_type(expected_type)) return decode_enum_underlying(expected_type, input, output);
    if (is_type<bool>(expected_type)) { output = cmm::Value(input.bits != 0); return cmm::Error::Success; }
    if (is_type<signed char>(expected_type)) return decode_integer<signed char>(input, CMM_FLOW_I8, output);
    if (is_type<unsigned char>(expected_type)) return decode_integer<unsigned char>(input, CMM_FLOW_U8, output);
    if (is_type<short>(expected_type)) return decode_integer<short>(input, width_kind(true, sizeof(short)), output);
    if (is_type<unsigned short>(expected_type)) return decode_integer<unsigned short>(input, width_kind(false, sizeof(unsigned short)), output);
    if (is_type<int>(expected_type)) return decode_integer<int>(input, width_kind(true, sizeof(int)), output);
    if (is_type<unsigned int>(expected_type)) return decode_integer<unsigned int>(input, width_kind(false, sizeof(unsigned int)), output);
    if (is_type<long>(expected_type)) return decode_integer<long>(input, width_kind(true, sizeof(long)), output);
    if (is_type<unsigned long>(expected_type)) return decode_integer<unsigned long>(input, width_kind(false, sizeof(unsigned long)), output);
    if (is_type<long long>(expected_type)) return decode_integer<long long>(input, width_kind(true, sizeof(long long)), output);
    if (is_type<unsigned long long>(expected_type)) return decode_integer<unsigned long long>(input, width_kind(false, sizeof(unsigned long long)), output);
    if (is_type<float>(expected_type)) { output = cmm::Value(std::bit_cast<float>(static_cast<uint32_t>(input.bits))); return cmm::Error::Success; }
    if (is_type<double>(expected_type)) { output = cmm::Value(std::bit_cast<double>(input.bits)); return cmm::Error::Success; }
    if (is_type<const char*>(expected_type)) { output = cmm::Value(cmm_flow_bits_string(input.bits)); return cmm::Error::Success; }
    if (is_type<ByteSpan>(expected_type))
    {
        const auto* data = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(input.bits));
        output = cmm::Value(ByteSpan(data, static_cast<std::size_t>(input.extra)));
        return cmm::Error::Success;
    }
    if (is_type<void*>(expected_type))
    {
        output = cmm::Value(reinterpret_cast<void*>(static_cast<std::uintptr_t>(input.bits)));
        return cmm::Error::Success;
    }
    if (cmm::is_pointer_type(expected_type) || cmm::is_lvalue_reference_type(expected_type)) return decode_borrow(expected_type, input, output);
    return cmm::Error::InvalidArgumentType;
}

template <typename T>
void encode_integer(const cmm::Value& input, uint32_t kind, cmm_flow_value& output)
{
    output.kind = kind;
    output.reserved = 0;
    output.bits = static_cast<uint64_t>(input.get<T>());
    output.extra = 0;
}

cmm::Error encode_value(cmm::info return_type, const cmm::Value& input, cmm_flow_value& output)
{
    output = cmm_flow_value{CMM_FLOW_VOID, 0, 0, 0};
    if (is_type<void>(return_type)) return cmm::Error::Success;
    if (cmm::is_enum_type(return_type))
    {
        const std::size_t size = cmm::size_of(return_type);
        if (size == 0 || size > sizeof(output.bits) || !input.data()) return cmm::Error::TypeMismatch;
        output.kind = kind_of(return_type);
        std::memcpy(&output.bits, input.data(), size);
        return output.kind == CMM_FLOW_UNSUPPORTED ? cmm::Error::TypeMismatch : cmm::Error::Success;
    }
    if (is_type<bool>(return_type)) { output.kind = CMM_FLOW_BOOL; output.bits = input.get<bool>() ? 1 : 0; return cmm::Error::Success; }
    if (is_type<signed char>(return_type)) { encode_integer<signed char>(input, CMM_FLOW_I8, output); return cmm::Error::Success; }
    if (is_type<unsigned char>(return_type)) { encode_integer<unsigned char>(input, CMM_FLOW_U8, output); return cmm::Error::Success; }
    if (is_type<short>(return_type)) { encode_integer<short>(input, width_kind(true, sizeof(short)), output); return cmm::Error::Success; }
    if (is_type<unsigned short>(return_type)) { encode_integer<unsigned short>(input, width_kind(false, sizeof(unsigned short)), output); return cmm::Error::Success; }
    if (is_type<int>(return_type)) { encode_integer<int>(input, width_kind(true, sizeof(int)), output); return cmm::Error::Success; }
    if (is_type<unsigned int>(return_type)) { encode_integer<unsigned int>(input, width_kind(false, sizeof(unsigned int)), output); return cmm::Error::Success; }
    if (is_type<long>(return_type)) { encode_integer<long>(input, width_kind(true, sizeof(long)), output); return cmm::Error::Success; }
    if (is_type<unsigned long>(return_type)) { encode_integer<unsigned long>(input, width_kind(false, sizeof(unsigned long)), output); return cmm::Error::Success; }
    if (is_type<long long>(return_type)) { encode_integer<long long>(input, width_kind(true, sizeof(long long)), output); return cmm::Error::Success; }
    if (is_type<unsigned long long>(return_type)) { encode_integer<unsigned long long>(input, width_kind(false, sizeof(unsigned long long)), output); return cmm::Error::Success; }
    if (is_type<float>(return_type)) { output.kind = CMM_FLOW_F32; output.bits = std::bit_cast<uint32_t>(input.get<float>()); return cmm::Error::Success; }
    if (is_type<double>(return_type)) { output.kind = CMM_FLOW_F64; output.bits = std::bit_cast<uint64_t>(input.get<double>()); return cmm::Error::Success; }
    if (is_type<const char*>(return_type)) { output.kind = CMM_FLOW_STRING; output.bits = cmm_flow_string_bits(input.get<const char*>()); return cmm::Error::Success; }
    if (is_type<ByteSpan>(return_type))
    {
        const ByteSpan value = input.get<ByteSpan>();
        output.kind = CMM_FLOW_BYTES;
        output.bits = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(value.data()));
        output.extra = static_cast<uint64_t>(value.size());
        return cmm::Error::Success;
    }
    if (cmm::is_lvalue_reference_type(return_type))
    {
        output.kind = cmm::is_const_type(cmm::underlying_type(return_type)) ? CMM_FLOW_CONST_REF : CMM_FLOW_MUT_REF;
        output.bits = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(input.data()));
        return cmm::Error::Success;
    }
    if (cmm::is_pointer_type(return_type))
    {
        const cmm::info target = cmm::underlying_type(return_type);
        if (cmm::is_volatile_type(target) || is_nested_borrow_target(target)) return cmm::Error::TypeMismatch;
        output.kind = cmm::is_const_type(target) ? CMM_FLOW_CONST_POINTER : CMM_FLOW_POINTER;
        output.bits = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(input.object_pointer()));
        return cmm::Error::Success;
    }
    return cmm::Error::TypeMismatch;
}

cmm::Error decode_arguments(cmm::info function_id,
                            const cmm_flow_value* arguments,
                            uint64_t argument_count,
                            std::vector<cmm::Value>& values)
{
    const auto parameters = cmm::parameters_view_of(function_id);
    if (parameters.size() != argument_count) return cmm::Error::InvalidArgumentCount;
    if (argument_count != 0 && !arguments) return cmm::Error::NullValue;

    values.reserve(values.size() + parameters.size());
    for (std::size_t i = 0; i < parameters.size(); ++i)
    {
        cmm::Value value;
        const cmm::Error decoded = decode_value(cmm::type_of(parameters[i]), arguments[i], value);
        if (decoded != cmm::Error::Success) return decoded;
        values.push_back(std::move(value));
    }
    return cmm::Error::Success;
}

} // namespace

extern "C" cmm_flow_info cmm_flow_reflect_name(const char* name)
{
    if (!name) return cmm::invalid_info;
    return cmm::reflect_name(name);
}

extern "C" uint64_t cmm_flow_parameter_count(cmm_flow_info function_id)
{
    return static_cast<uint64_t>(cmm::parameters_view_of(function_id).size());
}

extern "C" uint32_t cmm_flow_parameter_kind(cmm_flow_info function_id, uint64_t index)
{
    const auto parameters = cmm::parameters_view_of(function_id);
    if (index >= parameters.size()) return CMM_FLOW_UNSUPPORTED;
    return kind_of(cmm::type_of(parameters[static_cast<std::size_t>(index)]));
}

extern "C" uint32_t cmm_flow_parameter_pointee_kind(cmm_flow_info function_id, uint64_t index)
{
    const auto parameters = cmm::parameters_view_of(function_id);
    if (index >= parameters.size()) return CMM_FLOW_UNSUPPORTED;
    return pointee_kind(cmm::type_of(parameters[static_cast<std::size_t>(index)]));
}

extern "C" uint32_t cmm_flow_return_kind(cmm_flow_info function_id)
{
    if (cmm::is_constructor(function_id)) return CMM_FLOW_OBJECT;
    if (cmm::is_destructor(function_id)) return CMM_FLOW_VOID;
    return kind_of(cmm::return_type_of(function_id));
}

extern "C" uint32_t cmm_flow_return_pointee_kind(cmm_flow_info function_id)
{
    if (cmm::is_constructor(function_id) || cmm::is_destructor(function_id)) return CMM_FLOW_UNSUPPORTED;
    return pointee_kind(cmm::return_type_of(function_id));
}

extern "C" bool cmm_flow_requires_instance(cmm_flow_info function_id)
{
    return requires_instance(function_id);
}

extern "C" bool cmm_flow_is_constructor(cmm_flow_info function_id)
{
    return cmm::is_constructor(function_id);
}

extern "C" bool cmm_flow_is_destructor(cmm_flow_info function_id)
{
    return cmm::is_destructor(function_id);
}

extern "C" int32_t cmm_flow_invoke(cmm_flow_info function_id, const cmm_flow_value* arguments, uint64_t argument_count, cmm_flow_value* result)
{
    if (!result) return static_cast<int32_t>(cmm::Error::NullValue);
    if (requires_instance(function_id) || cmm::is_destructor(function_id)) return static_cast<int32_t>(cmm::Error::InvalidArgumentCount);

    std::vector<cmm::Value> values;
    const cmm::Error decoded = decode_arguments(function_id, arguments, argument_count, values);
    if (decoded != cmm::Error::Success) return static_cast<int32_t>(decoded);

    cmm::Value reflected_result;
    const cmm::Error invoked = cmm::reflect_invoke(function_id, values, reflected_result);
    if (invoked != cmm::Error::Success) return static_cast<int32_t>(invoked);

    if (cmm::is_constructor(function_id))
    {
        const std::uint64_t handle = store_object(std::move(reflected_result));
        if (handle == 0) return static_cast<int32_t>(cmm::Error::TypeMismatch);
        *result = cmm_flow_value{CMM_FLOW_OBJECT, 0, handle, cmm::parent_of(function_id)};
        return static_cast<int32_t>(cmm::Error::Success);
    }

    return static_cast<int32_t>(encode_value(cmm::return_type_of(function_id), reflected_result, *result));
}

extern "C" int32_t cmm_flow_invoke_method(cmm_flow_info function_id,
                                            uint64_t object_handle,
                                            const cmm_flow_value* arguments,
                                            uint64_t argument_count,
                                            cmm_flow_value* result)
{
    if (!result) return static_cast<int32_t>(cmm::Error::NullValue);
    if (!requires_instance(function_id) && !cmm::is_destructor(function_id)) return static_cast<int32_t>(cmm::Error::NotInvocable);

    std::shared_ptr<cmm::Value> object = get_object(object_handle);
    if (!object) return static_cast<int32_t>(cmm::Error::NullValue);
    if (object->pointee_type_id() != cmm::parent_of(function_id)) return static_cast<int32_t>(cmm::Error::InvalidArgumentType);

    std::vector<cmm::Value> values;
    values.reserve(static_cast<std::size_t>(argument_count) + 1);
    values.push_back(*object);
    const cmm::Error decoded = decode_arguments(function_id, arguments, argument_count, values);
    if (decoded != cmm::Error::Success) return static_cast<int32_t>(decoded);

    cmm::Value reflected_result;
    const cmm::Error invoked = cmm::reflect_invoke(function_id, values, reflected_result);
    if (invoked != cmm::Error::Success) return static_cast<int32_t>(invoked);

    if (cmm::is_destructor(function_id))
    {
        erase_object(object_handle);
        *result = cmm_flow_value{CMM_FLOW_VOID, 0, 0, 0};
        return static_cast<int32_t>(cmm::Error::Success);
    }

    return static_cast<int32_t>(encode_value(cmm::return_type_of(function_id), reflected_result, *result));
}

extern "C" const char* cmm_flow_error_string(int32_t error)
{
    if (error < static_cast<int32_t>(cmm::Error::Success) || error > static_cast<int32_t>(cmm::Error::NonCopyableValue)) return "Unknown cmm::Error";
    return cmm::to_string(static_cast<cmm::Error>(error));
}
