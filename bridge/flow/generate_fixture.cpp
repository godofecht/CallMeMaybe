#include "e2e_fixture.hpp"
#include "generator.hpp"

#include <iostream>
#include <vector>

int main()
{
    cmm::Error registration_error =
        cmm::flow::register_bindings<^^cmm_e2e_add, ^^cmm_e2e_scale, ^^cmm_e2e_not, ^^cmm_e2e_echo, ^^cmm_e2e_byte_sum, ^^cmm_e2e_bytes, ^^cmm_e2e_pointer, ^^cmm_e2e_mut_ref, ^^cmm_e2e_const_ref, ^^cmm_e2e_mut_ref_f64, ^^cmm_e2e_const_ref_bool, ^^cmm_e2e_pointer_u64, ^^cmm_e2e_mut_ref_u64, ^^cmm_e2e_enum_flip>();
    if (registration_error == cmm::Error::Success) registration_error = cmm::register_rrefl<^^CmmE2eCounter>();
    if (registration_error != cmm::Error::Success)
    {
        std::cerr << cmm::to_string(registration_error) << '\n';
        return 1;
    }

    const cmm::info class_id = cmm::get_id<^^CmmE2eCounter>();
    const cmm::info ctor_id = cmm::lookup::get_constructor<int>(class_id);
    const cmm::info add_method_id = cmm::lookup::get_member(class_id, "add");
    const cmm::info value_method_id = cmm::lookup::get_member(class_id, "value");
    const cmm::info dtor_id = cmm::lookup::get_destructor(class_id);
    if (ctor_id == cmm::invalid_info || add_method_id == cmm::invalid_info ||
        value_method_id == cmm::invalid_info || dtor_id == cmm::invalid_info)
    {
        std::cerr << "missing reflected class fixture metadata\n";
        return 1;
    }

    const std::vector<cmm::info> ids{
        cmm::get_id<^^cmm_e2e_add>(),
        cmm::get_id<^^cmm_e2e_scale>(),
        cmm::get_id<^^cmm_e2e_not>(),
        cmm::get_id<^^cmm_e2e_echo>(),
        cmm::get_id<^^cmm_e2e_byte_sum>(),
        cmm::get_id<^^cmm_e2e_bytes>(),
        cmm::get_id<^^cmm_e2e_pointer>(),
        cmm::get_id<^^cmm_e2e_mut_ref>(),
        cmm::get_id<^^cmm_e2e_const_ref>(),
        cmm::get_id<^^cmm_e2e_mut_ref_f64>(),
        cmm::get_id<^^cmm_e2e_const_ref_bool>(),
        cmm::get_id<^^cmm_e2e_pointer_u64>(),
        cmm::get_id<^^cmm_e2e_mut_ref_u64>(),
        cmm::get_id<^^cmm_e2e_enum_flip>(),
        ctor_id,
        add_method_id,
        value_method_id,
        dtor_id
    };

    const cmm::flow::GenerationResult generated = cmm::flow::generate_wrapper_fragment(ids);
    if (generated.error != cmm::Error::Success)
    {
        std::cerr << cmm::to_string(generated.error) << '\n';
        return 1;
    }

    const std::string add_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_add>());
    const std::string scale_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_scale>());
    const std::string not_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_not>());
    const std::string echo_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_echo>());
    const std::string byte_sum_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_byte_sum>());
    const std::string bytes_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_bytes>());
    const std::string pointer_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_pointer>());
    const std::string mut_ref_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_mut_ref>());
    const std::string const_ref_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_const_ref>());
    const std::string mut_ref_f64_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_mut_ref_f64>());
    const std::string const_ref_bool_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_const_ref_bool>());
    const std::string pointer_u64_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_pointer_u64>());
    const std::string mut_ref_u64_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_mut_ref_u64>());
    const std::string enum_flip_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_enum_flip>());
    const std::string ctor_name = cmm::flow::wrapper_name(ctor_id);
    const std::string add_method_name = cmm::flow::wrapper_name(add_method_id);
    const std::string value_method_name = cmm::flow::wrapper_name(value_method_id);
    const std::string dtor_name = cmm::flow::wrapper_name(dtor_id);

    std::cout << generated.source;
    std::cout << "extern {\n";
    std::cout << "    function cmm_flow_e2e_register() -> i32\n";
    std::cout << "    function cmm_flow_e2e_destroyed_count() -> i32\n";
    std::cout << "}\n\n";
    std::cout << "function main() -> i32 {\n";
    std::cout << "    let registration_error: i32 = cmm_flow_e2e_register()\n";
    std::cout << "    if registration_error != 0 { return 10 }\n";
    std::cout << "    let add_result = " << add_name << "(20, 22)\n";
    std::cout << "    if add_result.error != 0 or add_result.value != 42 { return 11 }\n";
    std::cout << "    let scale_result = " << scale_name << "(3.5, 2.0)\n";
    std::cout << "    if scale_result.error != 0 or scale_result.value != 7.0 { return 12 }\n";
    std::cout << "    let not_result = " << not_name << "(true)\n";
    std::cout << "    if not_result.error != 0 or not_result.value != false { return 13 }\n";
    std::cout << "    let echo_result = " << echo_name << "(\"call-me-maybe\")\n";
    std::cout << "    if echo_result.error != 0 or echo_result.value != \"call-me-maybe\" { return 14 }\n";
    std::cout << "    let bytes: array<u8, 4> = [1, 2, 3, 4]\n";
    std::cout << "    let byte_sum_result = " << byte_sum_name << "(bytes)\n";
    std::cout << "    if byte_sum_result.error != 0 { return 20 + byte_sum_result.error }\n";
    std::cout << "    if byte_sum_result.value != 10 { return 16 }\n";
    std::cout << "    let bytes_result = " << bytes_name << "()\n";
    std::cout << "    if bytes_result.error != 0 { return 40 + bytes_result.error }\n";
    std::cout << "    if bytes_result.value.len != 4 { return 17 }\n";
    std::cout << "    if bytes_result.value[0] != 9 or bytes_result.value[1] != 8 or bytes_result.value[2] != 7 or bytes_result.value[3] != 6 { return 18 }\n";
    std::cout << "    let mut borrowed: i32 = 37\n";
    std::cout << "    let pointer_result = " << pointer_name << "(&borrowed)\n";
    std::cout << "    if pointer_result.error != 0 or pointer_result.value[0] != 37 { return 19 }\n";
    std::cout << "    let mut_ref_result = " << mut_ref_name << "(&borrowed)\n";
    std::cout << "    if mut_ref_result.error != 0 or borrowed != 42 or mut_ref_result.value[0] != 42 { return 20 }\n";
    std::cout << "    let const_ref_result = " << const_ref_name << "(&borrowed)\n";
    std::cout << "    if const_ref_result.error != 0 or const_ref_result.value[0] != 42 { return 21 }\n";
    std::cout << "    let mut real: f64 = 3.25\n";
    std::cout << "    let real_result = " << mut_ref_f64_name << "(&real)\n";
    std::cout << "    if real_result.error != 0 or real != 6.5 or real_result.value[0] != 6.5 { return 23 }\n";
    std::cout << "    let mut truth: bool = true\n";
    std::cout << "    let truth_result = " << const_ref_bool_name << "(&truth)\n";
    std::cout << "    if truth_result.error != 0 or truth_result.value[0] != true { return 24 }\n";
    std::cout << "    let mut wide: u64 = 5000000000\n";
    std::cout << "    let wide_ptr_result = " << pointer_u64_name << "(&wide)\n";
    std::cout << "    if wide_ptr_result.error != 0 { return 60 + wide_ptr_result.error }\n";
    std::cout << "    if wide_ptr_result.value[0] != 5000000000 { return 27 }\n";
    std::cout << "    let wide_ref_result = " << mut_ref_u64_name << "(&wide)\n";
    std::cout << "    if wide_ref_result.error != 0 { return 80 + wide_ref_result.error }\n";
    std::cout << "    if wide != 5000000007 or wide_ref_result.value[0] != 5000000007 { return 28 }\n";
    std::cout << "    let enum_high = " << enum_flip_name << "(7)\n";
    std::cout << "    if enum_high.error != 0 or enum_high.value != 9223372036854775808 { return 29 }\n";
    std::cout << "    let enum_low = " << enum_flip_name << "(9223372036854775808)\n";
    std::cout << "    if enum_low.error != 0 or enum_low.value != 7 { return 30 }\n";
    std::cout << "    let ctor_result = " << ctor_name << "(10)\n";
    std::cout << "    if ctor_result.error != 0 or ctor_result.value.object_id == 0 { return 31 }\n";
    std::cout << "    let counter = ctor_result.value\n";
    std::cout << "    let method_result = " << add_method_name << "(counter, 5)\n";
    std::cout << "    if method_result.error != 0 or method_result.value != 15 { return 32 }\n";
    std::cout << "    let value_result = " << value_method_name << "(counter)\n";
    std::cout << "    if value_result.error != 0 or value_result.value != 15 { return 33 }\n";
    std::cout << "    let destroy_result = " << dtor_name << "(counter)\n";
    std::cout << "    if destroy_result.error != 0 or cmm_flow_e2e_destroyed_count() != 1 { return 34 }\n";
    std::cout << "    let after_destroy = " << value_method_name << "(counter)\n";
    std::cout << "    if after_destroy.error == 0 { return 35 }\n";
    std::cout << "    let owned_ctor_result = " << ctor_name << "(21)\n";
    std::cout << "    if owned_ctor_result.error != 0 or owned_ctor_result.value.object_id == 0 { return 36 }\n";
    std::cout << "    let owned_counter = owned_ctor_result.value\n";
    std::cout << "    let release_error = cmm_release(owned_counter, " << class_id << ")\n";
    std::cout << "    if release_error != 0 or cmm_flow_e2e_destroyed_count() != 2 { return 37 }\n";
    std::cout << "    let after_release = " << value_method_name << "(owned_counter)\n";
    std::cout << "    if after_release.error == 0 { return 38 }\n";
    std::cout << "    let second_release = cmm_release(owned_counter, " << class_id << ")\n";
    std::cout << "    if second_release == 0 or cmm_flow_e2e_destroyed_count() != 2 { return 39 }\n";
    std::cout << "    return 0\n";
    std::cout << "}\n";
    return 0;
}
