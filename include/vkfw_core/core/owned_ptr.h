/**
 * @file   owned_ptr.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.05.08
 *
 * @brief  Definition of the owned pointer.
 */

#pragma once

#include <utility>

namespace vkfw_core::stdext {

    template <typename T> class owned_ptr final
    {
    public:
        owned_ptr() : ptr_{ nullptr }, owned_{ false } {};
        owned_ptr(T* ptr, bool owned) : ptr_{ ptr }, owned_{ owned } {};
        owned_ptr(const owned_ptr& rhs) = delete;
        owned_ptr& operator=(const owned_ptr&) = delete;
        owned_ptr(owned_ptr&& rhs) noexcept : ptr_(rhs.ptr_), owned_{ rhs.owned_ } { rhs.ptr_ = nullptr; }
        owned_ptr& operator=(owned_ptr&& rhs) noexcept { ptr_ = rhs.ptr_; owned_ = rhs.owned_; rhs.ptr_ = nullptr; return *this; }
        ~owned_ptr() { if (owned_) delete ptr_; }

        operator T*() const { return ptr_; }
        T* operator->() { return ptr_; }
        const T* operator->() const { return ptr_; }
        T& operator*() { return *ptr_; }
        const T& operator*() const { return *ptr_; }
        explicit operator bool() const { return nullptr != ptr_; }
        bool operator==(const owned_ptr& rhs) { return rhs.ptr_ == ptr_; }

        T* release() { T* tmp = ptr_; ptr_ = nullptr; return tmp; }
        void reset(T* newObj = nullptr) { this->~owned_ptr(); ptr_ = newObj; }
        void swap(owned_ptr& other) noexcept { T* tmp = ptr_; ptr_ = other.ptr_; other.ptr_ = tmp; }
        bool is_owned() const { return owned_; }

    private:
        /** Holds the pointer. */
        T* ptr_;
        /** Holds a flag whether the pointer is owned or not. */
        bool owned_;
    };

    template<class T, class... Args> owned_ptr<T> make_owned(Args&&... args)
    {
        return owned_ptr<T>(new T(std::forward<Args>(args)...), true);
    }
}
