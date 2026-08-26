#ifndef CMM_STATIC_FUNCTION_ENUM_METADATA_HPP
#define CMM_STATIC_FUNCTION_ENUM_METADATA_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <meta>
#include <type_traits>

#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/info.hpp"

namespace cmm::detail {

template <std::meta::info FuncRefl>
struct StaticFunctionMetadata {
private:
    inline static constexpr auto parameters =
        std::define_static_array(std::meta::parameters_of(FuncRefl));

public:
    inline static constexpr auto parameter_ids = []() consteval {
        std::array<cmm::info, parameters.size()> result{};
        std::size_t index = 0;
        template for (constexpr std::meta::info parameter : parameters) {
            result[index++] = cmm::detail::hash_entity(parameter);
        }
        return result;
    }();

    static constexpr void apply(Function& function) {
        function.set_parameter_ids(parameter_ids);
    }
};

template <std::meta::info EnumRefl>
struct StaticEnumMetadata {
private:
    inline static constexpr auto enumerators =
        std::define_static_array(std::meta::enumerators_of(EnumRefl));

public:
    inline static constexpr auto entries = []() consteval {
        std::array<Enum::Entry, enumerators.size()> result{};
        std::size_t index = 0;
        using Underlying = typename[:std::meta::underlying_type(EnumRefl):];
        using Unsigned = std::make_unsigned_t<Underlying>;
        constexpr bool is_signed = std::is_signed_v<Underlying>;

        template for (constexpr std::meta::info enumerator : enumerators) {
            result[index++] = Enum::Entry{
                std::meta::identifier_of(enumerator),
                static_cast<std::uint64_t>(static_cast<Unsigned>([:enumerator:])),
                is_signed,
                cmm::detail::hash_entity(enumerator)
            };
        }
        return result;
    }();

    static constexpr void apply(Enum& enumeration) {
        enumeration.set_enumerators(entries);
    }
};

} // namespace cmm::detail

#endif
