#ifndef CALLMEMAYBE_CLASS_HPP
#define CALLMEMAYBE_CLASS_HPP

#include <span>
#include <string_view>
#include <utility>
#include <vector>
#include "cmm/info.hpp"
#include "cmm/detail/entities/type.hpp"

namespace cmm {
namespace detail {

class Class : public Type {
public:
    constexpr explicit Class(std::string_view name) : Type(name) {
        flags_.is_class = true;
    }

    void add_member(cmm::info id) {
        owned_members_.push_back(id);
        members_ = owned_members_;
    }

    void add_nonstatic_data_member(cmm::info id) {
        owned_nonstatic_data_members_.push_back(id);
        nonstatic_data_members_ = owned_nonstatic_data_members_;
    }

    void add_static_data_member(cmm::info id) {
        owned_static_data_members_.push_back(id);
        static_data_members_ = owned_static_data_members_;
    }

    void add_function(cmm::info id) {
        owned_functions_.push_back(id);
        functions_ = owned_functions_;
    }

    void add_constructor(cmm::info id) {
        owned_constructors_.push_back(id);
        constructors_ = owned_constructors_;
    }

    void add_base(cmm::info id) {
        owned_bases_.push_back(id);
        bases_ = owned_bases_;
    }

    constexpr void set_members(std::span<const cmm::info> ids) { members_ = ids; }
    constexpr void set_nonstatic_data_members(std::span<const cmm::info> ids) { nonstatic_data_members_ = ids; }
    constexpr void set_static_data_members(std::span<const cmm::info> ids) { static_data_members_ = ids; }
    constexpr void set_functions(std::span<const cmm::info> ids) { functions_ = ids; }
    constexpr void set_constructors(std::span<const cmm::info> ids) { constructors_ = ids; }
    constexpr void set_bases(std::span<const cmm::info> ids) { bases_ = ids; }
    constexpr void set_destructor(cmm::info id) { destructor_ = id; }

    constexpr std::span<const cmm::info> members() const { return members_; }
    constexpr std::span<const cmm::info> nonstatic_data_members() const { return nonstatic_data_members_; }
    constexpr std::span<const cmm::info> static_data_members() const { return static_data_members_; }
    constexpr std::span<const cmm::info> functions() const { return functions_; }
    constexpr std::span<const cmm::info> constructors() const { return constructors_; }
    constexpr std::span<const cmm::info> bases() const { return bases_; }
    constexpr cmm::info destructor() const { return destructor_; }

    void add_member_name(std::string_view name, cmm::info id) {
        for (auto& entry : owned_member_name_index_) {
            if (entry.first == name) {
                entry.second = cmm::invalid_info;
                member_name_index_ = owned_member_name_index_;
                return;
            }
        }
        owned_member_name_index_.push_back({name, id});
        member_name_index_ = owned_member_name_index_;
    }

    constexpr void set_member_names(
        std::span<const std::pair<std::string_view, cmm::info>> entries) {
        member_name_index_ = entries;
    }

    constexpr cmm::info get_member_by_name(std::string_view name) const {
        cmm::info result = cmm::invalid_info;
        bool found = false;
        for (const auto& entry : member_name_index_) {
            if (entry.first != name) continue;
            if (found) return cmm::invalid_info;
            result = entry.second;
            found = true;
        }
        return found ? result : cmm::invalid_info;
    }

private:
    std::vector<cmm::info> owned_members_;
    std::vector<cmm::info> owned_bases_;
    std::vector<cmm::info> owned_constructors_;
    std::vector<cmm::info> owned_functions_;
    std::vector<cmm::info> owned_static_data_members_;
    std::vector<cmm::info> owned_nonstatic_data_members_;
    std::vector<std::pair<std::string_view, cmm::info>> owned_member_name_index_;

    std::span<const cmm::info> members_{};
    std::span<const cmm::info> bases_{};
    std::span<const cmm::info> constructors_{};
    cmm::info destructor_{cmm::invalid_info};
    std::span<const cmm::info> functions_{};
    std::span<const cmm::info> static_data_members_{};
    std::span<const cmm::info> nonstatic_data_members_{};
    std::span<const std::pair<std::string_view, cmm::info>> member_name_index_{};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_CLASS_HPP
