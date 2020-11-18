/**
 * @file   Buffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.10
 *
 * @brief  Implementation of a general Vulkan buffer.
 */

#include "gfx/vk/buffers/Buffer.h"
#include "gfx/vk/buffers/BufferGroup.h"
#include "gfx/vk/CommandBuffers.h"

namespace vkfw_core::gfx {

    Buffer::Buffer(const LogicalDevice* device, const vk::BufferUsageFlags& usage,
        const vk::MemoryPropertyFlags& memoryFlags, std::vector<std::uint32_t> queueFamilyIndices) :
        m_device{ device },
        m_bufferDeviceMemory{ device, memoryFlags },
        m_size{ 0 },
        m_usage{ usage },
        m_queueFamilyIndices{ std::move(queueFamilyIndices) }
    {
    }

    Buffer::~Buffer() = default;

    Buffer::Buffer(Buffer&& rhs) noexcept
        : m_device{rhs.m_device},
          m_bufferDeviceMemory{std::move(rhs.m_bufferDeviceMemory)},
          m_buffer{std::move(rhs.m_buffer)},
          m_size{rhs.m_size},
          m_usage{rhs.m_usage},
          m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
    {
        rhs.m_size = 0;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        this->~Buffer();
        m_device = rhs.m_device;
        m_bufferDeviceMemory = std::move(rhs.m_bufferDeviceMemory);
        m_buffer = std::move(rhs.m_buffer);
        m_size = rhs.m_size;
        m_usage = rhs.m_usage;
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        rhs.m_size = 0;
        return *this;
    }

    void Buffer::InitializeBuffer(std::size_t size, bool initMemory)
    {
        this->~Buffer();

        m_size = size;
        vk::BufferCreateInfo bufferCreateInfo{ vk::BufferCreateFlags(), static_cast<vk::DeviceSize>(m_size), m_usage, vk::SharingMode::eExclusive };
        if (!m_queueFamilyIndices.empty()) {
            bufferCreateInfo.setQueueFamilyIndexCount(static_cast<std::uint32_t>(m_queueFamilyIndices.size()));
            bufferCreateInfo.setPQueueFamilyIndices(m_queueFamilyIndices.data());
        }
        if (m_queueFamilyIndices.size() > 1) { bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive); }
        m_buffer = m_device->GetDevice().createBufferUnique(bufferCreateInfo);

        if (initMemory) {
            auto memRequirements = m_device->GetDevice().getBufferMemoryRequirements(*m_buffer);
            m_bufferDeviceMemory.InitializeMemory(memRequirements, IsShaderDeviceAddress());
            m_bufferDeviceMemory.BindToBuffer(*this, 0);
        }
    }

    void Buffer::CopyBufferAsync(std::size_t srcOffset, const Buffer & dstBuffer, std::size_t dstOffset,
        std::size_t size, vk::CommandBuffer cmdBuffer) const
    {
        assert(m_usage & vk::BufferUsageFlagBits::eTransferSrc);
        assert(dstBuffer.m_usage & vk::BufferUsageFlagBits::eTransferDst);
        assert(srcOffset + size <= m_size);
        assert(dstOffset + size <= dstBuffer.m_size);

        vk::BufferCopy copyRegion{ srcOffset, dstOffset, size };
        cmdBuffer.copyBuffer(*m_buffer, *dstBuffer.m_buffer, copyRegion);
    }

    vk::UniqueCommandBuffer Buffer::CopyBufferAsync(std::size_t srcOffset, const Buffer& dstBuffer, std::size_t dstOffset,
        std::size_t size, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx, const std::vector<vk::Semaphore>& waitSemaphores,
        const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) const
    {
        auto transferCmdBuffer = CommandBuffers::beginSingleTimeSubmit(m_device, copyQueueIdx.first);
        CopyBufferAsync(srcOffset, dstBuffer, dstOffset, size, *transferCmdBuffer);
        CommandBuffers::endSingleTimeSubmit(m_device, *transferCmdBuffer, copyQueueIdx.first, copyQueueIdx.second,
            waitSemaphores, signalSemaphores, fence);

        return transferCmdBuffer;
    }

    vk::UniqueCommandBuffer Buffer::CopyBufferAsync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx,
        const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
        vk::Fence fence) const
    {
        return CopyBufferAsync(0, dstBuffer, 0, m_size, copyQueueIdx, waitSemaphores, signalSemaphores, fence);
    }

    void Buffer::CopyBufferSync(const Buffer& dstBuffer, std::pair<std::uint32_t, std::uint32_t> copyQueueIdx) const
    {
        auto cmdBuffer = CopyBufferAsync(dstBuffer, copyQueueIdx);
        m_device->GetQueue(copyQueueIdx.first, copyQueueIdx.second).waitIdle();
    }

    vk::DeviceOrHostAddressConstKHR Buffer::GetDeviceAddressConst() const
    {
        assert(IsShaderDeviceAddress());
        vk::BufferDeviceAddressInfoKHR bufferAddressInfo{ m_buffer.get() };
        return m_device->GetDevice().getBufferAddressKHR(bufferAddressInfo);
    }

    vk::DeviceOrHostAddressKHR Buffer::GetDeviceAddress()
    {
        assert(IsShaderDeviceAddress());
        vk::BufferDeviceAddressInfoKHR bufferAddressInfo{m_buffer.get()};
        return m_device->GetDevice().getBufferAddressKHR(bufferAddressInfo);
    }

}
