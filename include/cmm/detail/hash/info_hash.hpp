#ifndef CALLMEMAYBE_INFO_HASH_HPP
#define CALLMEMAYBE_INFO_HASH_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <meta>
#include "cmm/info.hpp"

namespace cmm {
namespace detail {

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
            return hash_integer(hash_entity(std::meta::type_of(entity)), hash);
        }
        ++index;
    }

    if (std::meta::has_identifier(entity)) {
        hash = hash_string(std::meta::identifier_of(entity), hash);
    }
    return hash_string(std::meta::display_string_of(std::meta::type_of(entity)), hash);
}

consteval cmm::info hash_base(std::meta::info entity) {
    cmm::info hash = hash_string("base:", hash_entity(std::meta::parent_of(entity)));
    hash = hash_integer(hash_entity(std::meta::type_of(entity)), hash);
    hash = hash_integer(std::meta::is_virtual(entity) ? 1U : 0U, hash);
    hash = hash_integer(std::meta::is_public(entity) ? 0U :
                        std::meta::is_protected(entity) ? 1U : 2U, hash);
    return hash;
}

#if defined(__clang__)

consteval void append_qualified_name(std::meta::info entity,
                                     std::string& result) {
    if (std::meta::is_namespace_member(entity) ||
        std::meta::is_class_member(entity) ||
        std::meta::is_enumerator(entity)) {
        const std::meta::info parent = std::meta::parent_of(entity);
        if (parent != ^^::) {
            append_qualified_name(parent, result);
            if (!result.empty()) result += "::";
        }
    }

    if (std::meta::has_identifier(entity)) {
        result += std::meta::identifier_of(entity);
    } else {
        result += std::meta::display_string_of(entity);
    }
}

consteval const char* qualified_name_of(std::meta::info entity) {
    std::string result;
    append_qualified_name(entity, result);
    return std::define_static_string(result);
}

consteval cmm::info hash_clang_entity(std::meta::info entity,
                                     cmm::info hash);

consteval cmm::info hash_clang_qualified_name(std::meta::info entity,
                                             cmm::info hash) {
    return hash_string(qualified_name_of(entity), hash);
}

consteval cmm::info hash_clang_function_qualifiers(
    std::meta::info function, cmm::info hash) {
    if (std::meta::is_const(function)) hash = hash_string(" const", hash);
    if (std::meta::is_volatile(function)) hash = hash_string(" volatile", hash);
    if (std::meta::is_lvalue_reference_qualified(function)) hash = hash_string(" &", hash);
    else if (std::meta::is_rvalue_reference_qualified(function)) hash = hash_string(" &&", hash);
    if (std::meta::is_noexcept(function)) hash = hash_string(" noexcept", hash);
    return hash;
}

consteval cmm::info hash_clang_type(std::meta::info type,
                                   cmm::info hash) {
    type = canonicalize_type(type);

    if (std::meta::is_function_type(type)) {
        hash = hash_string("function-type(", hash);
        hash = hash_clang_type(std::meta::return_type_of(type), hash);
        for (const std::meta::info parameter : std::meta::parameters_of(type)) {
            hash = hash_string(",", hash);
            hash = hash_clang_type(parameter, hash);
        }
        hash = hash_string(")", hash);
        return hash_clang_function_qualifiers(type, hash);
    }

    if (std::meta::is_const_type(type) || std::meta::is_volatile_type(type)) {
        if (std::meta::is_const_type(type)) hash = hash_string("const ", hash);
        if (std::meta::is_volatile_type(type)) hash = hash_string("volatile ", hash);
        return hash_clang_type(std::meta::remove_cv(type), hash);
    }
    if (std::meta::is_pointer_type(type)) {
        return hash_clang_type(std::meta::remove_pointer(type), hash_string("pointer ", hash));
    }
    if (std::meta::is_lvalue_reference_type(type)) {
        return hash_clang_type(std::meta::remove_reference(type), hash_string("lref ", hash));
    }
    if (std::meta::is_rvalue_reference_type(type)) {
        return hash_clang_type(std::meta::remove_reference(type), hash_string("rref ", hash));
    }
    if (std::meta::is_array_type(type)) {
        hash = hash_string("array ", hash);
        for (std::size_t rank = 0; rank < std::meta::rank(type); ++rank) {
            hash = hash_integer(std::meta::extent(type, rank), hash);
        }
        return hash_clang_type(std::meta::remove_all_extents(type), hash);
    }

    if (std::meta::has_template_arguments(type)) {
        hash = hash_clang_qualified_name(std::meta::template_of(type),
                                         hash_string("template ", hash));
        for (const std::meta::info argument : std::meta::template_arguments_of(type)) {
            hash = hash_string("<arg>", hash);
            hash = hash_clang_entity(argument, hash);
        }
        return hash;
    }

    if (std::meta::has_identifier(type)) {
        return hash_clang_qualified_name(type, hash_string("named-type ", hash));
    }
    return hash_string(std::meta::display_string_of(type), hash_string("type ", hash));
}

