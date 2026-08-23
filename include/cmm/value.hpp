#ifndef CMM_VALUE_HPP
#define CMM_VALUE_HPP

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <meta>

#include "cmm/error.hpp"
#include "cmm/info.hpp"
#include "cmm/detail/hash/info_hash.hpp"

namespace cmm {
namespace detail {

inline constexpr std::size_t SBO_SIZE = 32;
inline constexpr std::size_t SBO_ALIGN = alignof(std::max_align_t);

template <typename T>
inline constexpr bool UseSBO = (sizeof(T) <= SBO_SIZE) &&
                               (alignof(T) <= SBO_ALIGN) &&
                               std::is_nothrow_move_constructible_v<T>;

struct ValueOps {
    void (*destroy)(void* ptr);
    void* (*copy)(const void* src, void* inline_buffer);
    void* (*move)(void* src, void* inline_buffer);
    const void* (*object_pointer)(const void* src);
};

template <typename T>
void destroy_value(void* ptr) {
    if constexpr (UseSBO<T>) static_cast<T*>(ptr)->~T();
    else delete static_cast<T*>(ptr);
}

template <typename T>
void* copy_value(const void* src, void* inline_buffer) {
    if constexpr (!std::is_copy_constructible_v<T>) {
        return nullptr;
    } else if constexpr (UseSBO<T>) {
        return new (inline_buffer) T(*static_cast<const T*>(src));
    } else {
        return new T(*static_cast<const T*>(src));
    }
}

template <typename T>
void* move_value(void* src, void* inline_buffer) {
    if constexpr (UseSBO<T>) return new (inline_buffer) T(std::move(*static_cast<T*>(src)));
    return src;
}

template <typename T>
const void* object_pointer_value(const void* src) {
    if constexpr (std::is_pointer_v<T> &&
                  std::is_object_v<std::remove_cv_t<std::remove_pointer_t<T>>> &&
                  !std::is_volatile_v<std::remove_pointer_t<T>>) {
        const T pointer = *static_cast<const T*>(src);
        return static_cast<const void*>(pointer);
    }
    return nullptr;
}

template <typename T>
inline constexpr ValueOps value_ops = {
    &destroy_value<T>,
    std::is_copy_constructible_v<T> ? &copy_value<T> : nullptr,
    &move_value<T>,
    &object_pointer_value<T>
};

template <typename T>
inline constexpr ValueOps ref_ops = {
    [](void*) {},
    [](const void* src, void*) -> void* { return const_cast<void*>(src); },
    [](void* src, void*) -> void* { return src; },
    &object_pointer_value<T>
};

} // namespace detail

class Value {
public:
    enum class Policy : uint8_t {
        Owned,
        MutRef,
        ConstRef,
        RvalueRef
    };

    Value() = default;

    template <typename T, typename Decayed = std::decay_t<T>>
    requires (!std::is_same_v<Decayed, Value>)
    explicit Value(T&& val) {
        initialize_type_metadata<Decayed>();
        policy_ = Policy::Owned;
        ops_ = &detail::value_ops<Decayed>;

        if constexpr (detail::UseSBO<Decayed>) {
            data_ = new (buffer_) Decayed(std::forward<T>(val));
            is_inline_ = true;
        } else {
            data_ = new Decayed(std::forward<T>(val));
            is_inline_ = false;
        }
    }

    template <typename T>
    static Value ref(T& val) {
        if constexpr (std::is_const_v<T>) {
            return cref(val);
        } else {
            using Decayed = std::decay_t<T>;
            Value v;
            v.template initialize_type_metadata<Decayed>();
            v.policy_ = Policy::MutRef;
            v.data_ = static_cast<void*>(std::addressof(val));
            v.ops_ = &detail::ref_ops<Decayed>;
            return v;
        }
    }

    template <typename T>
    static Value cref(const T& val) {
        using Decayed = std::decay_t<T>;
        Value v;
        v.template initialize_type_metadata<Decayed>();
        v.policy_ = Policy::ConstRef;
        v.data_ = const_cast<void*>(static_cast<const void*>(std::addressof(val)));
        v.ops_ = &detail::ref_ops<Decayed>;
        return v;
    }

    template <typename T>
    static Value rref(T&& val) {
        using Decayed = std::decay_t<T>;
        Value v;
        v.template initialize_type_metadata<Decayed>();
        v.policy_ = Policy::RvalueRef;
        v.data_ = static_cast<void*>(std::addressof(val));
        v.ops_ = &detail::ref_ops<Decayed>;
        return v;
    }

    ~Value() { reset(); }

    Value(const Value& other) { copy_from(other); }

    Value& operator=(const Value& other) {
        if (this != &other) {
            Value replacement;
            if (other.try_copy_to(replacement) != cmm::Error::Success) std::abort();
            *this = std::move(replacement);
        }
        return *this;
    }

    Value(Value&& other) noexcept { move_from(std::move(other)); }

    Value& operator=(Value&& other) noexcept {
        if (this != &other) {
            reset();
            move_from(std::move(other));
        }
        return *this;
    }

