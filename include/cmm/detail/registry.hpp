#ifndef CALLMEMAYBE_REGISTRY_HPP
#define CALLMEMAYBE_REGISTRY_HPP

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <meta>

#include "cmm/annotations.hpp"
#include "cmm/info.hpp"
#include "cmm/error.hpp"
#include "cmm/detail/hash/info_hash.hpp"
#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/data_member.hpp"
#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/enumerator.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/entities/variable.hpp"
#include "cmm/detail/invocation/thunk.hpp"
#include "cmm/detail/static_function_enum_metadata.hpp"

#ifdef CMM_ENABLE_REGISTRY_LOGS
    #define CMM_REG_LOG(x) do { std::cout << x; } while(0)
#else
    #define CMM_REG_LOG(x) do {} while(0)
#endif

namespace cmm {
namespace detail {

consteval bool is_registerable_entity(std::meta::info entity) {
    return std::meta::is_function(entity) ||
           std::meta::is_variable(entity) ||
           std::meta::is_type(entity);
}

class Registry {
public:
    using EntityVariant = std::variant<Type,
                                       Class,
                                       Enum,
                                       Variable,
                                       DataMember,
                                       Function,
                                       Parameter,
                                       Enumerator,
                                       Base>;

    static Registry& instance() {
        static Registry inst;
        return inst;
    }

    template <std::meta::info EntityRefl>
    requires (is_registerable_entity(EntityRefl))
    cmm::Error register_entity() {
        std::lock_guard<std::recursive_mutex> lock(registration_mutex_);

        if (frozen_.load(std::memory_order_acquire)) {
            return cmm::Error::RegistryFrozen;
        }

        const cmm::info id = cmm::detail::hash_entity(EntityRefl);
        if (fully_registered_.contains(id)) return cmm::Error::Success;
        if (registering_.contains(id)) return cmm::Error::Success;

        registering_.insert(id);
        CMM_REG_LOG("Registering entity: " << std::meta::display_string_of(EntityRefl) << "\n");

        cmm::Error result = cmm::Error::EntityNotFound;

        if constexpr (std::meta::is_function(EntityRefl)) {
            result = register_free_function<EntityRefl>(id);
        } else if constexpr (std::meta::is_variable(EntityRefl)) {
            result = register_variable<EntityRefl>(id);
        } else if constexpr (std::meta::is_class_type(EntityRefl) || std::meta::is_union_type(EntityRefl)) {
            result = register_class<EntityRefl>(id);
        } else if constexpr (std::meta::is_enum_type(EntityRefl)) {
            result = register_enum<EntityRefl>(id);
        } else if constexpr (std::meta::is_type(EntityRefl)) {
            ensure_type_registered<EntityRefl>();
            result = cmm::Error::Success;
        }

        registering_.erase(id);
        if (result == cmm::Error::Success) fully_registered_.insert(id);
        return result;
    }

    cmm::info get_id_by_name(std::string_view name) const {
        freeze();
        if (auto it = top_level_entities_.find(name); it != top_level_entities_.end()) {
            return it->second;
        }
        return cmm::invalid_info;
    }

    bool contains(cmm::info id) const {
        freeze();
        return entity_registry_.contains(id);
    }

    EntityVariant& get_entity(cmm::info id) {
        freeze();
        return entity_registry_.at(id);
    }

    const EntityVariant& get_entity(cmm::info id) const {
        freeze();
        return entity_registry_.at(id);
    }

    std::string_view get_entity_name(cmm::info id) const {
        freeze();
        if (id == cmm::invalid_info) return {};
        auto it = entity_registry_.find(id);
        if (it == entity_registry_.end()) return {};
        return std::visit([](auto&& arg) -> std::string_view { return arg.name(); }, it->second);
    }

    std::string_view get_entity_display_name(cmm::info id) const {
        freeze();
        if (id == cmm::invalid_info) return {};
        auto it = entity_registry_.find(id);
        if (it == entity_registry_.end()) return {};
        return std::visit([](auto&& arg) -> std::string_view { return arg.display_name(); }, it->second);
    }

