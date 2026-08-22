#ifndef CALLMEMAYBE_INFO_HASH_HPP
#define CALLMEMAYBE_INFO_HASH_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <meta>
#include "cmm/info.hpp"

namespace cmm {
namespace detail {

// FNV-1a 64-bit hash. Runtime handles remain compact, but the input identity
// now includes entity kind and ownership so unrelated entities cannot alias
// merely because they share a type and identifier.
constexpr cmm::info hash_string(std::string_view str,
                                cmm::info hash = 0xcbf29ce484222325ULL) {
    for (unsigned char c : str) {
        hash ^= static_cast<cmm::info>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

constexpr cmm::info hash_integer(std::uint64_t value,
                                 cmm::info hash = 0xcbf29ce484222325ULL) {
    for (unsigned int byte = 0; byte < sizeof(value); ++byte) {
        hash ^= static_cast<cmm::info>(value & 0xffU);
        hash *= 0x100000001b3ULL;
        value >>= 8U;
    }
    return hash;
}

// Strip aliases while preserving cv/ref qualification. Runtime invocation
// needs int, const int, int&, etc. to remain distinct metadata types.
consteval std::meta::info canonicalize_type(std::meta::info type_info) {
    std::meta::info t = type_info;
    std::meta::info prev;

    do {
        prev = t;
        if (std::meta::is_type_alias(t)) {
            t = std::meta::dealias(t);
        }
    } while (t != prev);

    return t;
}

consteval cmm::info hash_entity(std::meta::info entity);

consteval cmm::info hash_parameter(std::meta::info entity) {
    const std::meta::info parent = std::meta::parent_of(entity);
    cmm::info hash = hash_string("parameter:", hash_entity(parent));

    std::size_t index = 0;
    for (const std::meta::info parameter : std::meta::parameters_of(parent)) {
        if (parameter == entity) {
            hash = hash_integer(index, hash);
            hash = hash_string(":type:", hash);
            return hash_entity(std::meta::type_of(entity)) ^ hash;
        }
        ++index;
    }

    // Defensive fallback for compiler implementations that fail to surface the
    // parameter through parameters_of(). This remains parent-scoped.
    if (std::meta::has_identifier(entity)) {
        hash = hash_string(std::meta::identifier_of(entity), hash);
    }
    return hash_string(std::meta::display_string_of(std::meta::type_of(entity)), hash);
}

// Accepts any std::meta::info and generates a parent-aware runtime identity.
consteval cmm::info hash_entity(std::meta::info entity) {
    if (std::meta::is_type(entity)) {
        cmm::info hash = hash_string("type:");
        return hash_string(std::meta::display_string_of(canonicalize_type(entity)), hash);
    }

    if (std::meta::is_function(entity)
        || std::meta::is_constructor(entity)
        || std::meta::is_destructor(entity)) {
        cmm::info hash = hash_string("function:");
        return hash_string(std::meta::display_string_of(entity), hash);
    }

    if (std::meta::is_function_parameter(entity)) {
        return hash_parameter(entity);
    }

    if (std::meta::is_enumerator(entity)) {
        cmm::info hash = hash_string("enumerator:", hash_entity(std::meta::parent_of(entity)));
        if (std::meta::has_identifier(entity)) {
            return hash_string(std::meta::identifier_of(entity), hash);
        }
        return hash_string(std::meta::display_string_of(entity), hash);
    }

    if (std::meta::is_class_member(entity)) {
        cmm::info hash = hash_string("member:", hash_entity(std::meta::parent_of(entity)));
        if (std::meta::has_identifier(entity)) {
            hash = hash_string(std::meta::identifier_of(entity), hash);
        } else {
            hash = hash_string(std::meta::display_string_of(entity), hash);
        }

        if (std::meta::is_variable(entity) || std::meta::is_nonstatic_data_member(entity)) {
            hash = hash_string(":type:", hash);
            hash ^= hash_entity(std::meta::type_of(entity));
        }
        return hash;
    }

    if (std::meta::is_variable(entity)) {
        cmm::info hash = hash_string("variable:");
        hash = hash_string(std::meta::display_string_of(entity), hash);
        hash = hash_string(":type:", hash);
        return hash ^ hash_entity(std::meta::type_of(entity));
    }

    cmm::info hash = hash_string("entity:");
    if (std::meta::has_identifier(entity)) {
        hash = hash_string(std::meta::identifier_of(entity), hash);
    } else {
        hash = hash_string(std::meta::display_string_of(entity), hash);
    }
    return hash;
}

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_INFO_HASH_HPP
