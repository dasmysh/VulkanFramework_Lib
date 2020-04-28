/**
 * @file   DeviceMemoryGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.20
 *
 * @brief  Implementation of a class managing multiple device memory Vulkan objects for simultaneous memory usage.
 */

#include "gfx/vk/memory/DeviceMemoryGroup.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/textures/HostTexture.h"


namespace vkfw_core::gfx {

    DeviceMemoryGroup::DeviceMemoryGroup(const LogicalDevice* device, const vk::MemoryPropertyFlags& memoryFlags) :
        m_device{ device },
        m_deviceMemory{ device, memoryFlags | vk::MemoryPropertyFlagBits::eDeviceLocal }
    {
    }


    DeviceMemoryGroup::~DeviceMemoryGroup() = default;

    DeviceMemoryGroup::DeviceMemoryGroup(DeviceMemoryGroup&& rhs) noexcept :
        m_device{ rhs.m_device },
        m_deviceMemory{ std::move(rhs.m_deviceMemory) },
        m_deviceBuffers{ std::move(rhs.m_deviceBuffers) },
        m_deviceImages{ std::move(rhs.m_deviceImages) },
        m_deviceOffsets{ std::move(rhs.m_deviceOffsets) }
    {
    }

    DeviceMemoryGroup& DeviceMemoryGroup::operator=(DeviceMemoryGroup&& rhs) noexcept
    {
        this->~DeviceMemoryGroup();
        m_device = rhs.m_device;
        m_deviceMemory = std::move(rhs.m_deviceMemory);
        m_deviceBuffers = std::move(rhs.m_deviceBuffers);
        m_deviceImages = std::move(rhs.m_deviceImages);
        m_deviceOffsets = std::move(rhs.m_deviceOffsets);
        return *this;
    }

    unsigned int DeviceMemoryGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        m_deviceBuffers.emplace_back(m_device, vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlags(),
                                     queueFamilyIndices);
        m_deviceBuffers.back().InitializeBuffer(size, false);

