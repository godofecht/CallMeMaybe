#ifndef CALLMEMAYBE_VARIABLE_HPP
#define CALLMEMAYBE_VARIABLE_HPP

#include <string_view>

#include "cmm/error.hpp"
#include "cmm/info.hpp"
#include "cmm/detail/entities/entity.hpp"
#include "cmm/value.hpp"

namespace cmm {
namespace detail {

using VariableGetterFn = Value (*)(const void* address);
using VariableRefGetterFn = Value (*)(void* address);
using VariableSetterFn = cmm::Error (*)(void* address, const Value& value);

class Variable : public Entity {
public:
    Variable(std::string_view name, cmm::info type_id)
        : Entity(name), type_id_(type_id) {}

    cmm::Error get_value(Value& out) const {
        if (!address_) return cmm::Error::NullValue;
        if (!getter_) return cmm::Error::ThunkNotInitialized;

        out = getter_(address_);
        return cmm::Error::Success;
    }

    cmm::Error get_ref(Value& out) const {
        if (!address_) return cmm::Error::NullValue;
        if (!ref_getter_) return cmm::Error::ThunkNotInitialized;

        out = ref_getter_(address_);
        return cmm::Error::Success;
    }

    cmm::Error set_value(const Value& value) const {
        if (is_const_) return cmm::Error::ConstViolation;
        if (!address_) return cmm::Error::NullValue;
        if (!setter_) return cmm::Error::ThunkNotInitialized;

        return setter_(address_, value);
    }

    cmm::info type_id() const { return type_id_; }

    const void* address() const { return address_; }
    void* mutable_address() const { return is_const_ ? nullptr : address_; }
    void set_address(void* ptr) { address_ = ptr; }

    bool is_const() const { return is_const_; }
    void set_is_const(bool c) { is_const_ = c; }

    void set_getter_thunk(VariableGetterFn fn) { getter_ = fn; }
    void set_ref_getter_thunk(VariableRefGetterFn fn) { ref_getter_ = fn; }
    void set_setter_thunk(VariableSetterFn fn) { setter_ = fn; }

private:
    cmm::info type_id_{cmm::invalid_info};
    void* address_{nullptr};
    bool is_const_{false};

    VariableGetterFn getter_{nullptr};
    VariableRefGetterFn ref_getter_{nullptr};
    VariableSetterFn setter_{nullptr};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_VARIABLE_HPP
