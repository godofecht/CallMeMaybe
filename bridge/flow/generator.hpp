#ifndef CMM_FLOW_GENERATOR_HPP
#define CMM_FLOW_GENERATOR_HPP

#include <array>
#include <meta>
#include <span>
#include <string>
#include <vector>

#include "cmm/meta.hpp"

namespace cmm::flow {

struct GenerationResult {
    cmm::Error error{cmm::Error::Success};
    std::string source;
};

std::string wrapper_name(cmm::info function_id);
GenerationResult generate_wrapper_fragment(std::span<const cmm::info> function_ids);

namespace detail {

template <std::meta::info TypeRefl>
void register_enum_dependency(cmm::Error& error)
{
    if (error != cmm::Error::Success) return;
    constexpr std::meta::info decayed = std::meta::remove_cvref(TypeRefl);
    if constexpr (std::meta::is_enum_type(decayed)) error = cmm::register_rrefl<decayed>();
}

template <std::meta::info Function>
void register_binding(cmm::Error& error)
{
    if (error != cmm::Error::Success) return;

    if constexpr (!std::meta::is_constructor(Function) && !std::meta::is_destructor(Function))
    {
        register_enum_dependency<std::meta::return_type_of(Function)>(error);
    }

    template for (constexpr std::meta::info parameter : std::define_static_array(std::meta::parameters_of(Function)))
    {
        register_enum_dependency<std::meta::type_of(parameter)>(error);
    }

    if (error == cmm::Error::Success) error = cmm::register_rrefl<Function>();
}

} // namespace detail

template <std::meta::info... Functions>
GenerationResult generate_wrapper_fragment()
{
    cmm::Error registration_error = cmm::Error::Success;
    (detail::register_binding<Functions>(registration_error), ...);
    if (registration_error != cmm::Error::Success) return GenerationResult{registration_error, {}};

    const std::array<cmm::info, sizeof...(Functions)> ids{cmm::get_id<Functions>()...};
    return generate_wrapper_fragment(ids);
}

template <std::meta::info Class>
GenerationResult generate_class_wrapper_fragment()
{
    static_assert(std::meta::is_class_type(Class) || std::meta::is_union_type(Class));
    const cmm::Error registration_error = cmm::register_rrefl<Class>();
    if (registration_error != cmm::Error::Success) return GenerationResult{registration_error, {}};

    std::vector<cmm::info> functions;
    for (cmm::info member : cmm::members_view_of(cmm::get_id<Class>()))
    {
        if (cmm::is_function(member)) functions.push_back(member);
    }
    return generate_wrapper_fragment(functions);
}

template <std::meta::info... Functions>
cmm::Error register_bindings()
{
    cmm::Error error = cmm::Error::Success;
    (detail::register_binding<Functions>(error), ...);
    return error;
}

} // namespace cmm::flow

#endif
