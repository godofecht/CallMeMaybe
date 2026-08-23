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
