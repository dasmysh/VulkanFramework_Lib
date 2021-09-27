/**
 * @file   DeviceMemoryGroup.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.20
 *
 * @brief  Declaration of a class managing multiple device memory Vulkan objects for simultaneous memory usage.
 */

#pragma once

#include "main.h"
#include "DeviceMemory.h"
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/textures/DeviceTexture.h"

namespace vkfw_core::gfx {

    class LogicalDevice;
    class HostBuffer;
    class HostTexture;

    class DeviceMemoryGroup
    {
    public:
        explicit DeviceMemoryGroup(const LogicalDevice* device, std::string_view name, const vk::MemoryPropertyFlags& memoryFlags = vk::MemoryPropertyFlags());
        virtual ~DeviceMemoryGroup();
        DeviceMemoryGroup(const DeviceMemoryGroup&) = delete;
        DeviceMemoryGroup& operator=(const DeviceMemoryGroup&) = delete;
        DeviceMemoryGroup(DeviceMemoryGroup&&) noexcept;
        DeviceMemoryGroup& operator=(DeviceMemoryGroup&&) noexcept;

        static constexpr unsigned int INVALID_INDEX = std::numeric_limits<unsigned int>::max();
        virtual unsigned int
        AddBufferToGroup(std::string_view name, const vk::BufferUsageFlags& usage,
                         const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        virtual unsigned int
        AddBufferToGroup(std::string_view name, const vk::BufferUsageFlags& usage, std::size_t size,
                         const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});
        virtual unsigned int
        AddTextureToGroup(std::string_view name, const TextureDescriptor& desc, const glm::u32vec4& size,
                          std::uint32_t mipLevels,
                          const std::vector<std::uint32_t>& queueFamilyIndices = std::vector<std::uint32_t>{});

        virtual void FinalizeDeviceGroup();

        [[nodiscard]] DeviceBuffer* GetBuffer(unsigned int bufferIdx) { return &m_deviceBuffers[bufferIdx]; }
        [[nodiscard]] const DeviceBuffer* GetBuffer(unsigned int bufferIdx) const { return &m_deviceBuffers[bufferIdx]; }
        [[nodiscard]] DeviceTexture* GetTexture(unsigned int textureIdx) { return &m_deviceImages[textureIdx]; }
        [[nodiscard]] std::size_t GetBuffersInGroup() const { return m_deviceBuffers.size(); }
        [[nodiscard]] std::size_t GetImagesInGroup() const { return m_deviceImages.size(); }

    protected:
        static void InitializeDeviceMemory(const LogicalDevice* device, std::vector<std::size_t>& deviceOffsets,
            const std::vector<DeviceBuffer>& deviceBuffers, const std::vector<DeviceTexture>& deviceImages,
            DeviceMemory& deviceMemory);
        static void InitializeHostMemory(const LogicalDevice* device, std::vector<std::size_t>& hostOffsets,
            const std::vector<HostBuffer>& hostBuffers, const std::vector<HostTexture>& hostImages,
            DeviceMemory& hostMemory);
        static void BindDeviceObjects(const std::vector<std::size_t>& deviceOffsets, std::vector<DeviceBuffer>& deviceBuffers,
            std::vector<DeviceTexture>& deviceImages, DeviceMemory& deviceMemory);
        static void BindHostObjects(const std::vector<std::size_t>& hostOffsets, std::vector<HostBuffer>& hostBuffers,
            std::vector<HostTexture>& hostImages, DeviceMemory& hostMemory);

        static std::size_t FillBufferAllocationInfo(const LogicalDevice* device, const Buffer& buffer, vk::MemoryAllocateInfo& allocInfo, bool& shaderDeviceAddress);
        static std::size_t FillImageAllocationInfo(const LogicalDevice* device, const Texture* lastImage,
            const Texture& image, std::size_t& imageOffset, vk::MemoryAllocateInfo& allocInfo);
        static std::size_t FillAllocationInfo(const LogicalDevice* device, const vk::MemoryRequirements& memRequirements,
            const vk::MemoryPropertyFlags& memProperties, vk::MemoryAllocateInfo& allocInfo);

        [[nodiscard]] const LogicalDevice* GetDevice() const { return m_device; }

    private:
        template<class B, class T> static void InitializeMemory(const LogicalDevice* device,
            std::vector<std::size_t>& offsets, const std::vector<B>& buffers,
            const std::vector<T>& images, DeviceMemory& memory);
        template<class B, class T> static void BindObjects(const std::vector<std::size_t>& offsets,
            std::vector<B>& buffers, std::vector<T>& images, DeviceMemory& memory);

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the Vulkan device memory for the device objects. */
        DeviceMemory m_deviceMemory;
        /** Holds the device buffers. */
        std::vector<DeviceBuffer> m_deviceBuffers;
        /** Holds the device images. */
        std::vector<DeviceTexture> m_deviceImages;
        /** Holds the offsets for the device memory objects. */
        std::vector<std::size_t> m_deviceOffsets;
    };
}
