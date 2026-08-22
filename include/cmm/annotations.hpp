#ifndef CALLMEMAYBE_ANNOTATIONS_HPP
#define CALLMEMAYBE_ANNOTATIONS_HPP

#include <meta>

namespace cmm {

struct reflectable_t {
    friend constexpr bool operator==(reflectable_t, reflectable_t) = default;
};

inline constexpr reflectable_t reflectable{};

consteval bool is_reflectable(std::meta::info entity) {
#if defined(__clang__)
    return !std::meta::annotations_of(entity, ^^reflectable_t).empty();
#else
    return !std::meta::annotations_of_with_type(entity, ^^reflectable_t).empty();
#endif
}

} // namespace cmm

#endif // CALLMEMAYBE_ANNOTATIONS_HPP
