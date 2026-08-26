#include <memory>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_zero_arg_function_registry.hpp"
#include "cmm/static_invoke.hpp"

std::unique_ptr<int> static_make_unique_value()
{
    return std::make_unique<int>(42);
}

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_zero_arg_function_static_registry<^^static_make_unique_value>()))

int main()
{
    const cmm::info function_id = cmm::detail::hash_entity(^^static_make_unique_value);

    if (cmm::reflect_name("static_make_unique_value") != function_id) return 1;

    cmm::Value raw_result;
    if (cmm::reflect_invoke(function_id, {}, raw_result) != cmm::Error::Success) return 2;
    if (raw_result.is_copyable()) return 3;
    if (!raw_result.get<std::unique_ptr<int>>() ||
        *raw_result.get<std::unique_ptr<int>>() != 42) return 4;

    cmm::Value copy;
    if (raw_result.try_copy_to(copy) != cmm::Error::NonCopyableValue) return 5;

    std::unique_ptr<int> typed_result = cmm::invoke<std::unique_ptr<int>>(function_id);
    if (!typed_result || *typed_result != 42) return 6;

    return 0;
}
