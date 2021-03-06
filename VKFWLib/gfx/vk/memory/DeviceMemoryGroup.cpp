/**
 * @file   DeviceMemoryGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.20
 *
 * @brief  Implementation of a class managing multiple device memory Vulkan objects for simultaneous memory usage.
 */

#include "DeviceMemoryGroup.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/textures/HostTexture.h"


namespace vku::gfx {

    DeviceMemoryGroup::DeviceMemoryGroup(const LogicalDevice* device, vk::MemoryPropertyFlags memoryFlags) :
        device_{ device },
        deviceMemory_{ device, memoryFlags | vk::MemoryPropertyFlagBits::eDeviceLocal }
    {
    }


    DeviceMemoryGroup::~DeviceMemoryGroup() = default;

    DeviceMemoryGroup::DeviceMemoryGroup(DeviceMemoryGroup&& rhs) noexcept :
        device_{ rhs.device_ },
        deviceMemory_{ std::move(rhs.deviceMemory_) },
        deviceBuffers_{ std::move(rhs.deviceBuffers_) },
        deviceImages_{ std::move(rhs.deviceImages_) },
        deviceOffsets_{ std::move(rhs.deviceOffsets_) }
    {
    }

    DeviceMemoryGroup& DeviceMemoryGroup::operator=(DeviceMemoryGroup&& rhs) noexcept
    {
        this->~DeviceMemoryGroup();
        device_ = rhs.device_;
        deviceMemory_ = std::move(rhs.deviceMemory_);
        deviceBuffers_ = std::move(rhs.deviceBuffers_);
        deviceImages_ = std::move(rhs.deviceImages_);
        deviceOffsets_ = std::move(rhs.deviceOffsets_);
        return *this;
    }

    unsigned int DeviceMemoryGroup::AddBufferToGroup(vk::BufferUsageFlags usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        deviceBuffers_.emplace_back(device_, vk::BufferUsageFlagBits::eTransferDst | usage, vk::MemoryPropertyFlags(), queueFamilyIndices);
        deviceBuffers_.back().InitializeBuffer(size, false);

        return static_cast<unsigned int>(deviceBuffers_.size() - 1);
    }

    unsigned int DeviceMemoryGroup::AddTextureToGroup(const TextureDescriptor& desc, const glm::u32vec4& size,
        std::uint32_t mipLevels, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        deviceImages_.emplace_back(device_, TextureDescriptor(desc, vk::ImageUsageFlagBits::eTransferDst), queueFamilyIndices);
        deviceImages_.back().InitializeImage(size, mipLevels, false);

        return static_cast<unsigned int>(deviceImages_.size() - 1);
    }

    void DeviceMemoryGroup::FinalizeDeviceGroup()
    {
        InitializeDeviceMemory(device_, deviceOffsets_, deviceBuffers_, deviceImages_, deviceMemory_);
        BindDeviceObjects(deviceOffsets_, deviceBuffers_, deviceImages_, deviceMemory_);
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
        for (auto i = 0U; i < deviceImages.size(); ++i) {
            deviceImages[i].InitializeImageView();
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
        if (lastImage == nullptr) newOffset = device->CalculateBufferImageOffset(image, imageOffset);
        else newOffset = device->CalculateImageImageOffset(*lastImage, image, imageOffset);
        memRequirements.size += newOffset - imageOffset;
        imageOffset = newOffset;
        return FillAllocationInfo(device, memRequirements, image.GetDeviceMemory().GetMemoryProperties(), allocInfo);
    }

    std::size_t DeviceMemoryGroup::FillAllocationInfo(const LogicalDevice* device, const vk::MemoryRequirements& memRequirements,
        vk::MemoryPropertyFlags memProperties, vk::MemoryAllocateInfo& allocInfo)
    {
        if (allocInfo.allocationSize == 0) allocInfo.memoryTypeIndex =
            DeviceMemory::FindMemoryType(device, memRequirements.memoryTypeBits, memProperties);
        else if (!DeviceMemory::CheckMemoryType(device, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits, memProperties)) {
            LOG(FATAL) << "MemoryGroup memory type (" << allocInfo.memoryTypeIndex << ") does not fit required memory type for buffer or image ("
                << std::hex << memRequirements.memoryTypeBits << ").";
            throw std::runtime_error("MemoryGroup memory type does not fit required memory type for buffer or image.");
        }

        allocInfo.allocationSize += memRequirements.size;
        return memRequirements.size;
    }
}
