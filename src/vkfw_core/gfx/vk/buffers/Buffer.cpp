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
        : VulkanObjectPrivateWrapper{nullptr, name, vk::UniqueBuffer{}}
        , MemoryBoundResource{device}
        , m_bufferDeviceMemory{device, fmt::format("BufferMem:{}", GetName()), memoryFlags}
        , m_size{0}
        , m_usage{usage}
        , m_queueFamilyIndices{std::move(queueFamilyIndices)}
    {
    }

    Buffer::~Buffer() = default;

    Buffer::Buffer(Buffer&& rhs) noexcept
        : VulkanObjectPrivateWrapper{std::move(rhs)}
        , MemoryBoundResource{std::move(rhs)}
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
        VulkanObjectPrivateWrapper::operator=(std::move(rhs));
        MemoryBoundResource::operator=(std::move(rhs));
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
            BindMemory(m_bufferDeviceMemory.GetHandle(), 0);
        }
    }

    void Buffer::CopyBufferAsync(std::size_t srcOffset, Buffer& dstBuffer, std::size_t dstOffset,
                                 std::size_t size, CommandBuffer& cmdBuffer)
    {
        assert(m_usage & vk::BufferUsageFlagBits::eTransferSrc);
        assert(dstBuffer.m_usage & vk::BufferUsageFlagBits::eTransferDst);
        assert(srcOffset + size <= m_size);
        assert(dstOffset + size <= dstBuffer.m_size);

        PipelineBarrier barrier{m_device};
        AccessBarrier(vk::AccessFlagBits2KHR::eTransferRead, vk::PipelineStageFlagBits2KHR::eTransfer, barrier);
        dstBuffer.AccessBarrier(vk::AccessFlagBits2KHR::eTransferWrite, vk::PipelineStageFlagBits2KHR::eTransfer, barrier);
        barrier.Record(cmdBuffer);

        vk::BufferCopy copyRegion{srcOffset, dstOffset, size};
        cmdBuffer.GetHandle().copyBuffer(GetHandle(), dstBuffer.GetHandle(), copyRegion);
    }

    CommandBuffer Buffer::CopyBufferAsync(std::size_t srcOffset, Buffer& dstBuffer, std::size_t dstOffset,
                                          std::size_t size, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores,
                                          std::optional<std::reference_wrapper<std::shared_ptr<Fence>>> fence)
    {
        auto transferCmdBuffer = CommandBuffer::beginSingleTimeSubmit(
            m_device, fmt::format("CopyCMDBuffer {}", GetName()),
            fmt::format("Copy {} to {}", GetName(), dstBuffer.GetName()), copyQueue.GetCommandPool());
        CopyBufferAsync(srcOffset, dstBuffer, dstOffset, size, transferCmdBuffer);
        auto submitFence = CommandBuffer::endSingleTimeSubmit(copyQueue, transferCmdBuffer, waitSemaphores, signalSemaphores);
        if (fence.has_value()) { fence = submitFence; }

        return transferCmdBuffer;
    }

    CommandBuffer Buffer::CopyBufferAsync(Buffer& dstBuffer, const Queue& copyQueue,
                                          std::span<vk::Semaphore> waitSemaphores,
                                          std::span<vk::Semaphore> signalSemaphores,
                                          std::optional<std::reference_wrapper<std::shared_ptr<Fence>>> fence)
    {
        return CopyBufferAsync(0, dstBuffer, 0, m_size, copyQueue, waitSemaphores, signalSemaphores, fence);
    }

    void Buffer::CopyBufferSync(Buffer& dstBuffer, const Queue& copyQueue)
    {
        auto cmdBuffer = CopyBufferAsync(dstBuffer, copyQueue);
        copyQueue.WaitIdle();
    }

    void Buffer::AccessBarrier(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                               PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, GetHandle(), access, pipelineStages);
    }

    vk::Buffer Buffer::GetBuffer(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                 PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, GetHandle(), access, pipelineStages);
        return GetHandle();
    }

    const vk::Buffer* Buffer::GetBufferPtr(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                     PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, GetHandle(), access, pipelineStages);
        return GetHandlePtr();
    }

    vk::MemoryRequirements Buffer::GetMemoryRequirements() const
    {
        return m_device->GetHandle().getBufferMemoryRequirements(GetHandle());
    }

    vk::DeviceOrHostAddressConstKHR Buffer::GetDeviceAddressConst(vk::AccessFlags2KHR access,
                                                                  vk::PipelineStageFlags2KHR pipelineStages,
                                                                  PipelineBarrier& barrier)
    {
        assert(IsShaderDeviceAddress());
        barrier.AddSingleBarrier(this, GetHandle(), access, pipelineStages);
        vk::BufferDeviceAddressInfo bufferAddressInfo{GetHandle()};
        return m_device->GetHandle().getBufferAddress(bufferAddressInfo);
    }

    vk::DeviceOrHostAddressKHR Buffer::GetDeviceAddress(vk::AccessFlags2KHR access,
                                                        vk::PipelineStageFlags2KHR pipelineStages,
                                                        PipelineBarrier& barrier)
    {
        assert(IsShaderDeviceAddress());
        barrier.AddSingleBarrier(this, GetHandle(), access, pipelineStages);
        vk::BufferDeviceAddressInfo bufferAddressInfo{GetHandle()};
        return m_device->GetHandle().getBufferAddress(bufferAddressInfo);
    }

    void Buffer::BindMemory(vk::DeviceMemory deviceMemory, std::size_t offset)
    {
        m_device->GetHandle().bindBufferMemory(GetHandle(), deviceMemory, offset);
    }

}
