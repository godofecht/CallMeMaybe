#ifndef CALLMEMAYBE_STATIC_SINGLE_METHOD_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_SINGLE_METHOD_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "cmm/annotations.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
struct SingleMethodStaticMetadata
{
    inline static constexpr auto reflected_method = []() consteval
    {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));
        std::meta::info result{};
        std::size_t count = 0;
        template for (constexpr std::meta::info member : members)
        {
            if (cmm::is_reflectable(member) &&
                std::meta::is_function(member) &&
                !std::meta::is_static_member(member))
            {
                result = member;
                ++count;
            }
        }
        if (count != 1) throw "single-method static registry requires exactly one reflected nonstatic member function";
        return result;
    }();

    inline static constexpr cmm::info method_id = cmm::detail::hash_entity(reflected_method);
    inline static constexpr std::array<cmm::info, 1> member_ids{{method_id}};
    inline static constexpr std::array<std::pair<std::string_view, cmm::info>, 1> member_names{{
        {std::meta::identifier_of(reflected_method), method_id}
    }};
};

template <std::meta::info ClassRefl>
consteval auto make_single_method_static_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    constexpr std::meta::info method = SingleMethodStaticMetadata<ClassRefl>::reflected_method;
    static constexpr auto parameters = std::define_static_array(std::meta::parameters_of(method));
    static_assert(parameters.empty(), "single-method static registry currently requires a zero-argument method");

    constexpr std::meta::info ReturnTypeRefl = std::meta::return_type_of(method);
    using ClassT = typename[:ClassRefl:];
    using ReturnT = typename[:ReturnTypeRefl:];

    const cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    const cmm::info method_id = cmm::detail::hash_entity(method);
    const cmm::info return_type_id = cmm::detail::hash_entity(ReturnTypeRefl);

    Class cls(std::meta::display_string_of(ClassRefl));
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    cls.set_members(SingleMethodStaticMetadata<ClassRefl>::member_ids);
    cls.set_member_names(SingleMethodStaticMetadata<ClassRefl>::member_names);

    Function fn(std::meta::identifier_of(method), true, false);
    fn.set_display_name(std::meta::display_string_of(method));
    fn.set_parent_id(class_id);
    fn.set_return_type_id(return_type_id);
    fn.set_is_const_member_function(std::meta::is_const(method));
    fn.set_thunk(cmm::detail::create_thunk<method>());

    Type return_type(std::meta::display_string_of(ReturnTypeRefl));
    if constexpr (!std::is_reference_v<ReturnT> &&
                  !std::is_void_v<ReturnT> &&
                  !std::is_function_v<ReturnT>)
    {
        return_type.set_size(sizeof(ReturnT));
        return_type.set_alignment(alignof(ReturnT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 3> entities{{
        {class_id, cls},
        {method_id, fn},
        {return_type_id, return_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_SINGLE_METHOD_REGISTRY_HPP
