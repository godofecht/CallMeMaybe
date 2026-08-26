#ifndef CALLMEMAYBE_STATIC_LIFECYCLE_REGISTRY_HPP
#define CALLMEMAYBE_STATIC_LIFECYCLE_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <meta>
#include <utility>

#include "cmm/annotations.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_registry_view.hpp"

namespace cmm::detail {

template <std::meta::info ClassRefl>
struct StaticLifecycleMetadata
{
    inline static constexpr auto reflected_lifecycle = []() consteval
    {
        static constexpr auto members = std::define_static_array(
            std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()));

        std::meta::info ctor{};
        std::meta::info dtor{};
        std::size_t ctor_count = 0;
        std::size_t dtor_count = 0;

        template for (constexpr std::meta::info member : members)
        {
            if (std::meta::is_constructor(member) &&
                cmm::is_reflectable(member) &&
                std::meta::parameters_of(member).empty())
            {
                ctor = member;
                ++ctor_count;
            }
            else if (std::meta::is_destructor(member))
            {
                dtor = member;
                ++dtor_count;
            }
        }

        if (ctor_count != 1) throw "static lifecycle registry requires exactly one reflected zero-argument constructor";
        if (dtor_count != 1) throw "static lifecycle registry requires exactly one destructor";
        return std::pair{ctor, dtor};
    }();

    inline static constexpr std::meta::info constructor = reflected_lifecycle.first;
    inline static constexpr std::meta::info destructor = reflected_lifecycle.second;
    inline static constexpr cmm::info constructor_id = cmm::detail::hash_entity(constructor);
    inline static constexpr cmm::info destructor_id = cmm::detail::hash_entity(destructor);
    inline static constexpr std::array<cmm::info, 2> member_ids{{constructor_id, destructor_id}};
    inline static constexpr std::array<cmm::info, 1> constructor_ids{{constructor_id}};
};

template <std::meta::info ClassRefl>
consteval auto make_static_lifecycle_registry()
{
    static_assert(std::meta::is_class_type(ClassRefl) || std::meta::is_union_type(ClassRefl));

    constexpr std::meta::info ctor_refl = StaticLifecycleMetadata<ClassRefl>::constructor;
    constexpr std::meta::info dtor_refl = StaticLifecycleMetadata<ClassRefl>::destructor;
    using ClassT = typename[:ClassRefl:];

    const cmm::info class_id = cmm::detail::hash_entity(ClassRefl);
    const cmm::info ctor_id = StaticLifecycleMetadata<ClassRefl>::constructor_id;
    const cmm::info dtor_id = StaticLifecycleMetadata<ClassRefl>::destructor_id;

    Class cls(std::meta::display_string_of(ClassRefl));
    cls.set_size(sizeof(ClassT));
    cls.set_alignment(alignof(ClassT));
    cls.set_members(StaticLifecycleMetadata<ClassRefl>::member_ids);
    cls.set_constructors(StaticLifecycleMetadata<ClassRefl>::constructor_ids);
    cls.set_destructor(dtor_id);

    Function ctor(std::meta::display_string_of(ctor_refl), true, false);
    ctor.set_display_name(std::meta::display_string_of(ctor_refl));
    ctor.set_parent_id(class_id);
    ctor.set_is_constructor(true);
    ctor.set_thunk(cmm::detail::create_constructor_thunk<ctor_refl>());

    Function dtor(std::meta::display_string_of(dtor_refl), true, false);
    dtor.set_display_name(std::meta::display_string_of(dtor_refl));
    dtor.set_parent_id(class_id);
    dtor.set_is_destructor(true);
    dtor.set_thunk(cmm::detail::create_destructor_thunk<dtor_refl>());

    std::array<std::pair<cmm::info, RegistryEntityVariant>, 3> entities{{
        {class_id, cls},
        {ctor_id, ctor},
        {dtor_id, dtor},
    }};
    return make_static_registry_data(entities);
}

} // namespace cmm::detail

#endif // CALLMEMAYBE_STATIC_LIFECYCLE_REGISTRY_HPP
