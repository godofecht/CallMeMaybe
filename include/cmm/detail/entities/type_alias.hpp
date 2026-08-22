#ifndef CALLMEMAYBE_TYPE_ALIAS_HPP
#define CALLMEMAYBE_TYPE_ALIAS_HPP

#include <string_view>

#include "cmm/info.hpp"
#include "cmm/detail/entities/entity.hpp"

namespace cmm {
namespace detail {

class TypeAlias : public Entity {
public:
    TypeAlias(std::string_view name, cmm::info aliased_type_id, cmm::info parent_id)
        : Entity(name), aliased_type_id_(aliased_type_id), parent_id_(parent_id) {}

    cmm::info aliased_type_id() const { return aliased_type_id_; }
    cmm::info parent_id() const { return parent_id_; }

private:
    cmm::info aliased_type_id_{cmm::invalid_info};
    cmm::info parent_id_{cmm::invalid_info};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_TYPE_ALIAS_HPP
