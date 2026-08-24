#include "cmm_flow.h"
#include "e2e_fixture.hpp"
#include "generator.hpp"

#include <iostream>
#include <vector>

int main()
{
    cmm::Error registration_error =
        cmm::flow::register_bindings<^^cmm_e2e_add, ^^cmm_e2e_scale, ^^cmm_e2e_not, ^^cmm_e2e_echo, ^^cmm_e2e_byte_sum, ^^cmm_e2e_bytes, ^^cmm_e2e_pointer, ^^cmm_e2e_mut_ref, ^^cmm_e2e_const_ref, ^^cmm_e2e_mut_ref_f64, ^^cmm_e2e_const_ref_bool, ^^cmm_e2e_pointer_u64, ^^cmm_e2e_mut_ref_u64, ^^cmm_e2e_enum_flip, ^^cmm_e2e_make_point, ^^cmm_e2e_point_score, ^^cmm_e2e_shift_point>();
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
        cmm::get_id<^^cmm_e2e_make_point>(),
        cmm::get_id<^^cmm_e2e_point_score>(),
        cmm::get_id<^^cmm_e2e_shift_point>(),
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

    std::cout
        << "{\n"
        << "  \"fixture_binding_count\": " << ids.size() << ",\n"
        << "  \"generated_wrapper_bytes\": " << generated.source.size() << ",\n"
        << "  \"cmm_flow_value_bytes\": " << sizeof(cmm_flow_value) << ",\n"
        << "  \"cmm_flow_span_u8_bytes\": " << sizeof(cmm_flow_span_u8) << ",\n"
        << "  \"cmm_flow_info_bytes\": " << sizeof(cmm_flow_info) << "\n"
        << "}\n";

    return 0;
}