    cmm::Error adjust_instance_pointer(cmm::info actual_class_id,
                                       cmm::info target_class_id,
                                       const void* instance,
                                       const void*& out) const {
        freeze();
        if (!instance) return cmm::Error::NullValue;
        if (actual_class_id == target_class_id) {
            out = instance;
            return cmm::Error::Success;
        }

        const void* candidate = nullptr;
        bool found = false;
        bool ambiguous = false;
        collect_base_adjustments(actual_class_id, target_class_id, instance,
                                 candidate, found, ambiguous);
        if (!found || ambiguous) return cmm::Error::InvalidArgumentType;

        out = candidate;
        return cmm::Error::Success;
    }

private:
    struct TransparentStringHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
    };

    mutable std::atomic<bool> frozen_{false};
    std::recursive_mutex registration_mutex_;
    std::unordered_map<cmm::info, EntityVariant> entity_registry_;
    std::unordered_map<std::string, cmm::info, TransparentStringHash, std::equal_to<>> top_level_entities_;
    std::unordered_set<cmm::info> fully_registered_;
    std::unordered_set<cmm::info> registering_;

    void freeze() const {
        frozen_.store(true, std::memory_order_release);
    }

    void collect_base_adjustments(cmm::info current_class_id,
                                  cmm::info target_class_id,
                                  const void* current_instance,
                                  const void*& candidate,
                                  bool& found,
                                  bool& ambiguous) const {
        if (ambiguous) return;

        auto class_it = entity_registry_.find(current_class_id);
        if (class_it == entity_registry_.end()) return;
        const auto* cls = std::get_if<Class>(&class_it->second);
        if (!cls) return;

        for (cmm::info base_id : cls->bases()) {
            auto base_it = entity_registry_.find(base_id);
            if (base_it == entity_registry_.end()) continue;
            const auto* base = std::get_if<Base>(&base_it->second);
            if (!base || !base->is_runtime_accessible()) continue;

            const void* base_instance = base->upcast(current_instance);
            if (!base_instance) continue;

            if (base->type_id() == target_class_id) {
                if (!found) {
                    candidate = base_instance;
                    found = true;
                } else if (candidate != base_instance) {
                    ambiguous = true;
                    return;
                }
            } else {
                collect_base_adjustments(base->type_id(), target_class_id,
                                         base_instance, candidate, found, ambiguous);
                if (ambiguous) return;
            }
        }
    }

    void add_lookup_name(std::string_view name, cmm::info id) {
        if (name.empty()) return;

        auto [it, inserted] = top_level_entities_.emplace(std::string(name), id);
        if (!inserted && it->second != id) it->second = cmm::invalid_info;
    }

    template <typename EntityT>
    cmm::Error insert_unique(cmm::info id, EntityT&& entity) {
        auto [_, inserted] = entity_registry_.emplace(id, std::forward<EntityT>(entity));
        return inserted ? cmm::Error::Success : cmm::Error::EntityIdCollision;
    }

    template <std::meta::info TypeRefl>
    cmm::info ensure_type_registered() {
        const cmm::info id = cmm::detail::hash_entity(TypeRefl);
        if (entity_registry_.contains(id)) return id;

        if constexpr (std::meta::is_class_type(TypeRefl) || std::meta::is_union_type(TypeRefl)) {
            entity_registry_.emplace(id, make_class_stub<TypeRefl>());
        } else if constexpr (std::meta::is_enum_type(TypeRefl)) {
            entity_registry_.emplace(id, make_enum_stub<TypeRefl>());
        } else {
            entity_registry_.emplace(id, make_type<TypeRefl>());
        }

        if constexpr (std::meta::is_fundamental_type(TypeRefl) ||
                      std::meta::is_enum_type(TypeRefl) ||
                      std::meta::is_class_type(TypeRefl) ||
                      std::meta::is_union_type(TypeRefl)) {
            add_lookup_name(std::meta::display_string_of(canonicalize_type(TypeRefl)), id);
        }

        return id;
    }

