#ifndef CALLMEMAYBE_ENUM_HPP
#define CALLMEMAYBE_ENUM_HPP

#include <cstdint>
#include <string_view>
#include <vector>
#include "cmm/info.hpp"
#include "cmm/detail/entities/type.hpp"

namespace cmm {
namespace detail {

class Enum : public Type {
public:
    struct Entry {
        std::string_view name;
        std::uint64_t value_bits;
        bool is_signed;
        cmm::info entity_id;
    };

    explicit Enum(std::string_view name) : Type(name) {
        flags_.is_enum = true;
    }

    void add_enumerator(std::string_view name, std::uint64_t value_bits,
                        bool is_signed, cmm::info entity_id) {
        enumerators_.push_back({name, value_bits, is_signed, entity_id});
    }

    void add_enumerator(std::string_view name, std::int64_t value,
                        cmm::info entity_id) {
        add_enumerator(name, static_cast<std::uint64_t>(value), true, entity_id);
    }

    const std::vector<Entry>& enumerators() const { return enumerators_; }

    bool get_value_by_name(std::string_view name, std::int64_t& out_value) const {
        for (const auto& entry : enumerators_) {
            if (entry.name == name) {
                out_value = static_cast<std::int64_t>(entry.value_bits);
                return true;
            }
        }
        return false;
    }

    bool get_value_bits_by_name(std::string_view name, std::uint64_t& out_value,
                                bool& out_is_signed) const {
        for (const auto& entry : enumerators_) {
            if (entry.name == name) {
                out_value = entry.value_bits;
                out_is_signed = entry.is_signed;
                return true;
            }
        }
        return false;
    }

    std::string_view get_name_by_value(std::int64_t value) const {
        return get_name_by_value_bits(static_cast<std::uint64_t>(value));
    }

    std::string_view get_name_by_value_bits(std::uint64_t value_bits) const {
        for (const auto& entry : enumerators_) {
            if (entry.value_bits == value_bits) return entry.name;
        }
        return {};
    }

private:
    std::vector<Entry> enumerators_;
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_ENUM_HPP
