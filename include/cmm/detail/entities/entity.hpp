#ifndef CALLMEMAYBE_ENTITY_HPP
#define CALLMEMAYBE_ENTITY_HPP

#include <string_view>
#include <vector>
#include "cmm/info.hpp"

namespace cmm {
namespace detail {

class Entity {
public:
    explicit Entity(std::string_view name)
        : name_(name), display_name_(name) {}

    std::string_view name() const { return name_; }
    std::string_view display_name() const { return display_name_; }

    void set_display_name(std::string_view display_name) {
        display_name_ = display_name;
    }

    void add_annotation(cmm::info id) { annotations_.push_back(id); }
    const std::vector<cmm::info>& annotations() const { return annotations_; }

protected:
    std::string_view name_;
    std::string_view display_name_;
    std::vector<cmm::info> annotations_;
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_ENTITY_HPP