    Policy policy() const { return policy_; }
    cmm::info type_id() const { return type_id_; }
    cmm::info pointee_type_id() const { return pointee_type_id_; }
    bool pointee_is_const() const { return pointee_is_const_; }
    bool has_value() const { return data_ != nullptr; }
    bool is_copyable() const noexcept { return !data_ || !ops_ || ops_->copy != nullptr; }

    cmm::Error try_copy_to(Value& out) const {
        if (!data_) {
            out.reset();
            return cmm::Error::Success;
        }
        if (!ops_ || !ops_->copy) return cmm::Error::NonCopyableValue;

        Value replacement;
        replacement.type_id_ = type_id_;
        replacement.pointee_type_id_ = pointee_type_id_;
        replacement.pointee_is_const_ = pointee_is_const_;
        replacement.policy_ = policy_;
        replacement.ops_ = ops_;
        replacement.is_inline_ = is_inline_;
        replacement.data_ = ops_->copy(data_, replacement.buffer_);
        if (!replacement.data_) return cmm::Error::NonCopyableValue;

        out = std::move(replacement);
        return cmm::Error::Success;
    }

    const void* data() const noexcept { return data_; }
    const void* object_pointer() const noexcept {
        return data_ && ops_ && ops_->object_pointer ? ops_->object_pointer(data_) : nullptr;
    }
    void* mutable_data() noexcept {
        if (policy_ == Policy::ConstRef) return nullptr;
        return data_;
    }

    template <typename T>
    T* get_if() noexcept {
        using Decayed = std::decay_t<T>;
        constexpr cmm::info req_id = cmm::detail::hash_entity(std::meta::decay(^^T));
        if (req_id != type_id_ || !data_) return nullptr;
        if constexpr (!std::is_const_v<T>) {
            if (policy_ == Policy::ConstRef) return nullptr;
        }
        return static_cast<Decayed*>(data_);
    }

    template <typename T>
    const T* get_if() const noexcept {
        using Decayed = std::decay_t<T>;
        constexpr cmm::info req_id = cmm::detail::hash_entity(std::meta::decay(^^T));
        if (req_id != type_id_ || !data_) return nullptr;
        return static_cast<const Decayed*>(data_);
    }

    template <typename T>
    cmm::Error try_get(T& out_val) const noexcept
        requires std::is_copy_assignable_v<T>
    {
        const T* ptr = get_if<T>();
        if (!ptr) return cmm::Error::TypeMismatch;
        out_val = *ptr;
        return cmm::Error::Success;
    }

    template <typename T>
    T& get() {
        T* ptr = get_if<T>();
        assert(ptr != nullptr && "cmm::Value::get() failed: Type mismatch or empty value!");
        return *ptr;
    }

    template <typename T>
    const T& get() const {
        const T* ptr = get_if<T>();
        assert(ptr != nullptr && "cmm::Value::get() failed: Type mismatch or empty value!");
        return *ptr;
    }

private:
    template <typename T>
    void initialize_type_metadata() {
        constexpr std::meta::info type_refl = std::meta::decay(^^T);
        type_id_ = cmm::detail::hash_entity(type_refl);
        if constexpr (std::is_pointer_v<T> &&
                      !std::is_void_v<std::remove_pointer_t<T>> &&
                      !std::is_function_v<std::remove_pointer_t<T>>) {
            using Pointee = std::remove_pointer_t<T>;
            constexpr std::meta::info pointee_refl =
                std::meta::remove_cv(std::meta::remove_pointer(type_refl));
            pointee_type_id_ = cmm::detail::hash_entity(pointee_refl);
            pointee_is_const_ = std::is_const_v<Pointee>;
        }
    }

    void reset() {
        if (data_ && ops_) ops_->destroy(data_);
        data_ = nullptr;
        ops_ = nullptr;
        type_id_ = cmm::invalid_info;
        pointee_type_id_ = cmm::invalid_info;
        pointee_is_const_ = false;
        policy_ = Policy::Owned;
        is_inline_ = false;
    }

    void copy_from(const Value& other) {
        if (other.try_copy_to(*this) != cmm::Error::Success) std::abort();
    }

    void move_from(Value&& other) noexcept {
        type_id_ = other.type_id_;
        pointee_type_id_ = other.pointee_type_id_;
        pointee_is_const_ = other.pointee_is_const_;
        policy_ = other.policy_;
        ops_ = other.ops_;
        is_inline_ = other.is_inline_;

        if (other.data_ && ops_) {
            if (is_inline_) data_ = ops_->move(other.data_, buffer_);
            else {
                data_ = other.data_;
                other.data_ = nullptr;
            }
        } else {
            data_ = nullptr;
        }
        other.reset();
    }

    cmm::info type_id_{cmm::invalid_info};
    cmm::info pointee_type_id_{cmm::invalid_info};
    Policy policy_{Policy::Owned};
    void* data_{nullptr};
    bool pointee_is_const_{false};

    const detail::ValueOps* ops_{nullptr};
    bool is_inline_{false};
    alignas(detail::SBO_ALIGN) std::byte buffer_[detail::SBO_SIZE];
};

} // namespace cmm

#endif // CMM_VALUE_HPP
