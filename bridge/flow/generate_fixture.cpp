#include "e2e_fixture.hpp"
#include "generator.hpp"

#include <iostream>

int main()
{
    const cmm::flow::GenerationResult generated =
        cmm::flow::generate_wrapper_fragment<^^cmm_e2e_add, ^^cmm_e2e_scale, ^^cmm_e2e_not, ^^cmm_e2e_echo>();
    if (generated.error != cmm::Error::Success)
    {
        std::cerr << cmm::to_string(generated.error) << '\n';
        return 1;
    }

    const std::string add_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_add>());
    const std::string scale_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_scale>());
    const std::string not_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_not>());
    const std::string echo_name = cmm::flow::wrapper_name(cmm::get_id<^^cmm_e2e_echo>());

    std::cout << generated.source;
    std::cout << "extern {\n";
    std::cout << "    function cmm_flow_e2e_register() -> i32\n";
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
    std::cout << "    return 0\n";
    std::cout << "}\n";
    return 0;
}
