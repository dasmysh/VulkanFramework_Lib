/**
 * @file   Buffer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Declaration of a general Vulkan buffer.
 */

#pragma once

#include "main.h"
#include <core/type_traits.h>

namespace vku { namespace gfx {

    class Buffer final
    {
    public:
        Buffer(LogicalDevice* device, vk::BufferUsageFlags usage);
        ~Buffer();
        Buffer(const Buffer&);
        Buffer& operator=(const Buffer&);
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;

        void InitializeData(size_t size, const void* data);
        void UploadData(size_t offset, size_t size, const void* data);
        void DownloadData(size_t size, void* data) const;

        unsigned int GetSize() const { return size_; }
        vk::Buffer GetBuffer() const { return buffer_; }

        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> InitializeData(const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> UploadData(size_t offset, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value>  DownloadData(T& data) const;

    private:
        uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
        void InitializeBuffer(size_t size);
        void UploadDataInternal(size_t offset, size_t size, const void* data) const;

        /** Holds the device. */
        LogicalDevice* device_;
        /** Holds the Vulkan buffer object. */
        vk::Buffer buffer_;
        /** Holds the Vulkan device memory for the buffer. */
        vk::DeviceMemory bufferDeviceMemory_;
        /** Holds the current size of the buffer in bytes. */
        size_t size_;
        /** Holds the buffer usage. */
        vk::BufferUsageFlags usage_;
    };

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> Buffer::InitializeData(const T& data)
    {
        InitializeData(static_cast<unsigned int>(sizeof(T::value_type) * data.size()), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> Buffer::UploadData(size_t offset, const T& data)
    {
        UploadData(offset, static_cast<unsigned int>(sizeof(T::value_type) * data.size()), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> Buffer::DownloadData(T& data) const
    {
        DownloadData(static_cast<unsigned int>(sizeof(T::value_type) * data.size()), data.data());
    }
}}