consteval cmm::info hash_clang_entity(std::meta::info entity,
                                     cmm::info hash) {
    if (std::meta::is_type(entity)) {
        return hash_clang_type(entity, hash_string("type:", hash));
    }

    if (std::meta::is_function(entity) ||
        std::meta::is_constructor(entity) ||
        std::meta::is_destructor(entity)) {
        hash = hash_clang_qualified_name(entity, hash_string("function:", hash));
        for (const std::meta::info parameter : std::meta::parameters_of(entity)) {
            hash = hash_string(",", hash);
            hash = hash_clang_type(std::meta::type_of(parameter), hash);
        }
        if (!std::meta::is_constructor(entity) && !std::meta::is_destructor(entity)) {
            hash = hash_string(" returns ", hash);
            hash = hash_clang_type(std::meta::return_type_of(entity), hash);
        }
        return hash_clang_function_qualifiers(entity, hash);
    }

    if (std::meta::is_function_parameter(entity)) return hash_parameter(entity);
    if (std::meta::is_base(entity)) return hash_base(entity);

    hash = hash_clang_qualified_name(entity, hash_string("entity:", hash));
    if (std::meta::is_variable(entity) ||
        std::meta::is_nonstatic_data_member(entity)) {
        hash = hash_string(":type:", hash);
        hash = hash_clang_type(std::meta::type_of(entity), hash);
    }
    return hash;
}

#endif

consteval cmm::info hash_entity(std::meta::info entity) {
#if defined(__clang__)
    return hash_clang_entity(entity, 0xcbf29ce484222325ULL);
#else
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

    if (std::meta::is_function_parameter(entity)) return hash_parameter(entity);
    if (std::meta::is_base(entity)) return hash_base(entity);

    if (std::meta::is_enumerator(entity)) {
        cmm::info hash = hash_string("enumerator:", hash_entity(std::meta::parent_of(entity)));
        if (std::meta::has_identifier(entity)) return hash_string(std::meta::identifier_of(entity), hash);
        return hash_string(std::meta::display_string_of(entity), hash);
    }

    if (std::meta::is_class_member(entity)) {
        cmm::info hash = hash_string("member:", hash_entity(std::meta::parent_of(entity)));
        if (std::meta::has_identifier(entity)) hash = hash_string(std::meta::identifier_of(entity), hash);
        else hash = hash_string(std::meta::display_string_of(entity), hash);

        if (std::meta::is_variable(entity) || std::meta::is_nonstatic_data_member(entity)) {
            hash = hash_string(":type:", hash);
            hash = hash_integer(hash_entity(std::meta::type_of(entity)), hash);
        }
        return hash;
    }

    if (std::meta::is_variable(entity)) {
        cmm::info hash = hash_string("variable:");
        hash = hash_string(std::meta::display_string_of(entity), hash);
        hash = hash_string(":type:", hash);
        return hash_integer(hash_entity(std::meta::type_of(entity)), hash);
    }

    cmm::info hash = hash_string("entity:");
    if (std::meta::has_identifier(entity)) hash = hash_string(std::meta::identifier_of(entity), hash);
    else hash = hash_string(std::meta::display_string_of(entity), hash);
    return hash;
#endif
}

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_INFO_HASH_HPP