    template <std::meta::info TypeRefl>
    Type make_type() {
        Type t(std::meta::display_string_of(TypeRefl));
        using T = typename[:TypeRefl:];
        using DecayedT = std::remove_cvref_t<T>;

        if constexpr (!std::is_void_v<DecayedT> &&
                      !std::is_function_v<DecayedT> &&
                      !std::is_reference_v<T>) {
            t.set_size(sizeof(T));
            t.set_alignment(alignof(T));
        }
        t.set_flags(make_type_flags<TypeRefl>());

        if constexpr (std::meta::is_pointer_type(TypeRefl)) {
            t.set_underlying_type_id(ensure_type_registered<std::meta::remove_pointer(TypeRefl)>());
        } else if constexpr (std::meta::is_reference_type(TypeRefl)) {
            t.set_underlying_type_id(ensure_type_registered<std::meta::remove_reference(TypeRefl)>());
        } else if constexpr (std::meta::is_array_type(TypeRefl)) {
            t.set_underlying_type_id(ensure_type_registered<std::meta::remove_extent(TypeRefl)>());
            if constexpr (std::meta::is_bounded_array_type(TypeRefl)) {
                t.set_array_extent(std::meta::extent(TypeRefl, 0));
            }
        }
        return t;
    }

    template <std::meta::info ClassRefl>
    static Class make_class_stub() {
        Class cls(std::meta::display_string_of(ClassRefl));
        using T = typename[:ClassRefl:];
        if constexpr (!std::is_void_v<T>) {
            cls.set_size(sizeof(T));
            cls.set_alignment(alignof(T));
        }
        cls.set_flags(make_type_flags<ClassRefl>());
        return cls;
    }

    template <std::meta::info EnumRefl>
    static Enum make_enum_stub() {
        Enum e(std::meta::display_string_of(EnumRefl));
        using T = typename[:EnumRefl:];
        e.set_size(sizeof(T));
        e.set_alignment(alignof(T));
        e.set_flags(make_type_flags<EnumRefl>());
        return e;
    }

    template <std::meta::info TypeRefl>
    static TypeFlags make_type_flags() {
        TypeFlags flags{};
        flags.is_void = std::meta::is_void_type(TypeRefl);
        flags.is_null_pointer = std::meta::is_null_pointer_type(TypeRefl);
        flags.is_integral = std::meta::is_integral_type(TypeRefl);
        flags.is_floating_point = std::meta::is_floating_point_type(TypeRefl);
        flags.is_arithmetic = std::meta::is_arithmetic_type(TypeRefl);
        flags.is_fundamental = std::meta::is_fundamental_type(TypeRefl);
        flags.is_pointer = std::meta::is_pointer_type(TypeRefl);
        flags.is_lvalue_reference = std::meta::is_lvalue_reference_type(TypeRefl);
        flags.is_rvalue_reference = std::meta::is_rvalue_reference_type(TypeRefl);
        flags.is_reference = std::meta::is_reference_type(TypeRefl);
        flags.is_class = std::meta::is_class_type(TypeRefl);
        flags.is_union = std::meta::is_union_type(TypeRefl);
        flags.is_enum = std::meta::is_enum_type(TypeRefl);
        flags.is_scoped_enum = std::meta::is_scoped_enum_type(TypeRefl);
        flags.is_array = std::meta::is_array_type(TypeRefl);
        flags.is_function_type = std::meta::is_function_type(TypeRefl);
        flags.is_const = std::meta::is_const_type(TypeRefl);
        flags.is_volatile = std::meta::is_volatile_type(TypeRefl);
        flags.is_signed = std::meta::is_signed_type(TypeRefl);
        flags.is_unsigned = std::meta::is_unsigned_type(TypeRefl);
        return flags;
    }

