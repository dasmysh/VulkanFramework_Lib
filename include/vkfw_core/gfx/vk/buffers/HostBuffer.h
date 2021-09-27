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

namespace vkfw_core::gfx {

    class DeviceBuffer;

    class HostBuffer final : public Buffer
    {
    public:
        HostBuffer(const LogicalDevice* device, std::string_view name, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryFlags = vk::MemoryPropertyFlags(),
            const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        ~HostBuffer() override;
        HostBuffer(const HostBuffer&);
        HostBuffer& operator=(const HostBuffer&);
        HostBuffer(HostBuffer&&) noexcept;
        HostBuffer& operator=(HostBuffer&&) noexcept;

        void InitializeData(std::size_t bufferSize, std::size_t dataSize, const void* data);
        void InitializeData(std::size_t size, const void* data);
        void UploadData(std::size_t offset, std::size_t size, const void* data);
        void DownloadData(std::size_t size, void* data) const;

        template<contiguous_memory T> void InitializeData(std::size_t bufferSize, const T& data);
        template<contiguous_memory T> void InitializeData(const T& data);
        template<contiguous_memory T> void UploadData(std::size_t offset, const T& data);
        template<contiguous_memory T> void DownloadData(T& data) const;

    private:
        void UploadDataInternal(std::size_t offset, std::size_t size, const void* data) const;
    };

    template<contiguous_memory T> void HostBuffer::InitializeData(std::size_t bufferSize, const T& data)
    {
        InitializeData(bufferSize, byteSizeOf(data), data.data());
    }

    template<contiguous_memory T> void HostBuffer::InitializeData(const T& data)
    {
        InitializeData(byteSizeOf(data), byteSizeOf(data), data.data());
    }

    template<contiguous_memory T> void HostBuffer::UploadData(std::size_t offset, const T& data)
    {
        UploadData(offset, byteSizeOf(data), data.data());
    }

    template<contiguous_memory T> void HostBuffer::DownloadData(T& data) const
    {
        DownloadData(byteSizeOf(data), data.data());
    }
}
