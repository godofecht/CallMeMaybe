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
    Function(std::string_view name,
             bool is_member_function = false,
             bool is_static_function = false)
        : Entity(name) {
        flags_.is_member_function = is_member_function;
        flags_.is_static_function = is_static_function;
    }

    void set_thunk(InvokerFn thunk) { thunk_ = thunk; }

    cmm::Error invoke(std::span<Value> args, Value& out,
                      const void* instance_override = nullptr) const {
        if (!thunk_) return cmm::Error::ThunkNotInitialized;
        return thunk_(args, out, instance_override);
    }

    void set_is_member_function(bool v) { flags_.is_member_function = v; }
    void set_is_static_function(bool v) { flags_.is_static_function = v; }
    void set_is_const_member_function(bool v) { flags_.is_const_member_function = v; }
    void set_is_constructor(bool v) { flags_.is_constructor = v; }
    void set_is_destructor(bool v) { flags_.is_destructor = v; }

    void set_parent_id(cmm::info id) { parent_id_ = id; }
    void set_return_type_id(cmm::info id) { return_type_id_ = id; }
    void add_parameter_id(cmm::info id) { parameter_ids_.push_back(id); }

    bool is_member_function() const { return flags_.is_member_function; }
    bool is_static_function() const { return flags_.is_static_function; }
    bool is_const_member_function() const { return flags_.is_const_member_function; }
    bool is_constructor() const { return flags_.is_constructor; }
    bool is_destructor() const { return flags_.is_destructor; }
    const FunctionFlags& flags() const { return flags_; }

    cmm::info parent_id() const { return parent_id_; }
    cmm::info return_type_id() const { return return_type_id_; }
    const std::vector<cmm::info>& parameter_ids() const { return parameter_ids_; }

private:
    FunctionFlags flags_{};
    cmm::info parent_id_{cmm::invalid_info};
    cmm::info return_type_id_{cmm::invalid_info};
    std::vector<cmm::info> parameter_ids_;
    InvokerFn thunk_{nullptr};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_FUNCTION_HPP
