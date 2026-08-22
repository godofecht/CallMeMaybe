#ifndef CALLMEMAYBE_BASE_HPP
#define CALLMEMAYBE_BASE_HPP

#include <string_view>

#include "cmm/info.hpp"
#include "cmm/detail/entities/entity.hpp"

namespace cmm {
namespace detail {

enum class Access {
    Public,
    Protected,
    Private
};

using BaseUpcastFn = const void* (*)(const void*);

class Base : public Entity {
public:
    Base(std::string_view name, cmm::info type_id, cmm::info parent_id)
        : Entity(name), type_id_(type_id), parent_id_(parent_id) {}

    cmm::info type_id() const { return type_id_; }
    cmm::info parent_id() const { return parent_id_; }
    Access access() const { return access_; }
    bool is_virtual() const { return is_virtual_; }
    bool is_runtime_accessible() const { return upcast_ != nullptr; }

    const void* upcast(const void* instance) const {
        return upcast_ ? upcast_(instance) : nullptr;
    }

    void set_access(Access access) { access_ = access; }
    void set_is_virtual(bool value) { is_virtual_ = value; }
    void set_upcast_thunk(BaseUpcastFn upcast) { upcast_ = upcast; }

private:
    cmm::info type_id_{cmm::invalid_info};
    cmm::info parent_id_{cmm::invalid_info};
    Access access_{Access::Public};
    bool is_virtual_{false};
    BaseUpcastFn upcast_{nullptr};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_BASE_HPP
