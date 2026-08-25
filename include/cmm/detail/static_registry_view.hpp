#ifndef CALLMEMAYBE_STATIC_REGISTRY_VIEW_HPP
#define CALLMEMAYBE_STATIC_REGISTRY_VIEW_HPP

#include <array>
#include <cstddef>
#include <cstdlib>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

#include "cmm/error.hpp"
#include "cmm/info.hpp"
#include "cmm/detail/entities/base.hpp"
#include "cmm/detail/entities/class.hpp"
#include "cmm/detail/entities/data_member.hpp"
#include "cmm/detail/entities/enum.hpp"
#include "cmm/detail/entities/enumerator.hpp"
#include "cmm/detail/entities/function.hpp"
#include "cmm/detail/entities/parameter.hpp"
#include "cmm/detail/entities/type.hpp"
#include "cmm/detail/entities/variable.hpp"

namespace cmm::detail {

using RegistryEntityVariant = std::variant<Type,
                                           Class,
                                           Enum,
                                           Variable,
                                           DataMember,
                                           Function,
                                           Parameter,
                                           Enumerator,
                                           Base>;

template <std::size_t EntityCount, std::size_t NameCount>
struct StaticRegistryData {
    std::array<std::pair<cmm::info, RegistryEntityVariant>, EntityCount> entities{};
    std::array<std::pair<std::string_view, cmm::info>, NameCount> names{};
};

template <typename T, std::size_t N, typename KeyFn>
consteval void insertion_sort(std::array<T, N>& values, KeyFn key)
{
    for (std::size_t i = 1; i < N; ++i)
    {
        T value = values[i];
        std::size_t j = i;
        while (j > 0 && key(value) < key(values[j - 1]))
        {
            values[j] = values[j - 1];
            --j;
        }
        values[j] = value;
    }
}

constexpr std::string_view registry_entity_name(const RegistryEntityVariant& entity)
{
    return std::visit([](const auto& value) -> std::string_view { return value.name(); }, entity);
}

constexpr std::string_view registry_entity_display_name(const RegistryEntityVariant& entity)
{
    return std::visit([](const auto& value) -> std::string_view { return value.display_name(); }, entity);
}

template <std::size_t EntityCount>
constexpr bool static_registry_has_duplicate_ids(
    const std::array<std::pair<cmm::info, RegistryEntityVariant>, EntityCount>& entities)
{
    for (std::size_t i = 0; i < EntityCount; ++i)
    {
        for (std::size_t j = i + 1; j < EntityCount; ++j)
        {
            if (entities[i].first == entities[j].first) return true;
        }
    }
    return false;
}

template <std::size_t EntityCount>
consteval void validate_static_registry_entity_ids(
    const std::array<std::pair<cmm::info, RegistryEntityVariant>, EntityCount>& entities)
{
    if (static_registry_has_duplicate_ids(entities)) std::abort();
}

template <std::size_t EntityCount, std::size_t NameCount>
consteval StaticRegistryData<EntityCount, NameCount> make_static_registry_data(
    std::array<std::pair<cmm::info, RegistryEntityVariant>, EntityCount> entities,
    std::array<std::pair<std::string_view, cmm::info>, NameCount> names)
{
    validate_static_registry_entity_ids(entities);
    insertion_sort(entities, [](const auto& entry) { return entry.first; });
    insertion_sort(names, [](const auto& entry) { return entry.first; });
    return StaticRegistryData<EntityCount, NameCount>{entities, names};
}

template <std::size_t EntityCount>
consteval StaticRegistryData<EntityCount, EntityCount> make_static_registry_data(
    std::array<std::pair<cmm::info, RegistryEntityVariant>, EntityCount> entities)
{
    validate_static_registry_entity_ids(entities);

    std::array<std::pair<std::string_view, cmm::info>, EntityCount> names{};
    for (std::size_t i = 0; i < EntityCount; ++i)
    {
        names[i] = {registry_entity_name(entities[i].second), entities[i].first};
    }

    insertion_sort(entities, [](const auto& entry) { return entry.first; });
    insertion_sort(names, [](const auto& entry) { return entry.first; });

    for (std::size_t i = 0; i < EntityCount;)
    {
        std::size_t end = i + 1;
        while (end < EntityCount && names[end].first == names[i].first) ++end;
        if (end - i > 1)
        {
            for (std::size_t j = i; j < end; ++j) names[j].second = cmm::invalid_info;
        }
        i = end;
    }

    return StaticRegistryData<EntityCount, EntityCount>{entities, names};
}

class RegistryView {
public:
    constexpr RegistryView() = default;

