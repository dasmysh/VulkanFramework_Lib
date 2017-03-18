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
#include "core/type_traits.h"

namespace vku::gfx {

    class DeviceBuffer;

    class HostBuffer final : public Buffer
    {
    public:
        HostBuffer(const LogicalDevice* device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        ~HostBuffer();
        HostBuffer(const HostBuffer&);
        HostBuffer& operator=(const HostBuffer&);
        HostBuffer(HostBuffer&&) noexcept;
        HostBuffer& operator=(HostBuffer&&) noexcept;

        void InitializeData(std::size_t bufferSize, std::size_t dataSize, const void* data);
        void InitializeData(std::size_t size, const void* data);
        void UploadData(std::size_t offset, std::size_t size, const void* data);
        void DownloadData(std::size_t size, void* data) const;

        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> InitializeData(std::size_t bufferSize, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> InitializeData(const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> UploadData(std::size_t offset, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value>  DownloadData(T& data) const;

    private:
        void UploadDataInternal(std::size_t offset, std::size_t size, const void* data) const;
    };

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::InitializeData(std::size_t bufferSize, const T& data)
    {
        InitializeData(bufferSize, byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::InitializeData(const T& data)
    {
        InitializeData(byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::UploadData(std::size_t offset, const T& data)
    {
        UploadData(offset, byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostBuffer::DownloadData(T& data) const
    {
        DownloadData(byteSizeOf(data), data.data());
    }
}
