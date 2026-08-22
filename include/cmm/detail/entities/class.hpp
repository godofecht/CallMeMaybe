#ifndef CALLMEMAYBE_CLASS_HPP
#define CALLMEMAYBE_CLASS_HPP

#include <string_view>
#include <unordered_map>
#include <vector>
#include "cmm/info.hpp"
#include "cmm/detail/entities/type.hpp"

namespace cmm {
namespace detail {

class Class : public Type {
public:
    explicit Class(std::string_view name) : Type(name) {
        flags_.is_class = true;
    }

    void add_member(cmm::info id) { members_.push_back(id); }
    void add_nonstatic_data_member(cmm::info id) { nonstatic_data_members_.push_back(id); }
    void add_static_data_member(cmm::info id) { static_data_members_.push_back(id); }
    void add_function(cmm::info id) { functions_.push_back(id); }
    void add_constructor(cmm::info id) { constructors_.push_back(id); }
    void add_base(cmm::info id) { bases_.push_back(id); }
    void set_destructor(cmm::info id) { destructor_ = id; }

    const std::vector<cmm::info>& members() const { return members_; }
    const std::vector<cmm::info>& nonstatic_data_members() const { return nonstatic_data_members_; }
    const std::vector<cmm::info>& static_data_members() const { return static_data_members_; }
    const std::vector<cmm::info>& functions() const { return functions_; }
    const std::vector<cmm::info>& constructors() const { return constructors_; }
    const std::vector<cmm::info>& bases() const { return bases_; }
    cmm::info destructor() const { return destructor_; }

    void add_member_name(std::string_view name, cmm::info id) {
        auto [it, inserted] = member_name_index_.emplace(name, id);
        if (!inserted) {
            // A bare name is intentionally only usable for unique members.
            // Overloads remain discoverable through members().
            it->second = cmm::invalid_info;
        }
    }

    cmm::info get_member_by_name(std::string_view name) const {
        auto it = member_name_index_.find(name);
        if (it == member_name_index_.end()) return cmm::invalid_info;
        return it->second;
    }

private:
    std::vector<cmm::info> members_;
    std::vector<cmm::info> bases_;
    std::vector<cmm::info> constructors_;
    cmm::info destructor_{cmm::invalid_info};
    std::vector<cmm::info> functions_;
    std::vector<cmm::info> static_data_members_;
    std::vector<cmm::info> nonstatic_data_members_;
    std::unordered_map<std::string_view, cmm::info> member_name_index_;
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_CLASS_HPP
