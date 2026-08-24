#ifndef CMM_FLOW_E2E_FIXTURE_HPP
#define CMM_FLOW_E2E_FIXTURE_HPP

#include <array>
#include <cstdint>
#include <span>

enum class CmmE2eCode : std::uint64_t {
    Low = 7,
    High = UINT64_C(0x8000000000000000)
};

class CmmE2eCounter {
public:
    [[=cmm::reflectable]] explicit CmmE2eCounter(int value)
        : value_(value)
    {
    }

    [[=cmm::reflectable]] int add(int amount)
    {
        value_ += amount;
        return value_;
    }

    [[=cmm::reflectable]] int value() const
    {
        return value_;
    }

private:
    int value_{};
};

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

inline int cmm_e2e_byte_sum(std::span<const std::uint8_t> bytes)
{
    int sum = 0;
    for (std::uint8_t byte : bytes) sum += byte;
    return sum;
}

inline std::span<const std::uint8_t> cmm_e2e_bytes()
{
    static constexpr std::array<std::uint8_t, 4> bytes{9, 8, 7, 6};
    return bytes;
}

inline int* cmm_e2e_pointer(int* value)
{
    return value;
}

inline int& cmm_e2e_mut_ref(int& value)
{
    value += 5;
    return value;
}

inline const int& cmm_e2e_const_ref(const int& value)
{
    return value;
}

inline double& cmm_e2e_mut_ref_f64(double& value)
{
    value *= 2.0;
    return value;
}

inline const bool& cmm_e2e_const_ref_bool(const bool& value)
{
    return value;
}

inline std::uint64_t* cmm_e2e_pointer_u64(std::uint64_t* value)
{
    return value;
}

inline std::uint64_t& cmm_e2e_mut_ref_u64(std::uint64_t& value)
{
    value += 7;
    return value;
}

inline CmmE2eCode cmm_e2e_enum_flip(CmmE2eCode value)
{
    return value == CmmE2eCode::Low ? CmmE2eCode::High : CmmE2eCode::Low;
}

#endif
