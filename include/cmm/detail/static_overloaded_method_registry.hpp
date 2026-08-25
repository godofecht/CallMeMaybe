#ifndef CALLMEMAYBE_STATIC_OVERLOADED_METHOD_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_OVERLOADED_METHOD_REGISTRY_HPP

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
struct OverloadedMethodStaticMetadata
{
    inline static constexpr auto reflected_methods = []() consteval
    {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));
        std::array<std::meta::info, 2> result{};
        std::size_t count = 0;
        template for (constexpr std::meta::info member : members)
        {
            if (cmm::is_reflectable(member) &&
                std::meta::is_function(member) &&
                !std::meta::is_static_member(member))
            {
                if (count >= result.size())
                    throw "overloaded-method static registry requires exactly two reflected nonstatic member functions";
                result[count++] = member;
            }
        }
        if (count != result.size())
            throw "overloaded-method static registry requires exactly two reflected nonstatic member functions";
        return result;
    }();

    inline static constexpr std::meta::info mutable_method = []() consteval
    {
        for (std::meta::info method : reflected_methods)
            if (!std::meta::is_const(method)) return method;
        throw "overloaded-method static registry requires one mutable overload";
    }();

    inline static constexpr std::meta::info const_method = []() consteval
    {
        for (std::meta::info method : reflected_methods)
            if (std::meta::is_const(method)) return method;
        throw "overloaded-method static registry requires one const overload";
    }();

    static_assert(std::meta::identifier_of(mutable_method) == std::meta::identifier_of(const_method));

    inline static constexpr cmm::info mutable_method_id = cmm::detail::hash_entity(mutable_method);
    inline static constexpr cmm::info const_method_id = cmm::detail::hash_entity(const_method);
    inline static constexpr std::array<cmm::info, 2> member_ids{{mutable_method_id, const_method_id}};
    inline static constexpr std::array<cmm::info, 2> function_ids{{mutable_method_id, const_method_id}};
    inline static constexpr std::array<std::pair<std::string_view, cmm::info>, 2> member_names{{
        {std::meta::identifier_of(mutable_method), mutable_method_id},
        {std::meta::identifier_of(const_method), const_method_id},
    }};
};

template <std::meta::info ClassRefl>
consteval auto make_overloaded_method_static_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    constexpr std::meta::info mutable_method = OverloadedMethodStaticMetadata<ClassRefl>::mutable_method;
    constexpr std::meta::info const_method = OverloadedMethodStaticMetadata<ClassRefl>::const_method;
    static constexpr auto mutable_parameters = std::define_static_array(std::meta::parameters_of(mutable_method));
    static constexpr auto const_parameters = std::define_static_array(std::meta::parameters_of(const_method));
    static_assert(mutable_parameters.empty());
    static_assert(const_parameters.empty());

    constexpr std::meta::info MutableReturnRefl = std::meta::return_type_of(mutable_method);
    constexpr std::meta::info ConstReturnRefl = std::meta::return_type_of(const_method);
    static_assert(cmm::detail::hash_entity(MutableReturnRefl) == cmm::detail::hash_entity(ConstReturnRefl));

    using ClassT = typename[:ClassRefl:];
    using ReturnT = typename[:MutableReturnRefl:];

    const cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    const cmm::info mutable_method_id = cmm::detail::hash_entity(mutable_method);
    const cmm::info const_method_id = cmm::detail::hash_entity(const_method);
    const cmm::info return_type_id = cmm::detail::hash_entity(MutableReturnRefl);

    Class cls(std::meta::display_string_of(ClassRefl));
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    cls.set_members(OverloadedMethodStaticMetadata<ClassRefl>::member_ids);
    cls.set_functions(OverloadedMethodStaticMetadata<ClassRefl>::function_ids);
    cls.set_member_names(OverloadedMethodStaticMetadata<ClassRefl>::member_names);

    Function mutable_fn(std::meta::identifier_of(mutable_method), true, false);
    mutable_fn.set_display_name(std::meta::display_string_of(mutable_method));
    mutable_fn.set_parent_id(class_id);
    mutable_fn.set_return_type_id(return_type_id);
    mutable_fn.set_is_const_member_function(false);
    mutable_fn.set_thunk(cmm::detail::create_thunk<mutable_method>());

    Function const_fn(std::meta::identifier_of(const_method), true, false);
    const_fn.set_display_name(std::meta::display_string_of(const_method));
    const_fn.set_parent_id(class_id);
    const_fn.set_return_type_id(return_type_id);
    const_fn.set_is_const_member_function(true);
    const_fn.set_thunk(cmm::detail::create_thunk<const_method>());

    Type return_type(std::meta::display_string_of(MutableReturnRefl));
    if constexpr (!std::is_reference_v<ReturnT> &&
                  !std::is_void_v<ReturnT> &&
                  !std::is_function_v<ReturnT>)
    {
        return_type.set_size(sizeof(ReturnT));
        return_type.set_alignment(alignof(ReturnT));
    }

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 4> entities{{
        {class_id, cls},
        {mutable_method_id, mutable_fn},
        {const_method_id, const_fn},
        {return_type_id, return_type},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_OVERLOADED_METHOD_REGISTRY_HPP