    template <std::size_t EntityCount, std::size_t NameCount>
    constexpr explicit RegistryView(const StaticRegistryData<EntityCount, NameCount>& data) noexcept
        : entities_(data.entities), names_(data.names)
    {
    }

    constexpr bool contains(cmm::info id) const
    {
        return find_entity(id) != entities_.end();
    }

    constexpr const RegistryEntityVariant* try_get_entity(cmm::info id) const
    {
        const auto it = find_entity(id);
        return it == entities_.end() ? nullptr : &it->second;
    }

    constexpr const RegistryEntityVariant& get_entity(cmm::info id) const
    {
        return *try_get_entity(id);
    }

    template <typename EntityT>
    constexpr const EntityT* try_get_as(cmm::info id) const
    {
        const RegistryEntityVariant* entity = try_get_entity(id);
        return entity ? std::get_if<EntityT>(entity) : nullptr;
    }

    constexpr std::string_view get_entity_name(cmm::info id) const
    {
        const RegistryEntityVariant* entity = try_get_entity(id);
        if (!entity) return {};
        return registry_entity_name(*entity);
    }

    constexpr std::string_view get_entity_display_name(cmm::info id) const
    {
        const RegistryEntityVariant* entity = try_get_entity(id);
        if (!entity) return {};
        return registry_entity_display_name(*entity);
    }

    constexpr cmm::info get_id_by_name(std::string_view name) const
    {
        std::size_t first = 0;
        std::size_t count = names_.size();
        while (count != 0)
        {
            const std::size_t step = count / 2;
            const std::size_t index = first + step;
            if (names_[index].first < name)
            {
                first = index + 1;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }

        if (first < names_.size() && names_[first].first == name) return names_[first].second;
        return cmm::invalid_info;
    }

    cmm::Error adjust_instance_pointer(cmm::info actual_class_id,
                                       cmm::info target_class_id,
                                       const void* instance,
                                       const void*& out) const
    {
        if (!instance) return cmm::Error::NullValue;
        if (actual_class_id == target_class_id)
        {
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

    constexpr std::size_t entity_count() const { return entities_.size(); }
    constexpr std::size_t name_count() const { return names_.size(); }

private:
    using EntityEntry = std::pair<cmm::info, RegistryEntityVariant>;

    void collect_base_adjustments(cmm::info current_class_id,
                                  cmm::info target_class_id,
                                  const void* current_instance,
                                  const void*& candidate,
                                  bool& found,
                                  bool& ambiguous) const
    {
        if (ambiguous) return;

        const Class* cls = try_get_as<Class>(current_class_id);
        if (!cls) return;

        for (cmm::info base_id : cls->bases())
        {
            const Base* base = try_get_as<Base>(base_id);
            if (!base || !base->is_runtime_accessible()) continue;

            const void* base_instance = base->upcast(current_instance);
            if (!base_instance) continue;

            if (base->type_id() == target_class_id)
            {
                if (!found)
                {
                    candidate = base_instance;
                    found = true;
                }
                else if (candidate != base_instance)
                {
                    ambiguous = true;
                    return;
                }
            }
            else
            {
                collect_base_adjustments(base->type_id(), target_class_id,
                                         base_instance, candidate, found, ambiguous);
                if (ambiguous) return;
            }
        }
    }

    constexpr std::span<const EntityEntry>::iterator find_entity(cmm::info id) const
    {
        std::size_t first = 0;
        std::size_t count = entities_.size();
        while (count != 0)
        {
            const std::size_t step = count / 2;
            const std::size_t index = first + step;
            if (entities_[index].first < id)
            {
                first = index + 1;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }

        if (first < entities_.size() && entities_[first].first == id) return entities_.begin() + static_cast<std::ptrdiff_t>(first);
        return entities_.end();
    }

    std::span<const EntityEntry> entities_{};
    std::span<const std::pair<std::string_view, cmm::info>> names_{};
};

} // namespace cmm::detail

#endif
