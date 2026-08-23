#include <array>
#include <iostream>
#include <memory>
#include <string_view>

#include "cmm/meta.hpp"

struct PaddingBase {
    char padding[32]{};
};

struct PublicBase {
    [[=cmm::reflectable]] int read() const { return state; }
    [[=cmm::reflectable]] void write(int value) { state = value; }
    int state = 17;
};

struct Derived : public PaddingBase, public PublicBase {};

std::unique_ptr<int> make_unique_value() {
    return std::make_unique<int>(42);
}

int consume_unique(std::unique_ptr<int> value) {
    return *value;
}

int managed_destructions = 0;

struct Managed {
    [[=cmm::reflectable]] Managed() = default;
    ~Managed() { ++managed_destructions; }
};

static bool check(bool condition) {
    return condition;
}

int main() {
    if (cmm::register_rrefl<^^Derived>() != cmm::Error::Success) return 1;
    if (cmm::register_rrefl<^^make_unique_value>() != cmm::Error::Success) return 2;
    if (cmm::register_rrefl<^^consume_unique>() != cmm::Error::Success) return 3;
    if (cmm::register_rrefl<^^Managed>() != cmm::Error::Success) return 4;

    const cmm::info derived_id = cmm::reflect_name("Derived");
    const auto bases = cmm::bases_view_of(derived_id);
    if (!check(bases.size() == 2)) return 5;

    cmm::info public_base_specifier = cmm::invalid_info;
    for (cmm::info base : bases) {
        if (cmm::type_of(base) == cmm::get_id<^^PublicBase>()) {
            public_base_specifier = base;
            break;
        }
    }
    if (!check(public_base_specifier != cmm::invalid_info)) return 6;
    if (!check(cmm::is_base(public_base_specifier))) return 7;
    if (!check(cmm::parent_of(public_base_specifier) == derived_id)) return 8;
    if (!check(cmm::is_public_base(public_base_specifier))) return 9;
    if (!check(!cmm::is_virtual_base(public_base_specifier))) return 10;

    Derived derived;
    PublicBase* native_base = static_cast<PublicBase*>(&derived);
    if (!check(static_cast<void*>(&derived) != static_cast<void*>(native_base))) return 11;

    const cmm::info public_base_id = cmm::reflect_name("PublicBase");
    const cmm::info read_id = cmm::lookup::get_member(public_base_id, "read");
    const cmm::info write_id = cmm::lookup::get_member(public_base_id, "write");

    std::array<cmm::Value, 1> read_args{cmm::Value(&derived)};
    cmm::Value read_result;
    const cmm::Error read_error = cmm::reflect_invoke(read_id, read_args, read_result);
    if (read_error != cmm::Error::Success) {
        std::cerr << "derived read error: " << cmm::to_string(read_error)
                  << " instance_type=" << read_args[0].pointee_type_id()
                  << " derived_type=" << derived_id
                  << " parent_type=" << public_base_id << '\n';
        return 12;
    }
    if (!check(read_result.get<int>() == 17)) return 12;

    std::cerr << "advanced stage: derived write\n";
    cmm::invoke<void>(write_id, &derived, 31);
    if (!check(derived.state == 31)) return 13;

    const Derived* const_derived = &derived;
    std::cerr << "advanced stage: const derived read\n";
    if (!check(cmm::invoke<int>(read_id, const_derived) == 31)) return 14;
    std::array<cmm::Value, 2> const_write_args{
        cmm::Value(const_derived),
        cmm::Value(99)
    };
    cmm::Value ignored;
    if (!check(cmm::reflect_invoke(write_id, const_write_args, ignored) == cmm::Error::ConstViolation)) return 15;

    const cmm::info make_id = cmm::reflect_name("make_unique_value");
    cmm::Value result;
    if (cmm::reflect_invoke(make_id, {}, result) != cmm::Error::Success) return 16;
    if (!check(!result.is_copyable())) return 17;
    if (!check(*result.get<std::unique_ptr<int>>() == 42)) return 18;

    cmm::Value copy;
    if (!check(result.try_copy_to(copy) == cmm::Error::NonCopyableValue)) return 19;

    std::cerr << "advanced stage: typed unique return\n";
    std::unique_ptr<int> typed_result = cmm::invoke<std::unique_ptr<int>>(make_id);
    if (!check(typed_result && *typed_result == 42)) return 20;

    std::unique_ptr<int> owned = std::move(result.get<std::unique_ptr<int>>());
    const cmm::info consume_id = cmm::reflect_name("consume_unique");
    std::cerr << "advanced stage: unique consume\n";
    if (!check(cmm::invoke<int>(consume_id, std::move(owned)) == 42)) return 21;
    if (!check(!owned)) return 22;

    const cmm::info managed_id = cmm::reflect_name("Managed");
    {
        std::cerr << "advanced stage: construct managed\n";
        cmm::DynamicObject managed = cmm::construct(managed_id);
        if (!check(managed.get<Managed>() != nullptr)) return 23;
        if (!check(managed_destructions == 0)) return 24;
    }
    if (!check(managed_destructions == 1)) return 25;

    if (!check(cmm::identifier_of(make_id) == "make_unique_value")) return 26;
    if (!check(!cmm::display_string_of(make_id).empty())) return 27;

    return 0;
}
