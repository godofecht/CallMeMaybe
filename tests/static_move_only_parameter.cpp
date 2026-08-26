#include <memory>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_function_registry.hpp"
#include "cmm/static_invoke.hpp"

int static_consume_unique(std::unique_ptr<int> value)
{
    return *value;
}

int static_consume_unique_plus_one(std::unique_ptr<int> value)
{
    return *value + 1;
}

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_two_function_static_registry<
        ^^static_consume_unique,
        ^^static_consume_unique_plus_one>()))

int main()
{
    const cmm::info consume_id = cmm::detail::hash_entity(^^static_consume_unique);
    const cmm::info plus_one_id = cmm::detail::hash_entity(^^static_consume_unique_plus_one);

    if (cmm::reflect_name("static_consume_unique") != consume_id) return 1;
    if (cmm::reflect_name("static_consume_unique_plus_one") != plus_one_id) return 2;

    std::unique_ptr<int> first = std::make_unique<int>(42);
    if (cmm::invoke<int>(consume_id, std::move(first)) != 42) return 3;
    if (first) return 4;

    std::unique_ptr<int> second = std::make_unique<int>(42);
    if (cmm::invoke<int>(plus_one_id, std::move(second)) != 43) return 5;
    if (second) return 6;

    return 0;
}
