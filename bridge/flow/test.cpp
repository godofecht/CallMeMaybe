#include "cmm_flow.h"
#include "generator.hpp"

#include <array>
#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>

#include "cmm/meta.hpp"

int reflected_add(int a, int b) { return a + b; }
double reflected_scale(double value, double gain) { return value * gain; }
bool reflected_not(bool value) { return !value; }
const char* reflected_echo(const char* value) { return value; }

int reflected_byte_sum(std::span<const std::uint8_t> bytes)
{
    int sum = 0;
    for (std::uint8_t byte : bytes) sum += byte;
    return sum;
}

int reflected_read_const(const int* value)
{
    return value ? *value : -1;
}

const int* reflected_const_return(int* value)
{
    return value;
}

int reflected_rvalue(int&& value)
{
    return value;
}

int reflected_nested(int** value)
{
    return value && *value ? **value : -1;
}

int reflected_volatile(const volatile int* value)
{
    return value ? *value : -1;
}

int main()
{
    assert(cmm::register_rrefl<^^reflected_add>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_scale>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_not>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_echo>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_byte_sum>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_read_const>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_const_return>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_rvalue>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_nested>() == cmm::Error::Success);
    assert(cmm::register_rrefl<^^reflected_volatile>() == cmm::Error::Success);

    const cmm::flow::GenerationResult generated =
        cmm::flow::generate_wrapper_fragment<^^reflected_add, ^^reflected_scale, ^^reflected_not, ^^reflected_echo, ^^reflected_byte_sum>();
    assert(generated.error == cmm::Error::Success);
    assert(generated.source.find("arg0: i32") != std::string::npos);
    assert(generated.source.find("cmm_i32(arg0)") != std::string::npos);
    assert(generated.source.find("arg0: f64") != std::string::npos);
    assert(generated.source.find("cmm_as_f64(result)") != std::string::npos);
    assert(generated.source.find("arg0: bool") != std::string::npos);
    assert(generated.source.find("cmm_as_bool(result)") != std::string::npos);
    assert(generated.source.find("arg0: string") != std::string::npos);
    assert(generated.source.find("cmm_string(arg0)") != std::string::npos);
    assert(generated.source.find("cmm_as_string(result)") != std::string::npos);
    assert(generated.source.find("arg0: span<u8>") != std::string::npos);
    assert(generated.source.find("cmm_bytes(arg0)") != std::string::npos);

    const cmm::flow::GenerationResult const_pointer_parameter =
        cmm::flow::generate_wrapper_fragment<^^reflected_read_const>();
    assert(const_pointer_parameter.error == cmm::Error::InvalidArgumentType);

    const cmm::flow::GenerationResult const_pointer_return =
        cmm::flow::generate_wrapper_fragment<^^reflected_const_return>();
    assert(const_pointer_return.error == cmm::Error::TypeMismatch);

    const cmm::flow::GenerationResult rvalue =
        cmm::flow::generate_wrapper_fragment<^^reflected_rvalue>();
    assert(rvalue.error == cmm::Error::InvalidArgumentType);

    const cmm::flow::GenerationResult nested =
        cmm::flow::generate_wrapper_fragment<^^reflected_nested>();
    assert(nested.error == cmm::Error::InvalidArgumentType);

    const cmm::flow::GenerationResult volatile_pointer =
        cmm::flow::generate_wrapper_fragment<^^reflected_volatile>();
    assert(volatile_pointer.error == cmm::Error::InvalidArgumentType);

    const cmm_flow_info add_id = cmm_flow_reflect_name("reflected_add");
    assert(add_id != cmm::invalid_info);
    assert(cmm_flow_parameter_count(add_id) == 2);
    assert(cmm_flow_parameter_kind(add_id, 0) == CMM_FLOW_I32);
    assert(cmm_flow_parameter_kind(add_id, 1) == CMM_FLOW_I32);
    assert(cmm_flow_return_kind(add_id) == CMM_FLOW_I32);

    const cmm_flow_value add_args[] = {{CMM_FLOW_I32, 0, 20, 0}, {CMM_FLOW_I32, 0, 22, 0}};
    cmm_flow_value add_result{};
    assert(cmm_flow_invoke(add_id, add_args, 2, &add_result) == 0);
    assert(add_result.kind == CMM_FLOW_I32);
    assert(static_cast<int32_t>(add_result.bits) == 42);

    const cmm_flow_info scale_id = cmm_flow_reflect_name("reflected_scale");
    const cmm_flow_value scale_args[] = {
        {CMM_FLOW_F64, 0, cmm_flow_f64_bits(3.5), 0},
        {CMM_FLOW_F64, 0, cmm_flow_f64_bits(2.0), 0},
    };
    cmm_flow_value scale_result{};
    assert(cmm_flow_invoke(scale_id, scale_args, 2, &scale_result) == 0);
    assert(scale_result.kind == CMM_FLOW_F64);
    assert(cmm_flow_bits_f64(scale_result.bits) == 7.0);

    const cmm_flow_info not_id = cmm_flow_reflect_name("reflected_not");
    const cmm_flow_value bool_arg{CMM_FLOW_BOOL, 0, 1, 0};
    cmm_flow_value bool_result{};
    assert(cmm_flow_invoke(not_id, &bool_arg, 1, &bool_result) == 0);
    assert(bool_result.kind == CMM_FLOW_BOOL);
    assert(bool_result.bits == 0);

    const char* text = "borrowed-string";
    const cmm_flow_info echo_id = cmm_flow_reflect_name("reflected_echo");
    assert(echo_id != cmm::invalid_info);
    assert(cmm_flow_parameter_kind(echo_id, 0) == CMM_FLOW_STRING);
    assert(cmm_flow_return_kind(echo_id) == CMM_FLOW_STRING);
    const cmm_flow_value string_arg{CMM_FLOW_STRING, 0, cmm_flow_string_bits(text), 0};
    cmm_flow_value string_result{};
    assert(cmm_flow_invoke(echo_id, &string_arg, 1, &string_result) == 0);
    assert(string_result.kind == CMM_FLOW_STRING);
    assert(std::strcmp(cmm_flow_bits_string(string_result.bits), text) == 0);

    const std::array<std::uint8_t, 4> bytes{1, 2, 3, 4};
    const cmm_flow_info byte_sum_id = cmm_flow_reflect_name("reflected_byte_sum");
    assert(byte_sum_id != cmm::invalid_info);
    assert(cmm_flow_parameter_kind(byte_sum_id, 0) == CMM_FLOW_BYTES);
    assert(cmm_flow_return_kind(byte_sum_id) == CMM_FLOW_I32);
    const cmm_flow_value bytes_arg = cmm_flow_bytes({bytes.data(), static_cast<int64_t>(bytes.size())});
    assert(bytes_arg.kind == CMM_FLOW_BYTES);
    assert(bytes_arg.extra == bytes.size());
    cmm_flow_value bytes_result{};
    assert(cmm_flow_invoke(byte_sum_id, &bytes_arg, 1, &bytes_result) == 0);
    assert(bytes_result.kind == CMM_FLOW_I32);
    assert(static_cast<int32_t>(bytes_result.bits) == 10);

    int borrowed = 91;
    const cmm_flow_info read_const_id = cmm_flow_reflect_name("reflected_read_const");
    assert(read_const_id != cmm::invalid_info);
    assert(cmm_flow_parameter_kind(read_const_id, 0) == CMM_FLOW_CONST_POINTER);
    assert(cmm_flow_parameter_pointee_kind(read_const_id, 0) == CMM_FLOW_I32);
    const cmm_flow_value const_pointer_arg{
        CMM_FLOW_CONST_POINTER,
        0,
        static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(&borrowed)),
        0
    };
    cmm_flow_value const_pointer_result{};
    assert(cmm_flow_invoke(read_const_id, &const_pointer_arg, 1, &const_pointer_result) == 0);
    assert(const_pointer_result.kind == CMM_FLOW_I32);
    assert(static_cast<int32_t>(const_pointer_result.bits) == borrowed);

    const cmm_flow_info const_return_id = cmm_flow_reflect_name("reflected_const_return");
    assert(cmm_flow_return_kind(const_return_id) == CMM_FLOW_CONST_POINTER);
    assert(cmm_flow_return_pointee_kind(const_return_id) == CMM_FLOW_I32);

    const cmm_flow_info rvalue_id = cmm_flow_reflect_name("reflected_rvalue");
    assert(cmm_flow_parameter_kind(rvalue_id, 0) == CMM_FLOW_UNSUPPORTED);
    assert(cmm_flow_parameter_pointee_kind(rvalue_id, 0) == CMM_FLOW_I32);

    const cmm_flow_info nested_id = cmm_flow_reflect_name("reflected_nested");
    assert(cmm_flow_parameter_kind(nested_id, 0) == CMM_FLOW_UNSUPPORTED);
    assert(cmm_flow_parameter_pointee_kind(nested_id, 0) == CMM_FLOW_UNSUPPORTED);

    const cmm_flow_info volatile_id = cmm_flow_reflect_name("reflected_volatile");
    assert(cmm_flow_parameter_kind(volatile_id, 0) == CMM_FLOW_UNSUPPORTED);
    assert(cmm_flow_parameter_pointee_kind(volatile_id, 0) == CMM_FLOW_UNSUPPORTED);

    cmm_flow_value bad_result{};
    assert(cmm_flow_invoke(add_id, &bool_arg, 1, &bad_result) == static_cast<int32_t>(cmm::Error::InvalidArgumentCount));
    return 0;
}
