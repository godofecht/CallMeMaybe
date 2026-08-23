#ifndef CALLMEMAYBE_FUNCTION_HPP
#define CALLMEMAYBE_FUNCTION_HPP

#include <span>
#include <string_view>
#include <vector>
#include "cmm/error.hpp"
#include "cmm/info.hpp"
#include "cmm/detail/entities/entity.hpp"
#include "cmm/value.hpp"

namespace cmm {
namespace detail {

using InvokerFn = cmm::Error (*)(std::span<Value>, Value&, const void* instance_override);

struct FunctionFlags {
    bool is_member_function : 1 {false};
    bool is_static_function : 1 {false};
    bool is_const_member_function : 1 {false};
    bool is_constructor : 1 {false};
    bool is_destructor : 1 {false};
};

class Function : public Entity {
public:
    constexpr Function(std::string_view name,
                       bool is_member_function = false,
                       bool is_static_function = false)
        : Entity(name) {
        flags_.is_member_function = is_member_function;
        flags_.is_static_function = is_static_function;
    }

    constexpr void set_thunk(InvokerFn thunk) { thunk_ = thunk; }

    cmm::Error invoke(std::span<Value> args, Value& out,
                      const void* instance_override = nullptr) const {
        if (!thunk_) return cmm::Error::ThunkNotInitialized;
        return thunk_(args, out, instance_override);
    }

    constexpr void set_is_member_function(bool v) { flags_.is_member_function = v; }
    constexpr void set_is_static_function(bool v) { flags_.is_static_function = v; }
    constexpr void set_is_const_member_function(bool v) { flags_.is_const_member_function = v; }
    constexpr void set_is_constructor(bool v) { flags_.is_constructor = v; }
    constexpr void set_is_destructor(bool v) { flags_.is_destructor = v; }

    constexpr void set_parent_id(cmm::info id) { parent_id_ = id; }
    constexpr void set_return_type_id(cmm::info id) { return_type_id_ = id; }

    constexpr void set_parameter_ids(std::span<const cmm::info> ids) {
        parameter_ids_ = ids;
    }

    void add_parameter_id(cmm::info id) {
        owned_parameter_ids_.push_back(id);
        parameter_ids_ = owned_parameter_ids_;
    }

    constexpr bool is_member_function() const { return flags_.is_member_function; }
    constexpr bool is_static_function() const { return flags_.is_static_function; }
    constexpr bool is_const_member_function() const { return flags_.is_const_member_function; }
    constexpr bool is_constructor() const { return flags_.is_constructor; }
    constexpr bool is_destructor() const { return flags_.is_destructor; }
    constexpr const FunctionFlags& flags() const { return flags_; }

    constexpr cmm::info parent_id() const { return parent_id_; }
    constexpr cmm::info return_type_id() const { return return_type_id_; }
    constexpr std::span<const cmm::info> parameter_ids() const { return parameter_ids_; }

private:
    FunctionFlags flags_{};
    cmm::info parent_id_{cmm::invalid_info};
    cmm::info return_type_id_{cmm::invalid_info};
    std::vector<cmm::info> owned_parameter_ids_;
    std::span<const cmm::info> parameter_ids_{};
    InvokerFn thunk_{nullptr};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_FUNCTION_HPP