    template <std::meta::info FuncRefl>
    cmm::Error register_free_function(cmm::info id) {
        Function func(std::meta::identifier_of(FuncRefl));
        func.set_display_name(std::meta::display_string_of(FuncRefl));
        if (cmm::Error err = register_function_signature<FuncRefl>(func, id);
            err != cmm::Error::Success) return err;
        func.set_thunk(cmm::detail::create_thunk<FuncRefl>());

        if (cmm::Error err = insert_unique(id, std::move(func));
            err != cmm::Error::Success) return err;

        add_lookup_name(std::meta::display_string_of(FuncRefl), id);
        add_lookup_name(std::meta::identifier_of(FuncRefl), id);
        return cmm::Error::Success;
    }

    template <std::meta::info FuncRefl>
    cmm::Error register_function_signature(Function& func, cmm::info func_id) {
        if constexpr (!std::meta::is_constructor(FuncRefl) && !std::meta::is_destructor(FuncRefl)) {
            constexpr std::meta::info ret_type_refl = std::meta::return_type_of(FuncRefl);
            func.set_return_type_id(ensure_type_registered<ret_type_refl>());
        }

        StaticFunctionMetadata<FuncRefl>::apply(func);
        std::size_t idx = 0;
        cmm::Error result = cmm::Error::Success;
        template for (constexpr std::meta::info p : std::define_static_array(std::meta::parameters_of(FuncRefl))) {
            if (result == cmm::Error::Success) {
                result = register_parameter<FuncRefl, p>(func_id, idx);
            }
            ++idx;
        }
        return result;
    }

    template <std::meta::info FuncRefl, std::meta::info ParamRefl>
    cmm::Error register_parameter(cmm::info func_id, std::size_t idx) {
        constexpr std::meta::info p_type_refl = std::meta::type_of(ParamRefl);
        const cmm::info p_type_id = ensure_type_registered<p_type_refl>();
        constexpr std::meta::info p_decayed_refl = std::meta::remove_cvref(p_type_refl);
        const cmm::info p_decayed_id = ensure_type_registered<p_decayed_refl>();

        const cmm::info p_id = cmm::detail::hash_entity(ParamRefl);
        std::string_view p_name;
        if constexpr (std::meta::has_identifier(ParamRefl)) p_name = std::meta::identifier_of(ParamRefl);

        Parameter p(p_name, p_type_id, func_id, idx);
        p.set_display_name(std::meta::display_string_of(ParamRefl));
        p.set_decayed_type_id(p_decayed_id);

        return insert_unique(p_id, std::move(p));
    }

    template <std::meta::info VarRefl>
    cmm::Error register_variable(cmm::info var_id) {
        constexpr std::meta::info var_type_refl = std::meta::type_of(VarRefl);
        const cmm::info type_id = ensure_type_registered<var_type_refl>();
        constexpr bool is_const = std::meta::is_const_type(var_type_refl);

        Variable var(std::meta::identifier_of(VarRefl), type_id);
        var.set_display_name(std::meta::display_string_of(VarRefl));
        var.set_is_const(is_const);

        constexpr void* var_address = const_cast<void*>(static_cast<const void*>(&[:VarRefl:]));
        var.set_address(var_address);

        using VarT = std::remove_cvref_t<typename[:var_type_refl:]>;
        var.set_getter_thunk(&cmm::detail::StaticThunks<VarT>::get);
        if constexpr (is_const) {
            var.set_ref_getter_thunk(&cmm::detail::StaticThunks<VarT>::get_cref);
        } else {
            var.set_ref_getter_thunk(&cmm::detail::StaticThunks<VarT>::get_ref);
            var.set_setter_thunk(&cmm::detail::StaticThunks<VarT>::set);
        }

        if (cmm::Error err = insert_unique(var_id, std::move(var)); err != cmm::Error::Success) return err;
        add_lookup_name(std::meta::display_string_of(VarRefl), var_id);
        add_lookup_name(std::meta::identifier_of(VarRefl), var_id);
        return cmm::Error::Success;
    }

