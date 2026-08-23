#include "cmm_flow.h"

#include <bit>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

#include "cmm/meta.hpp"

namespace {

using ByteSpan = std::span<const std::uint8_t>;

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

uint32_t kind_of(cmm::info type_id)
{
    if (is_type<void>(type_id)) return CMM_FLOW_VOID;
    if (is_type<bool>(type_id)) return CMM_FLOW_BOOL;
    if (is_type<float>(type_id)) return CMM_FLOW_F32;
    if (is_type<double>(type_id)) return CMM_FLOW_F64;
    if (is_type<const char*>(type_id)) return CMM_FLOW_STRING;
    if (is_type<ByteSpan>(type_id)) return CMM_FLOW_BYTES;
    if (cmm::is_pointer_type(type_id)) return CMM_FLOW_POINTER;
    if (cmm::is_integral_type(type_id))
    {
        return width_kind(cmm::is_signed_type(type_id), cmm::size_of(type_id));
    }
    return CMM_FLOW_UNSUPPORTED;
}

template <typename T>
cmm::Error decode_integer(const cmm_flow_value& input, uint32_t expected_kind, cmm::Value& output)
{
    if (input.kind != expected_kind) return cmm::Error::InvalidArgumentType;
    output = cmm::Value(static_cast<T>(input.bits));
    return cmm::Error::Success;
}

cmm::Error decode_value(cmm::info expected_type, const cmm_flow_value& input, cmm::Value& output)
{
    const uint32_t expected_kind = kind_of(expected_type);
    if (input.kind != expected_kind) return cmm::Error::InvalidArgumentType;

    if (is_type<bool>(expected_type))
    {
        output = cmm::Value(input.bits != 0);
        return cmm::Error::Success;
    }
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

    if (is_type<float>(expected_type))
    {
        output = cmm::Value(std::bit_cast<float>(static_cast<uint32_t>(input.bits)));
        return cmm::Error::Success;
    }
    if (is_type<double>(expected_type))
    {
        output = cmm::Value(std::bit_cast<double>(input.bits));
        return cmm::Error::Success;
    }
    if (is_type<const char*>(expected_type))
    {
        output = cmm::Value(cmm_flow_bits_string(input.bits));
        return cmm::Error::Success;
    }
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
    if (is_type<bool>(return_type))
    {
        output.kind = CMM_FLOW_BOOL;
        output.bits = input.get<bool>() ? 1 : 0;
        return cmm::Error::Success;
    }
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

    if (is_type<float>(return_type))
    {
        output.kind = CMM_FLOW_F32;
        output.bits = std::bit_cast<uint32_t>(input.get<float>());
        return cmm::Error::Success;
    }
    if (is_type<double>(return_type))
    {
        output.kind = CMM_FLOW_F64;
        output.bits = std::bit_cast<uint64_t>(input.get<double>());
        return cmm::Error::Success;
    }
    if (is_type<const char*>(return_type))
    {
        output.kind = CMM_FLOW_STRING;
        output.bits = cmm_flow_string_bits(input.get<const char*>());
        return cmm::Error::Success;
    }
    if (is_type<ByteSpan>(return_type))
    {
        const ByteSpan value = input.get<ByteSpan>();
        output.kind = CMM_FLOW_BYTES;
        output.bits = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(value.data()));
        output.extra = static_cast<uint64_t>(value.size());
        return cmm::Error::Success;
    }
    if (cmm::is_pointer_type(return_type))
    {
        output.kind = CMM_FLOW_POINTER;
        output.bits = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(input.object_pointer()));
        return cmm::Error::Success;
    }

    return cmm::Error::TypeMismatch;
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

extern "C" uint32_t cmm_flow_return_kind(cmm_flow_info function_id)
{
    return kind_of(cmm::return_type_of(function_id));
}

extern "C" int32_t cmm_flow_invoke(
    cmm_flow_info function_id,
    const cmm_flow_value* arguments,
    uint64_t argument_count,
    cmm_flow_value* result)
{
    if (!result) return static_cast<int32_t>(cmm::Error::NullValue);
    if (argument_count != 0 && !arguments) return static_cast<int32_t>(cmm::Error::NullValue);

    const auto parameters = cmm::parameters_view_of(function_id);
    if (parameters.size() != argument_count)
    {
        return static_cast<int32_t>(cmm::Error::InvalidArgumentCount);
    }

    std::vector<cmm::Value> values;
    values.reserve(parameters.size());

    for (std::size_t i = 0; i < parameters.size(); ++i)
    {
        cmm::Value value;
        const cmm::Error decoded = decode_value(cmm::type_of(parameters[i]), arguments[i], value);
        if (decoded != cmm::Error::Success) return static_cast<int32_t>(decoded);
        values.push_back(std::move(value));
    }

    cmm::Value reflected_result;
    const cmm::Error invoked = cmm::reflect_invoke(function_id, values, reflected_result);
    if (invoked != cmm::Error::Success) return static_cast<int32_t>(invoked);

    return static_cast<int32_t>(encode_value(cmm::return_type_of(function_id), reflected_result, *result));
}

extern "C" const char* cmm_flow_error_string(int32_t error)
{
    if (error < static_cast<int32_t>(cmm::Error::Success) ||
        error > static_cast<int32_t>(cmm::Error::NonCopyableValue))
    {
        return "Unknown cmm::Error";
    }
    return cmm::to_string(static_cast<cmm::Error>(error));
}
