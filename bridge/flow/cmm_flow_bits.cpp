#include "cmm_flow.h"

#include <bit>
#include <cstdint>

extern "C" uint64_t cmm_flow_f32_bits(float value)
{
    return static_cast<uint64_t>(std::bit_cast<uint32_t>(value));
}

extern "C" uint64_t cmm_flow_f64_bits(double value)
{
    return std::bit_cast<uint64_t>(value);
}

extern "C" float cmm_flow_bits_f32(uint64_t bits)
{
    return std::bit_cast<float>(static_cast<uint32_t>(bits));
}

extern "C" double cmm_flow_bits_f64(uint64_t bits)
{
    return std::bit_cast<double>(bits);
}

extern "C" uint64_t cmm_flow_string_bits(const char* value)
{
    return static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(value));
}

extern "C" const char* cmm_flow_bits_string(uint64_t bits)
{
    return reinterpret_cast<const char*>(static_cast<std::uintptr_t>(bits));
}

extern "C" cmm_flow_value cmm_flow_bytes(cmm_flow_span_u8 value)
{
    return cmm_flow_value{
        CMM_FLOW_BYTES,
        0,
        static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(value.data)),
        static_cast<uint64_t>(value.len)
    };
}

extern "C" cmm_flow_span_u8 cmm_flow_as_bytes(cmm_flow_value value)
{
    if (value.kind != CMM_FLOW_BYTES) return cmm_flow_span_u8{nullptr, 0};
    return cmm_flow_span_u8{
        reinterpret_cast<const uint8_t*>(static_cast<std::uintptr_t>(value.bits)),
        static_cast<int64_t>(value.extra)
    };
}

extern "C" uint64_t cmm_flow_byte_span_data(cmm_flow_span_u8 value)
{
    return static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(value.data));
}

extern "C" const uint8_t* cmm_flow_bits_u8_ptr(uint64_t bits)
{
    return reinterpret_cast<const uint8_t*>(static_cast<std::uintptr_t>(bits));
}
