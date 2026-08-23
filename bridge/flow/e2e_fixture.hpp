#ifndef CMM_FLOW_E2E_FIXTURE_HPP
#define CMM_FLOW_E2E_FIXTURE_HPP

#include <cstdint>
#include <span>

inline int cmm_e2e_add(int a, int b)
{
    return a + b;
}

inline double cmm_e2e_scale(double value, double gain)
{
    return value * gain;
}

inline bool cmm_e2e_not(bool value)
{
    return !value;
}

inline const char* cmm_e2e_echo(const char* value)
{
    return value;
}

inline std::uint64_t cmm_e2e_byte_sum(std::span<const std::uint8_t> bytes)
{
    std::uint64_t sum = 0;
    for (std::uint8_t byte : bytes) sum += byte;
    return sum;
}

#endif
