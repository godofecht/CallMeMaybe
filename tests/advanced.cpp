#include <memory>
#include <string_view>

#include "cmm/meta.hpp"

struct PublicBase {};
struct Derived : public PublicBase {};

std::unique_ptr<int> make_unique_value() {
    return std::make_unique<int>(42);
}

int consume_unique(std::unique_ptr<int> value) {
    return *value;
}

static bool check(bool condition) {
    return condition;
}

int main() {
    if (cmm::register_rrefl<^^Derived>() != cmm::Error::Success) return 1;
    if (cmm::register_rrefl<^^make_unique_value>() != cmm::Error::Success) return 2;
    if (cmm::register_rrefl<^^consume_unique>() != cmm::Error::Success) return 3;

    const cmm::info derived_id = cmm::reflect_name("Derived");
    const auto bases = cmm::bases_view_of(derived_id);
    if (!check(bases.size() == 1)) return 4;

    const cmm::info base = bases[0];
    if (!check(cmm::is_base(base))) return 5;
    if (!check(cmm::parent_of(base) == derived_id)) return 6;
    if (!check(cmm::type_of(base) == cmm::get_id<^^PublicBase>())) return 7;
    if (!check(cmm::is_public_base(base))) return 8;
    if (!check(!cmm::is_virtual_base(base))) return 9;

    const cmm::info make_id = cmm::reflect_name("make_unique_value");
    cmm::Value result;
    if (cmm::reflect_invoke(make_id, {}, result) != cmm::Error::Success) return 10;
    if (!check(!result.is_copyable())) return 11;
    if (!check(*result.get<std::unique_ptr<int>>() == 42)) return 12;

    cmm::Value copy;
    if (!check(result.try_copy_to(copy) == cmm::Error::NonCopyableValue)) return 13;

    std::unique_ptr<int> owned = std::move(result.get<std::unique_ptr<int>>());
    const cmm::info consume_id = cmm::reflect_name("consume_unique");
    if (!check(cmm::invoke<int>(consume_id, std::move(owned)) == 42)) return 14;
    if (!check(!owned)) return 15;

    if (!check(cmm::identifier_of(make_id) == "make_unique_value")) return 16;
    if (!check(!cmm::display_string_of(make_id).empty())) return 17;

    return 0;
}
