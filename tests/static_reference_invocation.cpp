#include <array>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_function_registry.hpp"
#include "cmm/static_invoke.hpp"
#include "cmm/static_query.hpp"

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
    const cmm::info void_id = cmm::detail::hash_entity(^^void);
    const cmm::info const_int_ref_id = cmm::detail::hash_entity(^^const int&);

    if (cmm::reflect_name("static_increment") != increment_id) return 1;
    if (cmm::reflect_name("static_identity_ref") != identity_id) return 2;
    if (!cmm::is_function(increment_id) || !cmm::is_function(identity_id)) return 3;
    if (cmm::return_type_of(increment_id) != void_id) return 4;
    if (cmm::return_type_of(identity_id) != const_int_ref_id) return 5;
    if (cmm::return_type_of(void_id) != cmm::invalid_info) return 6;

    int value = 10;
    std::array<cmm::Value, 1> increment_args{cmm::Value::ref(value)};
    cmm::Value increment_result;
    if (cmm::reflect_invoke(increment_id, increment_args, increment_result) != cmm::Error::Success) return 7;
    if (value != 11) return 8;

    const int referenced = 42;
    std::array<cmm::Value, 1> identity_args{cmm::Value::cref(referenced)};
    cmm::Value identity_result;
    if (cmm::reflect_invoke(identity_id, identity_args, identity_result) != cmm::Error::Success) return 9;

    const int& returned = identity_result.get<const int>();
    if (&returned != &referenced) return 10;

    int typed_value = 20;
    cmm::invoke<void>(increment_id, typed_value);
    if (typed_value != 21) return 11;

    const int typed_referenced = 84;
    const int& typed_returned = cmm::invoke<const int&>(identity_id, typed_referenced);
    if (&typed_returned != &typed_referenced) return 12;

    return 0;
}
