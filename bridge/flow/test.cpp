#include "cmm_flow.h"
#include "generator.hpp"

#include <cassert>
#include <cstdint>
#include <string>

#include "cmm/meta.hpp"

int reflected_add(int a, int b)
{
    return a + b;
}

double reflected_scale(double value, double gain)
{
    return value * gain;
}

bool reflected_not(bool value)
{
    return !value;
}

int main()
{
    assert(cmm::register_rrefl<^^reflected_add>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_scale>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_not>() == cmm::Error::Success);

    const cmm::flow::GenerationResult generated =
        cmm::flow::generate_wrapper_fragment<^^reflected_add, ^^reflected_scale, ^^reflected_not>();
    assert(generated.error == cmm::Error::Success);
    assert(generated.source.find("arg0: i32") != std::string::npos);
    assert(generated.source.find("cmm_i32(arg0)") != std::string::npos);
    assert(generated.source.find("arg0: f64") != std::string::npos);
    assert(generated.source.find("cmm_as_f64(result)") != std::string::npos);
    assert(generated.source.find("arg0: bool") != std::string::npos);
    assert(generated.source.find("cmm_as_bool(result)") != std::string::npos);

    const cmm_flow_info add_id = cmm_flow_reflect_name("reflected_add");
    assert(add_id != cmm::invalid_info);
    assert(cmm_flow_parameter_count(add_id) == 2);
    assert(cmm_flow_parameter_kind(add_id, 0) == CMM_FLOW_I32);
    assert(cmm_flow_parameter_kind(add_id, 1) == CMM_FLOW_I32);
    assert(cmm_flow_return_kind(add_id) == CMM_FLOW_I32);

    const cmm_flow_value add_args[] = {
        {CMM_FLOW_I32, 0, 20},
        {CMM_FLOW_I32, 0, 22},
    };
    cmm_flow_value add_result{};
    assert(cmm_flow_invoke(add_id, add_args, 2, &add_result) == 0);
    assert(add_result.kind == CMM_FLOW_I32);
    assert(static_cast<int32_t>(add_result.bits) == 42);

    const cmm_flow_info scale_id = cmm_flow_reflect_name("reflected_scale");
    const cmm_flow_value scale_args[] = {
        {CMM_FLOW_F64, 0, cmm_flow_f64_bits(3.5)},
        {CMM_FLOW_F64, 0, cmm_flow_f64_bits(2.0)},
    };
    cmm_flow_value scale_result{};
    assert(cmm_flow_invoke(scale_id, scale_args, 2, &scale_result) == 0);
    assert(scale_result.kind == CMM_FLOW_F64);
    assert(cmm_flow_bits_f64(scale_result.bits) == 7.0);

    const cmm_flow_info not_id = cmm_flow_reflect_name("reflected_not");
    const cmm_flow_value bool_arg{CMM_FLOW_BOOL, 0, 1};
    cmm_flow_value bool_result{};
    assert(cmm_flow_invoke(not_id, &bool_arg, 1, &bool_result) == 0);
    assert(bool_result.kind == CMM_FLOW_BOOL);
    assert(bool_result.bits == 0);

    cmm_flow_value bad_result{};
    assert(cmm_flow_invoke(add_id, &bool_arg, 1, &bad_result) ==
           static_cast<int32_t>(cmm::Error::InvalidArgumentCount));

    return 0;
}
