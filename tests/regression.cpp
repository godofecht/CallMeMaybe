#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "cmm/meta.hpp"

struct CollisionA {
    [[=cmm::reflectable]] int value = 0;
    [[=cmm::reflectable]] double other = 0.0;
};

struct CollisionB {
    [[=cmm::reflectable]] double other = 0.0;
    [[=cmm::reflectable]] int value = 0;
};

int named_params_one(int value) { return value; }
int named_params_two(int value) { return value + 1; }
int unnamed_params(int, int) { return 0; }

struct Widget {
    [[=cmm::reflectable]] int payload = 0;
};

Widget* pass_widget(Widget* widget) { return widget; }

enum class State : unsigned int {
    Idle = 0,
    Ready = 1
};

const int const_global = 7;
int mutable_global = 8;

void increment(int& value) { ++value; }
const int& identity_ref(const int& value) { return value; }

struct Invokable {
    [[=cmm::reflectable]] int get() const { return 1; }
};

struct Owned {
    [[=cmm::reflectable]] explicit Owned(int* destruction_count)
        : destruction_count(destruction_count) {}

    ~Owned() { ++*destruction_count; }

    int* destruction_count;
};

union SampleUnion {
    int i;
    float f;
};

struct Overloaded {
    [[=cmm::reflectable]] int call(int value) { return value; }
    [[=cmm::reflectable]] double call(double value) { return value; }
};

struct OrderProbe {
    [[=cmm::reflectable]] int first = 0;
    [[=cmm::reflectable]] void middle() {}
    [[=cmm::reflectable]] int last = 0;
};

static bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

