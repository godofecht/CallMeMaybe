#ifndef CMM_STATIC_CLASS_METADATA_HPP
#define CMM_STATIC_CLASS_METADATA_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>
#include <utility>

#include "cmm/annotations.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/info.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
struct StaticClassMetadata {
private:
    static consteval bool included_member(std::meta::info member) {
        return cmm::is_reflectable(member) || std::meta::is_destructor(member);
    }

    static consteval std::size_t count_members() {
        std::size_t count = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (included_member(member)) ++count;
        }
        return count;
    }

    static consteval std::size_t count_nonstatic_data_members() {
        std::size_t count = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_nonstatic_data_member(member)) ++count;
        }
        return count;
    }

    static consteval std::size_t count_static_data_members() {
        std::size_t count = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_static_member(member) &&
                !std::meta::is_function(member)) ++count;
        }
        return count;
    }

    static consteval std::size_t count_functions() {
        std::size_t count = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_function(member) &&
                !std::meta::is_constructor(member) && !std::meta::is_destructor(member)) ++count;
        }
        return count;
    }

    static consteval std::size_t count_constructors() {
        std::size_t count = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_constructor(member)) ++count;
        }
        return count;
    }

    static consteval std::size_t count_names() {
        std::size_t count = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (included_member(member) && std::meta::has_identifier(member)) ++count;
        }
        return count;
    }

public:
    inline static constexpr auto member_ids = []() consteval {
        std::array<cmm::info, count_members()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (included_member(member)) result[index++] = cmm::detail::hash_entity(member);
        }
        return result;
    }();

    inline static constexpr auto nonstatic_data_member_ids = []() consteval {
        std::array<cmm::info, count_nonstatic_data_members()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_nonstatic_data_member(member)) {
                result[index++] = cmm::detail::hash_entity(member);
            }
        }
        return result;
    }();

    inline static constexpr auto static_data_member_ids = []() consteval {
        std::array<cmm::info, count_static_data_members()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_static_member(member) &&
                !std::meta::is_function(member)) {
                result[index++] = cmm::detail::hash_entity(member);
            }
        }
        return result;
    }();

    inline static constexpr auto function_ids = []() consteval {
        std::array<cmm::info, count_functions()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_function(member) &&
                !std::meta::is_constructor(member) && !std::meta::is_destructor(member)) {
                result[index++] = cmm::detail::hash_entity(member);
            }
        }
        return result;
    }();

    inline static constexpr auto constructor_ids = []() consteval {
        std::array<cmm::info, count_constructors()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (cmm::is_reflectable(member) && std::meta::is_constructor(member)) {
                result[index++] = cmm::detail::hash_entity(member);
            }
        }
        return result;
    }();

    inline static constexpr auto base_ids = []() consteval {
        constexpr auto bases = std::define_static_array(
            std::meta::bases_of(ClassRefl, std::meta::access_context::unchecked()));
        std::array<cmm::info, bases.size()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info base : bases) {
            result[index++] = cmm::detail::hash_entity(base);
        }
        return result;
    }();

    inline static constexpr cmm::info destructor_id = []() consteval {
        cmm::info result = cmm::invalid_info;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (std::meta::is_destructor(member)) result = cmm::detail::hash_entity(member);
        }
        return result;
    }();

    inline static constexpr auto member_names = []() consteval {
        std::array<std::pair<std::string_view, cmm::info>, count_names()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (included_member(member) && std::meta::has_identifier(member)) {
                result[index++] = {std::meta::identifier_of(member), cmm::detail::hash_entity(member)};
            }
        }
        return result;
    }();

    static constexpr void apply(Class& cls) {
        cls.set_members(member_ids);
        cls.set_nonstatic_data_members(nonstatic_data_member_ids);
        cls.set_static_data_members(static_data_member_ids);
        cls.set_functions(function_ids);
        cls.set_constructors(constructor_ids);
        cls.set_bases(base_ids);
        cls.set_destructor(destructor_id);
        cls.set_member_names(member_names);
    }
};

} // namespace cmm::detail

#endif
