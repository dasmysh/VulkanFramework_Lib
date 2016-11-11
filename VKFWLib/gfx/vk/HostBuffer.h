/**
 * @file   HostBuffer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Declaration of a general host accessible Vulkan buffer.
 */

#pragma once

#include "main.h"
#include "Buffer.h"
#include <core/type_traits.h>

namespace vku { namespace gfx {
    class DeviceBuffer;

    class HostBuffer final : public Buffer
    {
    public:
        HostBuffer(const LogicalDevice* device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        ~HostBuffer();
        HostBuffer(const HostBuffer&);
        HostBuffer& operator=(const HostBuffer&);
        HostBuffer(HostBuffer&&) noexcept;
        HostBuffer& operator=(HostBuffer&&) noexcept;

        void InitializeData(size_t size, const void* data);
        void UploadData(size_t offset, size_t size, const void* data);
        void DownloadData(size_t size, void* data) const;

        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> InitializeData(const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> UploadData(size_t offset, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value>  DownloadData(T& data) const;

    private:
        void UploadDataInternal(size_t offset, size_t size, const void* data) const;
    };

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::InitializeData(const T& data)
    {
        InitializeData(static_cast<unsigned int>(sizeof(T::value_type) * data.size()), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::UploadData(size_t offset, const T& data)
    {
        UploadData(offset, static_cast<unsigned int>(sizeof(T::value_type) * data.size()), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::DownloadData(T& data) const
    {
        DownloadData(static_cast<unsigned int>(sizeof(T::value_type) * data.size()), data.data());
    }
}}
