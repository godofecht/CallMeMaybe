#ifndef CALLMEMAYBE_ENTITY_HPP
#define CALLMEMAYBE_ENTITY_HPP

#include <string_view>

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

protected:
    std::string_view name_;
    std::string_view display_name_;
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_ENTITY_HPP
