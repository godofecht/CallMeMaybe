#include <cstddef>

#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/static_active_registry.hpp"
#include "cmm/detail/static_type_registry.hpp"
#include "cmm/static_meta.hpp"
#include "cmm/static_query.hpp"

union StaticPredicateUnion
{
    int i;
    float f;
};

enum class StaticPredicateState : unsigned int
{
    Idle = 0,
    Ready = 1
};

CMM_USE_STATIC_REGISTRY(
    (cmm::detail::make_type_static_registry<
        ^^int,
        ^^int*,
        ^^const volatile int,
        ^^unsigned int,
        ^^int&,
        ^^int&&,
        ^^std::nullptr_t,
        ^^StaticPredicateUnion,
        ^^StaticPredicateState>()))

int main()
{
    const cmm::info int_id = cmm::detail::hash_entity(^^int);
    const cmm::info pointer_id = cmm::detail::hash_entity(^^int*);
    const cmm::info cv_int_id = cmm::detail::hash_entity(^^const volatile int);
    const cmm::info uint_id = cmm::detail::hash_entity(^^unsigned int);
    const cmm::info lref_id = cmm::detail::hash_entity(^^int&);
    const cmm::info rref_id = cmm::detail::hash_entity(^^int&&);
    const cmm::info nullptr_id = cmm::detail::hash_entity(^^std::nullptr_t);
    const cmm::info union_id = cmm::detail::hash_entity(^^StaticPredicateUnion);
    const cmm::info state_id = cmm::detail::hash_entity(^^StaticPredicateState);

    if (cmm::type_of(cv_int_id) != cv_int_id) return 1;
    if (!cmm::is_const_type(cv_int_id)) return 2;
    if (!cmm::is_volatile_type(cv_int_id)) return 3;
    if (!cmm::is_integral_type(cv_int_id)) return 4;
    if (!cmm::is_arithmetic_type(cv_int_id)) return 5;
    if (!cmm::is_fundamental_type(cv_int_id)) return 6;
    if (!cmm::is_signed_type(cv_int_id)) return 7;

    if (!cmm::is_unsigned_type(uint_id)) return 8;
    if (!cmm::is_integral_type(uint_id)) return 9;
    if (!cmm::is_arithmetic_type(uint_id)) return 10;

    if (!cmm::is_pointer_type(pointer_id)) return 11;
    if (cmm::underlying_type(pointer_id) != int_id) return 12;

    if (!cmm::is_lvalue_reference_type(lref_id)) return 13;
    if (!cmm::is_reference_type(lref_id)) return 14;
    if (cmm::is_rvalue_reference_type(lref_id)) return 15;
    if (cmm::underlying_type(lref_id) != int_id) return 16;

    if (!cmm::is_rvalue_reference_type(rref_id)) return 17;
    if (!cmm::is_reference_type(rref_id)) return 18;
    if (cmm::is_lvalue_reference_type(rref_id)) return 19;
    if (cmm::underlying_type(rref_id) != int_id) return 20;

    if (!cmm::is_null_pointer_type(nullptr_id)) return 21;
    if (!cmm::is_fundamental_type(nullptr_id)) return 22;

    if (!cmm::is_union_type(union_id)) return 23;
    if (cmm::is_class_type(union_id)) return 24;

    if (!cmm::is_enum_type(state_id)) return 25;
    if (!cmm::is_scoped_enum_type(state_id)) return 26;
    if (cmm::is_class_type(state_id)) return 27;

    if (cmm::is_pointer_type(cmm::invalid_info)) return 28;
    if (cmm::is_enum_type(cv_int_id)) return 29;

    return 0;
}
