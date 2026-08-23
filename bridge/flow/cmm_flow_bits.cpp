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
