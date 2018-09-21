/**
 * @file   aligned_vector.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.17
 *
 * @brief  Definition of a vector class that allows its entries to be aligned at compile time.
 */

#pragma once

#include <vector>
#include <cstdint>

namespace vku {

    template<typename T> class aligned_vector
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using reference = value_type&;
        using const_reference = const value_type&;

        aligned_vector(size_type alignedSize, size_type count, const T& value);
        aligned_vector(size_type alignedSize, size_type count = 0) : alignedSize_{ alignedSize }, cont_{ count * alignedSize } {}
        // template<class InputIt> aligned_vector(size_type alignedSize, InputIt first, InputIt last);
        aligned_vector(size_type alignedSize, std::initializer_list<T> init);
        aligned_vector(const aligned_vector& rhs) : alignedSize_{ rhs.alignedSize_ }, cont_{ rhs.cont_ } {}
        aligned_vector& operator=(const aligned_vector&);
        aligned_vector(aligned_vector&& rhs) noexcept : alignedSize_{ rhs.alignedSize_ }, cont_{ std::move(rhs.cont_) } {}
        aligned_vector& operator=(aligned_vector&&) noexcept;
        ~aligned_vector();

        /*aligned_vector& operator=(std::initializer_list<T> ilist);

        void assign(size_type count, const T& value);
        template<class InputIt> void assign(size_type alignment, InputIt first, InputIt last);
        void assign(std::initializer_list<T> init);*/

        reference at(size_type pos) { return *reinterpret_cast<T*>(&cont_.at(pos * alignedSize_)); }
        const_reference at(size_type pos) const { return *reinterpret_cast<const T*>(&cont_.at(pos * alignedSize_)); }
        reference operator[](size_type pos) { return *reinterpret_cast<T*>(&cont_[pos * alignedSize_]); }
        const_reference operator[](size_type pos) const { return *reinterpret_cast<const T*>(&cont_[pos * alignedSize_]); }
        reference front() { return *reinterpret_cast<T*>(&cont_.front()); }
        const_reference front() const { return *reinterpret_cast<const T*>(&cont_.front()); }
        reference back() { return *reinterpret_cast<T*>(&cont_[cont_.size() - alignedSize_]); }
        const_reference back() const { return *reinterpret_cast<const T*>(&cont_[cont_.size() - alignedSize_]); }
        T* data() noexcept { return reinterpret_cast<T*>(cont_.data()); }
        const T* data() const noexcept { return reinterpret_cast<const T*>(cont_.data()); }

        /*iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;
        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;
        reverse_iterator rbegin() noexcept;
        const_reverse_iterator rbegin() const noexcept;
        const_reverse_iterator crbegin() const noexcept;
        reverse_iterator rend() noexcept;
        const_reverse_iterator rend() const noexcept;
        const_reverse_iterator crend() const noexcept;*/

        bool empty() const noexcept { return cont_.empty(); }
        size_type size() const noexcept { return cont_.size() / alignedSize_; }
        size_type max_size() const noexcept { return cont_.max_size() / alignedSize_; }
        void reserve(size_type new_cap) { cont_.reserve(new_cap * alignedSize_); }
        size_type capacity() const noexcept { return cont_.capacity() / alignedSize_; }
        void shrink_to_fit() { cont_.shrink_to_fit(); }

        void clear() noexcept { cont_.clear(); }
        /*iterator insert(const_iterator pos, const T& value);
        iterator insert(const_iterator pos, T&& value);
        iterator insert(const_iterator pos, size_type count, const T& value);
        template<class InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last);
        iterator insert(const_iterator pos, std::initializer_list<T> ilist);
        template<class... Args> iterator emplace(const_iterator pos, Args&&... args);
        iterator erase(const_iterator pos);
        iterator erase(const_iterator first, const_iterator last);*/
        void push_back(const T& value);
        void push_back(T&& value);
        template<class... Args> reference emplace_back(Args&&... args);
        void pop_back();
        void resize(size_type count) { cont_.resize(count * alignedSize_); }
        void resize(size_type count, const value_type& value);
        void swap(aligned_vector& other) noexcept;

        std::size_t GetAlignedSize() const { return alignedSize_; }

    private:
        /** Holds the vectors alignment. */
        std::size_t alignedSize_;
        /** Holds the internal byte vector. */
        std::vector<std::uint8_t> cont_;
    };

    template<typename T>
    inline aligned_vector<T>::aligned_vector(size_type alignedSize, size_type count, const T& value) :
        aligned_vector{ alignedSize, count }
    {
        for (size_type i = 0U; i < count; ++i) {
            new(reinterpret_cast<T*>(cont_[i * alignedSize_])) T(value);
        }
    }

    template<typename T>
    inline aligned_vector<T>::aligned_vector(size_type alignedSize, std::initializer_list<T> init) :
        aligned_vector{ alignedSize, init.size() }
    {
        size_type i = 0U;
        for (const auto& elem : init) {
            new(reinterpret_cast<T*>(cont_[i * alignedSize_])) T(init[i]);
            i += 1;
        }
    }

    template<typename T>
    inline aligned_vector<T>& aligned_vector<T>::operator=(const aligned_vector<T>& rhs)
    {
        if (this != &rhs) {
            alignedSize_ = rhs.alignedSize_;
            cont_ = rhs.cont_;
        }
        return *this;
    }

    template<typename T>
    inline aligned_vector<T>& aligned_vector<T>::operator=(aligned_vector<T>&& rhs) noexcept
    {
        alignedSize_ = rhs.alignedSize_;
        cont_ = std::move(rhs.cont_);
        return *this;
    }

    template<typename T>
    inline aligned_vector<T>::~aligned_vector()
    {
        for (size_type i = 0; i < cont_.size(); i += alignedSize_) reinterpret_cast<T*>(&cont_[i])->~T();
    }

    template<typename T>
    inline void aligned_vector<T>::push_back(const T& value)
    {
        auto oldSize = cont_.size();
        resize(oldSize + alignedSize_);
        new(reinterpret_cast<T*>(cont_.data() + oldSize)) T(value);
    }

    template<typename T>
    inline void aligned_vector<T>::push_back(T&& value)
    {
        auto oldSize = cont_.size();
        resize(oldSize + alignedSize_);
        new(reinterpret_cast<T*>(cont_.data() + oldSize)) T(std::move(value));
    }

    template<typename T>
    template<class ...Args>
    inline typename aligned_vector<T>::reference aligned_vector<T>::emplace_back(Args&&... args)
    {
        auto oldSize = cont_.size();
        resize(oldSize + alignedSize_);
        new(reinterpret_cast<T*>(cont_.data() + oldSize)) T(std::forward<Args>(args));
    }

    template<typename T>
    inline void aligned_vector<T>::pop_back()
    {
        for (auto i = 0U; i < alignedSize_; ++i) cont_.pop_back();
    }

    template<typename T>
    inline void aligned_vector<T>::resize(size_type count, const value_type& value)
    {
        auto oldElems = cont_.size() / alignedSize_;
        cont_.resize(count * alignedSize_);
        for (auto i = oldElems; i < count; ++i) {
            auto newElemPos = (oldElems + i) * alignedSize_;
            new(reinterpret_cast<T*>(cont_.data() + newElemPos)) T(value);
        }
    }

    template<typename T>
    inline void aligned_vector<T>::swap(aligned_vector& other) noexcept
    {
        auto tmp = other.alignedSize_;
        other.alignedSize_ = alignedSize_;
        alignedSize_ = tmp;
        cont_.swap(other.cont_);
    }
}
