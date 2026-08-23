#ifndef CMM_FLOW_GENERATOR_HPP
#define CMM_FLOW_GENERATOR_HPP

#include <array>
#include <meta>
#include <span>
#include <string>

#include "cmm/meta.hpp"

namespace cmm::flow {

struct GenerationResult {
    cmm::Error error{cmm::Error::Success};
    std::string source;
};

GenerationResult generate_wrapper_fragment(std::span<const cmm::info> function_ids);

template <std::meta::info... Functions>
GenerationResult generate_wrapper_fragment()
{
    cmm::Error registration_error = cmm::Error::Success;

    auto register_one = [&]<std::meta::info Function>() {
        if (registration_error != cmm::Error::Success) return;
        registration_error = cmm::register_rrefl<Function>();
    };

    (register_one.template operator()<Functions>(), ...);
    if (registration_error != cmm::Error::Success)
    {
        return GenerationResult{registration_error, {}};
    }

    const std::array<cmm::info, sizeof...(Functions)> ids{cmm::get_id<Functions>()...};
    return generate_wrapper_fragment(ids);
}

template <std::meta::info... Functions>
cmm::Error register_bindings()
{
    cmm::Error error = cmm::Error::Success;

    auto register_one = [&]<std::meta::info Function>() {
        if (error != cmm::Error::Success) return;
        error = cmm::register_rrefl<Function>();
    };

    (register_one.template operator()<Functions>(), ...);
    return error;
}

} // namespace cmm::flow

#endif
