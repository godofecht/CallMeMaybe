#ifndef CALLMEMAYBE_ENUMERATOR_HPP
#define CALLMEMAYBE_ENUMERATOR_HPP

#include <cstdint>
#include <string_view>

#include "cmm/info.hpp"
#include "cmm/detail/entities/entity.hpp"

namespace cmm {
namespace detail {

class Enumerator : public Entity {
public:
    Enumerator(std::string_view name, std::uint64_t value_bits, bool is_signed)
        : Entity(name), value_bits_(value_bits), is_signed_(is_signed) {}

    std::int64_t value() const { return static_cast<std::int64_t>(value_bits_); }
    std::uint64_t value_bits() const { return value_bits_; }
    bool is_signed() const { return is_signed_; }
    cmm::info parent_id() const { return parent_id_; }

    void set_parent_id(cmm::info id) { parent_id_ = id; }

private:
    std::uint64_t value_bits_{0};
    bool is_signed_{false};
    cmm::info parent_id_{cmm::invalid_info};
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_ENUMERATOR_HPP
