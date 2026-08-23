#ifndef CMM_FLOW_E2E_FIXTURE_HPP
#define CMM_FLOW_E2E_FIXTURE_HPP

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

#endif
