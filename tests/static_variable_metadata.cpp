#include <string_view>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_two_variable_registry.hpp"
#include "cmm/static_query.hpp"

const int static_const_global = 7;
int static_mutable_global = 8;

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_two_variable_static_registry<^^static_const_global, ^^static_mutable_global>()))

int main()
{
    const cmm::info const_id = cmm::detail::hash_entity(^^static_const_global);
    const cmm::info mutable_id = cmm::detail::hash_entity(^^static_mutable_global);
    const cmm::info const_type_id = cmm::detail::hash_entity(^^const int);
    const cmm::info mutable_type_id = cmm::detail::hash_entity(^^int);

    if (cmm::reflect_name("static_const_global") != const_id) return 1;
    if (cmm::reflect_name("static_mutable_global") != mutable_id) return 2;
    if (cmm::identifier_of(const_id) != std::string_view("static_const_global")) return 3;
    if (cmm::identifier_of(mutable_id) != std::string_view("static_mutable_global")) return 4;
    if (cmm::type_of(const_id) != const_type_id) return 5;
    if (cmm::type_of(mutable_id) != mutable_type_id) return 6;

    if (cmm::const_address_of(const_id) != &static_const_global) return 7;
    if (cmm::address_of(const_id) != nullptr) return 8;
    if (cmm::const_address_of(mutable_id) != &static_mutable_global) return 9;
    if (cmm::address_of(mutable_id) != &static_mutable_global) return 10;

    const auto* const_variable =
        cmm::detail::active_static_registry().try_get_as<cmm::detail::Variable>(const_id);
    const auto* mutable_variable =
        cmm::detail::active_static_registry().try_get_as<cmm::detail::Variable>(mutable_id);
    if (!const_variable || !mutable_variable) return 11;

    cmm::Value const_value;
    if (const_variable->get_value(const_value) != cmm::Error::Success) return 12;
    if (const_value.get<int>() != 7) return 13;
    if (const_variable->set_value(cmm::Value(9)) != cmm::Error::ConstViolation) return 14;

    cmm::Value mutable_ref;
    if (mutable_variable->get_ref(mutable_ref) != cmm::Error::Success) return 15;
    int* mutable_ptr = mutable_ref.get_if<int>();
    if (!mutable_ptr) return 16;
    *mutable_ptr = 9;
    if (static_mutable_global != 9) return 17;

    if (mutable_variable->set_value(cmm::Value(10)) != cmm::Error::Success) return 18;
    if (static_mutable_global != 10) return 19;

    return 0;
}
