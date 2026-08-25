#include <array>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_function_registry.hpp"
#include "cmm/static_meta.hpp"

void static_increment(int& value)
{
    ++value;
}

const int& static_identity_ref(const int& value)
{
    return value;
}

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_two_function_static_registry<^^static_increment, ^^static_identity_ref>()))

int main()
{
    const cmm::info increment_id = cmm::detail::hash_entity(^^static_increment);
    const cmm::info identity_id = cmm::detail::hash_entity(^^static_identity_ref);

    if (cmm::reflect_name("static_increment") != increment_id) return 1;
    if (cmm::reflect_name("static_identity_ref") != identity_id) return 2;

    int value = 10;
    std::array<cmm::Value, 1> increment_args{cmm::Value::ref(value)};
    cmm::Value increment_result;
    if (cmm::reflect_invoke(increment_id, increment_args, increment_result) != cmm::Error::Success) return 3;
    if (value != 11) return 4;

    const int referenced = 42;
    std::array<cmm::Value, 1> identity_args{cmm::Value::cref(referenced)};
    cmm::Value identity_result;
    if (cmm::reflect_invoke(identity_id, identity_args, identity_result) != cmm::Error::Success) return 5;

    const int& returned = identity_result.get<const int&>();
    if (&returned != &referenced) return 6;

    return 0;
}