int main() {
    bool ok = true;

    // Registration intentionally happens before any runtime query. pass_widget
    // is registered before Widget to exercise stub promotion.
    const cmm::Error registrations[] = {
        cmm::register_rrefl<^^CollisionA>(),
        cmm::register_rrefl<^^CollisionB>(),
        cmm::register_rrefl<^^named_params_one>(),
        cmm::register_rrefl<^^named_params_two>(),
        cmm::register_rrefl<^^unnamed_params>(),
        cmm::register_rrefl<^^pass_widget>(),
        cmm::register_rrefl<^^Widget>(),
        cmm::register_rrefl<^^State>(),
        cmm::register_rrefl<^^const_global>(),
        cmm::register_rrefl<^^mutable_global>(),
        cmm::register_rrefl<^^increment>(),
        cmm::register_rrefl<^^identity_ref>(),
        cmm::register_rrefl<^^Invokable>(),
        cmm::register_rrefl<^^Owned>(),
        cmm::register_rrefl<^^SampleUnion>(),
        cmm::register_rrefl<^^Overloaded>(),
        cmm::register_rrefl<^^OrderProbe>(),
        cmm::register_rrefl<^^const volatile int>(),
        cmm::register_rrefl<^^unsigned int>(),
        cmm::register_rrefl<^^int&>(),
        cmm::register_rrefl<^^int&&>(),
        cmm::register_rrefl<^^std::nullptr_t>()
    };

    for (cmm::Error err : registrations) {
        ok &= expect(err == cmm::Error::Success, cmm::to_string(err));
    }

    const cmm::info a_id = cmm::reflect_name("CollisionA");
    const cmm::info b_id = cmm::reflect_name("CollisionB");
    const cmm::info a_value = cmm::lookup::get_member(a_id, "value");
    const cmm::info b_value = cmm::lookup::get_member(b_id, "value");

    ok &= expect(a_value != b_value, "same-named data members must have distinct ids");
    ok &= expect(cmm::parent_of(a_value) == a_id, "CollisionA::value parent");
    ok &= expect(cmm::parent_of(b_value) == b_id, "CollisionB::value parent");
    ok &= expect(cmm::offset_of(a_value) == offsetof(CollisionA, value), "CollisionA::value offset");
    ok &= expect(cmm::offset_of(b_value) == offsetof(CollisionB, value), "CollisionB::value offset");

    const cmm::info one_id = cmm::reflect_name("named_params_one");
    const cmm::info two_id = cmm::reflect_name("named_params_two");
    const auto one_params = cmm::parameters_view_of(one_id);
    const auto two_params = cmm::parameters_view_of(two_id);
    ok &= expect(one_params.size() == 1 && two_params.size() == 1, "named parameter metadata exists");
    if (one_params.size() == 1 && two_params.size() == 1) {
        ok &= expect(one_params[0] != two_params[0], "parameters in different functions have distinct ids");
        ok &= expect(cmm::parent_of(one_params[0]) == one_id, "first parameter parent");
        ok &= expect(cmm::parent_of(two_params[0]) == two_id, "second parameter parent");
    }

    const cmm::info unnamed_id = cmm::reflect_name("unnamed_params");
    const auto unnamed = cmm::parameters_view_of(unnamed_id);
    ok &= expect(unnamed.size() == 2, "unnamed parameter metadata exists");
    if (unnamed.size() == 2) {
        ok &= expect(unnamed[0] != unnamed[1], "same-typed unnamed parameters use position in identity");
        ok &= expect(cmm::parent_of(unnamed[0]) == unnamed_id, "unnamed parameter zero parent");
        ok &= expect(cmm::parent_of(unnamed[1]) == unnamed_id, "unnamed parameter one parent");
    }

    const cmm::info widget_id = cmm::reflect_name("Widget");
    ok &= expect(cmm::nonstatic_data_members_view_of(widget_id).size() == 1,
                 "transitively stubbed Widget must later fully register");

    const cmm::info state_id = cmm::reflect_name("State");
    const auto enumerators = cmm::enumerators_of(state_id);
    ok &= expect(enumerators.size() == 2, "enum enumerators registered");
    for (cmm::info enumerator : enumerators) {
        ok &= expect(cmm::parent_of(enumerator) == state_id, "enumerator parent id");
        ok &= expect(cmm::type_of(enumerator) == state_id, "enumerator type id");
    }

    const cmm::info const_global_id = cmm::reflect_name("const_global");
    const cmm::info mutable_global_id = cmm::reflect_name("mutable_global");
    ok &= expect(cmm::address_of(const_global_id) == nullptr,
                 "const global must not expose a mutable address");
    ok &= expect(cmm::const_address_of(const_global_id) != nullptr,
                 "const global exposes a const inspection address");
    int* mutable_address = static_cast<int*>(cmm::address_of(mutable_global_id));
    ok &= expect(mutable_address != nullptr, "mutable global exposes mutable address");
    if (mutable_address) {
        *mutable_address = 9;
        ok &= expect(mutable_global == 9, "mutable global address writes through");
    }

    const cmm::info increment_id = cmm::reflect_name("increment");
    int value = 10;
    cmm::invoke<void>(increment_id, value);
    ok &= expect(value == 11, "variadic invoke preserves mutable lvalue references");

    const cmm::info identity_id = cmm::reflect_name("identity_ref");
    const int referenced = 42;
    const int& returned = cmm::invoke<const int&>(identity_id, referenced);
    ok &= expect(&returned == &referenced, "reference return preserves aliasing");

    const cmm::info invokable_id = cmm::reflect_name("Invokable");
    const cmm::info get_id = cmm::lookup::get_member(invokable_id, "get");
    Invokable* null_instance = nullptr;
    std::array<cmm::Value, 1> null_args{cmm::Value(null_instance)};
    cmm::Value null_result;
    ok &= expect(cmm::reflect_invoke(get_id, null_args, null_result) == cmm::Error::NullValue,
                 "null member-function instance returns NullValue");

    const cmm::info owned_id = cmm::reflect_name("Owned");
    const cmm::info owned_ctor = cmm::lookup::get_constructor<int*>(owned_id);
    int destroyed = 0;
    cmm::Value owned_value = cmm::invoke(owned_ctor, &destroyed);
    Owned* owned_ptr = owned_value.get<Owned*>();
    cmm::info owned_dtor = cmm::invalid_info;
    for (cmm::info member : cmm::members_view_of(owned_id)) {
        if (cmm::is_destructor(member)) {
            owned_dtor = member;
            break;
        }
    }
    ok &= expect(owned_dtor != cmm::invalid_info, "destructor metadata exists");
    if (owned_dtor != cmm::invalid_info) {
        cmm::invoke<void>(owned_dtor, owned_ptr);
        ok &= expect(destroyed == 1, "reflected destructor releases dynamic instance");
    }

    const cmm::info cv_int_id = cmm::get_id<^^const volatile int>();
    const cmm::info uint_id = cmm::get_id<^^unsigned int>();
    const cmm::info lref_id = cmm::get_id<^^int&>();
    const cmm::info rref_id = cmm::get_id<^^int&&>();
    const cmm::info nullptr_id = cmm::get_id<^^std::nullptr_t>();
    const cmm::info union_id = cmm::get_id<^^SampleUnion>();

    ok &= expect(cmm::is_const_type(cv_int_id), "const predicate populated");
    ok &= expect(cmm::is_volatile_type(cv_int_id), "volatile predicate populated");
    ok &= expect(cmm::is_arithmetic_type(cv_int_id), "arithmetic predicate populated");
    ok &= expect(cmm::is_fundamental_type(cv_int_id), "fundamental predicate populated");
    ok &= expect(cmm::is_unsigned_type(uint_id), "unsigned predicate populated");
    ok &= expect(cmm::is_lvalue_reference_type(lref_id), "lvalue reference predicate populated");
    ok &= expect(cmm::is_rvalue_reference_type(rref_id), "rvalue reference predicate populated");
    ok &= expect(cmm::is_null_pointer_type(nullptr_id), "nullptr predicate populated");
    ok &= expect(cmm::is_union_type(union_id), "union predicate populated");
    ok &= expect(!cmm::is_class_type(union_id), "union is not misclassified as class");
    ok &= expect(cmm::is_scoped_enum_type(state_id), "scoped enum predicate populated");

    const cmm::info overloaded_id = cmm::reflect_name("Overloaded");
    ok &= expect(cmm::lookup::get_member(overloaded_id, "call") == cmm::invalid_info,
                 "bare lookup rejects ambiguous overload sets");

    const cmm::info order_id = cmm::reflect_name("OrderProbe");
    std::size_t first_pos = static_cast<std::size_t>(-1);
    std::size_t middle_pos = static_cast<std::size_t>(-1);
    std::size_t last_pos = static_cast<std::size_t>(-1);
    std::size_t pos = 0;
    for (cmm::info member : cmm::members_view_of(order_id)) {
        const auto name = cmm::identifier_of(member);
        if (name == "first") first_pos = pos;
        if (name == "middle") middle_pos = pos;
        if (name == "last") last_pos = pos;
        ++pos;
    }
    ok &= expect(first_pos < middle_pos && middle_pos < last_pos,
                 "members_of preserves reflected declaration order");

    // A runtime query has occurred, so late mutation of the registry is rejected.
    ok &= expect(cmm::register_rrefl<^^long double>() == cmm::Error::RegistryFrozen,
                 "registry freezes when runtime queries begin");

    if (!ok) return 1;
    std::cout << "CallMeMaybe regression suite passed\n";
    return 0;
}