        return static_cast<unsigned int>(m_deviceBuffers.size() - 1);
    }

    unsigned int DeviceMemoryGroup::AddTextureToGroup(const TextureDescriptor& desc, const glm::u32vec4& size,
        std::uint32_t mipLevels, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        m_deviceImages.emplace_back(m_device, TextureDescriptor(desc, vk::ImageUsageFlagBits::eTransferDst),
                                    queueFamilyIndices);
        m_deviceImages.back().InitializeImage(size, mipLevels, false);

        return static_cast<unsigned int>(m_deviceImages.size() - 1);
    }

    void DeviceMemoryGroup::FinalizeDeviceGroup()
    {
        InitializeDeviceMemory(m_device, m_deviceOffsets, m_deviceBuffers, m_deviceImages, m_deviceMemory);
        BindDeviceObjects(m_deviceOffsets, m_deviceBuffers, m_deviceImages, m_deviceMemory);
    }

    template<class B, class T> void DeviceMemoryGroup::InitializeMemory(const LogicalDevice* device,
        std::vector<std::size_t>& offsets, const std::vector<B>& buffers, const std::vector<T>& images,
        DeviceMemory& memory)
    {
        vk::MemoryAllocateInfo allocInfo;
        offsets.resize(buffers.size() + images.size());
        std::size_t offset = 0U;
        for (auto i = 0U; i < buffers.size(); ++i) {
            offsets[i] = offset;
            offset += FillBufferAllocationInfo(device, buffers[i], allocInfo);
        }
        for (auto i = 0U; i < images.size(); ++i) {
            offsets[i + buffers.size()] = offset;
            offset += FillImageAllocationInfo(device, (i == 0 ? nullptr : &images[i - 1]),
                images[i], offsets[i + buffers.size()], allocInfo);
        }
        memory.InitializeMemory(allocInfo);
    }

    template<class B, class T> void DeviceMemoryGroup::BindObjects(const std::vector<std::size_t>& offsets,
        std::vector<B>& buffers, std::vector<T>& images, DeviceMemory & memory)
    {
        for (auto i = 0U; i < buffers.size(); ++i) {
            memory.BindToBuffer(buffers[i], offsets[i]);
        }
        for (auto i = 0U; i < images.size(); ++i) {
            memory.BindToTexture(images[i], offsets[i + buffers.size()]);
        }
    }

    void DeviceMemoryGroup::InitializeDeviceMemory(const LogicalDevice* device, std::vector<std::size_t>& deviceOffsets,
        const std::vector<DeviceBuffer>& deviceBuffers, const std::vector<DeviceTexture>& deviceImages, DeviceMemory& deviceMemory)
    {
        InitializeMemory(device, deviceOffsets, deviceBuffers, deviceImages, deviceMemory);
    }

    void DeviceMemoryGroup::InitializeHostMemory(const LogicalDevice* device, std::vector<std::size_t>& hostOffsets,
        const std::vector<HostBuffer>& hostBuffers, const std::vector<HostTexture>& hostImages, DeviceMemory& hostMemory)
    {
        InitializeMemory(device, hostOffsets, hostBuffers, hostImages, hostMemory);
    }

    void DeviceMemoryGroup::BindDeviceObjects(const std::vector<std::size_t>& deviceOffsets,
        std::vector<DeviceBuffer>& deviceBuffers, std::vector<DeviceTexture>& deviceImages, DeviceMemory& deviceMemory)
    {
        BindObjects(deviceOffsets, deviceBuffers, deviceImages, deviceMemory);
        for (auto& deviceImage : deviceImages) {
            deviceImage.InitializeImageView();
        }
    }

    void DeviceMemoryGroup::BindHostObjects(const std::vector<std::size_t>& hostOffsets,
        std::vector<HostBuffer>& hostBuffers, std::vector<HostTexture>& hostImages, DeviceMemory & hostMemory)
    {
        BindObjects(hostOffsets, hostBuffers, hostImages, hostMemory);
    }

    std::size_t DeviceMemoryGroup::FillBufferAllocationInfo(const LogicalDevice* device, const Buffer& buffer, vk::MemoryAllocateInfo& allocInfo)
    {
        auto memRequirements = device->GetDevice().getBufferMemoryRequirements(buffer.GetBuffer());
        return FillAllocationInfo(device, memRequirements, buffer.GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::size_t DeviceMemoryGroup::FillImageAllocationInfo(const LogicalDevice* device,
        const Texture* lastImage, const Texture& image,
        std::size_t& imageOffset, vk::MemoryAllocateInfo& allocInfo)
    {
        auto memRequirements = device->GetDevice().getImageMemoryRequirements(image.GetImage());
        std::size_t newOffset;
        if (lastImage == nullptr) {
            newOffset = device->CalculateBufferImageOffset(image, imageOffset);
        } else {
            newOffset = device->CalculateImageImageOffset(*lastImage, image, imageOffset);
        }
        memRequirements.size += newOffset - imageOffset;
        imageOffset = newOffset;
        return FillAllocationInfo(device, memRequirements, image.GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::size_t DeviceMemoryGroup::FillAllocationInfo(const LogicalDevice* device, const vk::MemoryRequirements& memRequirements,
        const vk::MemoryPropertyFlags& memProperties, vk::MemoryAllocateInfo& allocInfo)
    {
        if (allocInfo.allocationSize == 0) {
            allocInfo.memoryTypeIndex =
                DeviceMemory::FindMemoryType(device, memRequirements.memoryTypeBits, memProperties);
        } else if (!DeviceMemory::CheckMemoryType(device, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, memProperties)) {
            spdlog::critical(
                "MemoryGroup memory type ({}) does not fit required memory type for buffer or image ({:#x}).",
                allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits);
            throw std::runtime_error("MemoryGroup memory type does not fit required memory type for buffer or image.");
        }

        allocInfo.allocationSize += memRequirements.size;
        return memRequirements.size;
    }
}