    template <std::meta::info EnumRefl>
    cmm::Error register_enum(cmm::info enum_id) {
        ensure_type_registered<EnumRefl>();

        auto& e = std::get<Enum>(entity_registry_.at(enum_id));
        constexpr std::meta::info underlying = std::meta::underlying_type(EnumRefl);
        e.set_underlying_type_id(ensure_type_registered<underlying>());
        StaticEnumMetadata<EnumRefl>::apply(e);

        std::size_t idx = 0;
        cmm::Error result = cmm::Error::Success;
        template for (constexpr std::meta::info enumerator : std::define_static_array(std::meta::enumerators_of(EnumRefl))) {
            if (result != cmm::Error::Success) continue;

            const auto& entry = StaticEnumMetadata<EnumRefl>::entries[idx++];
            Enumerator en(entry.name, entry.value_bits, entry.is_signed);
            en.set_display_name(std::meta::display_string_of(enumerator));
            en.set_parent_id(enum_id);
            result = insert_unique(entry.entity_id, std::move(en));
        }

        if (result != cmm::Error::Success) return result;
        add_lookup_name(std::meta::display_string_of(EnumRefl), enum_id);
        return cmm::Error::Success;
    }

    template <std::meta::info ClassRefl>
    cmm::Error register_class(cmm::info class_id) {
        ensure_type_registered<ClassRefl>();
        Class cls = make_class_stub<ClassRefl>();

        cmm::Error result = cmm::Error::Success;
        template for (constexpr std::meta::info base : std::define_static_array(
                          std::meta::bases_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (result != cmm::Error::Success) continue;

            constexpr std::meta::info base_type_refl = std::meta::type_of(base);
            result = register_entity<base_type_refl>();
            if (result != cmm::Error::Success) continue;

            const cmm::info base_type_id = ensure_type_registered<base_type_refl>();
            const cmm::info base_id = cmm::detail::hash_entity(base);

            Base base_entity(std::meta::display_string_of(base_type_refl), base_type_id, class_id);
            base_entity.set_display_name(std::meta::display_string_of(base));
            base_entity.set_is_virtual(std::meta::is_virtual(base));

            if constexpr (std::meta::is_public(base)) {
                base_entity.set_access(Access::Public);
                using DerivedT = typename[:ClassRefl:];
                using BaseT = typename[:base_type_refl:];
                base_entity.set_upcast_thunk(+[](const void* instance) -> const void* {
                    return static_cast<const BaseT*>(static_cast<const DerivedT*>(instance));
                });
            } else if constexpr (std::meta::is_protected(base)) {
                base_entity.set_access(Access::Protected);
            } else {
                base_entity.set_access(Access::Private);
            }

            result = insert_unique(base_id, std::move(base_entity));
            if (result == cmm::Error::Success) cls.add_base(base_id);
        }

        template for (constexpr std::meta::info member : std::define_static_array(
                          std::meta::members_of(ClassRefl, std::meta::access_context::unchecked()))) {
            if (result != cmm::Error::Success) continue;
            if constexpr (!cmm::is_reflectable(member) && !std::meta::is_destructor(member)) continue;

            const cmm::info member_id = cmm::detail::hash_entity(member);
            if constexpr (std::meta::has_identifier(member)) cls.add_member_name(std::meta::identifier_of(member), member_id);
            cls.add_member(member_id);

            if constexpr (std::meta::is_nonstatic_data_member(member)) {
                constexpr std::meta::info mem_type_refl = std::meta::type_of(member);
                constexpr bool is_const = std::meta::is_const_type(mem_type_refl);
                const cmm::info mem_type_id = ensure_type_registered<mem_type_refl>();

                DataMember dm(std::meta::identifier_of(member), false);
                dm.set_display_name(std::meta::display_string_of(member));
                dm.set_type_id(mem_type_id);
                dm.set_parent_id(class_id);
                dm.set_offset_bytes(std::meta::offset_of(member).bytes);
                dm.set_offset_bits(std::meta::offset_of(member).bits);
                dm.set_is_bit_field(std::meta::is_bit_field(member));
                dm.set_is_const(is_const);

                using MemT = std::remove_cvref_t<typename[:mem_type_refl:]>;
                dm.set_getter_thunk(&cmm::detail::PropertyThunks<MemT>::get);
                if constexpr (is_const) {
                    dm.set_ref_getter_thunk(&cmm::detail::PropertyThunks<MemT>::get_cref);
                } else {
                    dm.set_ref_getter_thunk(&cmm::detail::PropertyThunks<MemT>::get_ref);
                    dm.set_setter_thunk(&cmm::detail::PropertyThunks<MemT>::set);
                }

                result = insert_unique(member_id, std::move(dm));
                if (result == cmm::Error::Success) cls.add_nonstatic_data_member(member_id);
            } else if constexpr (std::meta::is_constructor(member)) {
                Function ctor(std::meta::display_string_of(member), true, false);
                ctor.set_display_name(std::meta::display_string_of(member));
                ctor.set_is_constructor(true);
                ctor.set_parent_id(class_id);
                result = register_function_signature<member>(ctor, member_id);
                if (result == cmm::Error::Success) {
                    ctor.set_thunk(cmm::detail::create_constructor_thunk<member>());
                    result = insert_unique(member_id, std::move(ctor));
                }
                if (result == cmm::Error::Success) cls.add_constructor(member_id);
            } else if constexpr (std::meta::is_destructor(member)) {
                Function dtor(std::meta::display_string_of(member), true, false);
                dtor.set_display_name(std::meta::display_string_of(member));
                dtor.set_is_destructor(true);
                dtor.set_parent_id(class_id);
                dtor.set_thunk(cmm::detail::create_destructor_thunk<member>());
                result = insert_unique(member_id, std::move(dtor));
                if (result == cmm::Error::Success) cls.set_destructor(member_id);
            } else if constexpr (std::meta::is_function(member)) {
                constexpr bool is_static = std::meta::is_static_member(member);
                std::string_view fn_name;
                if constexpr (std::meta::has_identifier(member)) fn_name = std::meta::identifier_of(member);
                else fn_name = std::meta::display_string_of(member);

                Function fn(fn_name, true, is_static);
                fn.set_display_name(std::meta::display_string_of(member));
                fn.set_parent_id(class_id);
                if constexpr (!is_static) fn.set_is_const_member_function(std::meta::is_const(member));
                result = register_function_signature<member>(fn, member_id);
                if (result == cmm::Error::Success) {
                    fn.set_thunk(cmm::detail::create_thunk<member>());
                    result = insert_unique(member_id, std::move(fn));
                }
                if (result == cmm::Error::Success) cls.add_function(member_id);
            } else if constexpr (std::meta::is_static_member(member)) {
                constexpr std::meta::info mem_type_refl = std::meta::type_of(member);
                constexpr bool is_const = std::meta::is_const_type(mem_type_refl);
                const cmm::info mem_type_id = ensure_type_registered<mem_type_refl>();

                DataMember dm(std::meta::identifier_of(member), true);
                dm.set_display_name(std::meta::display_string_of(member));
                dm.set_type_id(mem_type_id);
                dm.set_parent_id(class_id);
                dm.set_is_const(is_const);

                constexpr void* mem_address = const_cast<void*>(static_cast<const void*>(&[:member:]));
                dm.set_address(mem_address);

                using MemT = std::remove_cvref_t<typename[:mem_type_refl:]>;
                dm.set_static_getter_thunk(&cmm::detail::StaticThunks<MemT>::get);
                if constexpr (is_const) {
                    dm.set_static_ref_getter_thunk(&cmm::detail::StaticThunks<MemT>::get_cref);
                } else {
                    dm.set_static_ref_getter_thunk(&cmm::detail::StaticThunks<MemT>::get_ref);
                    dm.set_static_setter_thunk(&cmm::detail::StaticThunks<MemT>::set);
                }

                result = insert_unique(member_id, std::move(dm));
                if (result == cmm::Error::Success) cls.add_static_data_member(member_id);
            }
        }

        if (result != cmm::Error::Success) return result;

        entity_registry_.insert_or_assign(class_id, std::move(cls));
        add_lookup_name(std::meta::display_string_of(ClassRefl), class_id);
        return cmm::Error::Success;
    }
};

} // namespace detail
} // namespace cmm

#endif // CALLMEMAYBE_REGISTRY_HPP