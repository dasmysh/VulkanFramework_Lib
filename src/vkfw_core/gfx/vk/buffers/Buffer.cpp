/**
 * @file   Buffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general Vulkan buffer.
 */

#include "gfx/vk/buffers/Buffer.h"
#include "gfx/vk/wrappers/CommandBuffer.h"

namespace vkfw_core::gfx {

    Buffer::Buffer(const LogicalDevice* device, std::string_view name, const vk::BufferUsageFlags& usage,
                   const vk::MemoryPropertyFlags& memoryFlags, std::vector<std::uint32_t> queueFamilyIndices)
        : VulkanObjectWrapper{nullptr, name, vk::UniqueBuffer{}}
        , m_device{device}
        , m_bufferDeviceMemory{device, fmt::format("ImgMem:{}", GetName()), memoryFlags}
        , m_size{0}
        , m_usage{usage}
        , m_queueFamilyIndices{std::move(queueFamilyIndices)}
    {
    }

    Buffer::~Buffer() = default;

    Buffer::Buffer(Buffer&& rhs) noexcept : VulkanObjectWrapper{std::move(rhs)},
        m_device{rhs.m_device}
        , m_bufferDeviceMemory{std::move(rhs.m_bufferDeviceMemory)}
        , m_size{rhs.m_size}
        , m_usage{rhs.m_usage}
        , m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
    {
        rhs.m_size = 0;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        this->~Buffer();
        VulkanObjectWrapper::operator=(std::move(rhs));
        m_device = rhs.m_device;
        m_bufferDeviceMemory = std::move(rhs.m_bufferDeviceMemory);
        m_size = rhs.m_size;
        m_usage = rhs.m_usage;
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        rhs.m_size = 0;
        return *this;
    }

    void Buffer::InitializeBuffer(std::size_t size, bool initMemory)
    {
        // not sure if this is needed again at some point.
        assert(!GetHandle());

        m_size = size;
        vk::BufferCreateInfo bufferCreateInfo{vk::BufferCreateFlags(), static_cast<vk::DeviceSize>(m_size), m_usage,
                                              vk::SharingMode::eExclusive};
        if (!m_queueFamilyIndices.empty()) {
            bufferCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(m_queueFamilyIndices.size()));
            bufferCreateInfo.setPQueueFamilyIndices(m_queueFamilyIndices.data());
        }
        if (m_queueFamilyIndices.size() > 1) { bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive); }
        SetHandle(m_device->GetHandle(), m_device->GetHandle().createBufferUnique(bufferCreateInfo));

        if (initMemory) {
            auto memRequirements = m_device->GetHandle().getBufferMemoryRequirements(GetHandle());
            m_bufferDeviceMemory.InitializeMemory(memRequirements, IsShaderDeviceAddress());
            m_bufferDeviceMemory.BindToBuffer(*this, 0);
        }
    }

    void Buffer::CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset,
                                 std::size_t size, const CommandBuffer& cmdBuffer) const
    {
        assert(m_usage & vk::BufferUsageFlagBits::eTransferSrc);
        assert(dstBuffer.m_usage & vk::BufferUsageFlagBits::eTransferDst);
        assert(srcOffset + size <= m_size);
        assert(dstOffset + size <= dstBuffer.m_size);

        vk::BufferCopy copyRegion{srcOffset, dstOffset, size};
        cmdBuffer.GetHandle().copyBuffer(GetHandle(), dstBuffer.GetHandle(), copyRegion);
    }

    CommandBuffer Buffer::CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset,
                                          std::size_t size, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores, const Fence& fence) const
    {
        auto transferCmdBuffer = CommandBuffer::beginSingleTimeSubmit(
            m_device, fmt::format("CopyCMDBuffer {}", GetName()),
            fmt::format("Copy {} to {}", GetName(), dstBuffer.GetName()), copyQueue.GetCommandPool());
        CopyBufferAsync(srcOffset, dstBuffer, dstOffset, size, transferCmdBuffer);
        CommandBuffer::endSingleTimeSubmit(copyQueue, transferCmdBuffer, waitSemaphores, signalSemaphores, fence);

        return transferCmdBuffer;
    }

    CommandBuffer Buffer::CopyBufferAsync(const Buffer& dstBuffer, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores, const Fence& fence) const
    {
        return CopyBufferAsync(0, dstBuffer, 0, m_size, copyQueue, waitSemaphores, signalSemaphores, fence);
    }

    void Buffer::CopyBufferSync(const Buffer& dstBuffer, const Queue& copyQueue) const
    {
        auto cmdBuffer = CopyBufferAsync(dstBuffer, copyQueue);
        copyQueue.WaitIdle();
    }

    vk::DeviceOrHostAddressConstKHR Buffer::GetDeviceAddressConst() const
    {
        assert(IsShaderDeviceAddress());
        vk::BufferDeviceAddressInfo bufferAddressInfo{GetHandle()};
        return m_device->GetHandle().getBufferAddress(bufferAddressInfo);
    }

    vk::DeviceOrHostAddressKHR Buffer::GetDeviceAddress()
    {
        assert(IsShaderDeviceAddress());
        vk::BufferDeviceAddressInfo bufferAddressInfo{GetHandle()};
        return m_device->GetHandle().getBufferAddress(bufferAddressInfo);
    }

}
