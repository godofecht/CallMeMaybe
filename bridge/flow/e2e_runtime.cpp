#include "e2e_fixture.hpp"
#include "generator.hpp"

#include <cstdint>

extern "C" std::int32_t cmm_flow_e2e_register()
{
    return static_cast<std::int32_t>(
        cmm::flow::register_bindings<^^cmm_e2e_add, ^^cmm_e2e_scale, ^^cmm_e2e_not, ^^cmm_e2e_echo, ^^cmm_e2e_byte_sum, ^^cmm_e2e_bytes, ^^cmm_e2e_pointer, ^^cmm_e2e_mut_ref, ^^cmm_e2e_const_ref, ^^cmm_e2e_pointer_u64, ^^cmm_e2e_mut_ref_f64, ^^cmm_e2e_const_ref_bool>());
}
