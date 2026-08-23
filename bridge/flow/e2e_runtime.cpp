#include "e2e_fixture.hpp"
#include "generator.hpp"

#include <cstdint>

extern "C" std::int32_t cmm_flow_e2e_register()
{
    return static_cast<std::int32_t>(
        cmm::flow::register_bindings<^^cmm_e2e_add, ^^cmm_e2e_scale, ^^cmm_e2e_not, ^^cmm_e2e_echo, ^^cmm_e2e_byte_sum, ^^cmm_e2e_echo_bytes>());
}
