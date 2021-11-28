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
        , m_device{device}
        , m_bufferDeviceMemory{device, fmt::format("BufferMem:{}", GetName()), memoryFlags}
        , m_size{0}
        , m_usage{usage}
        , m_queueFamilyIndices{std::move(queueFamilyIndices)}
        , m_bufferRangeAccess{m_size}
    {
    }

    Buffer::~Buffer() = default;

    Buffer::Buffer(Buffer&& rhs) noexcept
        : VulkanObjectPrivateWrapper{std::move(rhs)}
        , m_device{rhs.m_device}
        , m_bufferDeviceMemory{std::move(rhs.m_bufferDeviceMemory)}
        , m_size{rhs.m_size}
        , m_usage{rhs.m_usage}
        , m_queueFamilyIndices{std::move(rhs.m_queueFamilyIndices)}
        , m_bufferRangeAccess{std::move(rhs.m_bufferRangeAccess)}
    {
        rhs.m_size = 0;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        this->~Buffer();
        VulkanObjectPrivateWrapper::operator=(std::move(rhs));
        m_device = rhs.m_device;
        m_bufferDeviceMemory = std::move(rhs.m_bufferDeviceMemory);
        m_size = rhs.m_size;
        m_usage = rhs.m_usage;
        m_queueFamilyIndices = std::move(rhs.m_queueFamilyIndices);
        m_bufferRangeAccess = std::move(rhs.m_bufferRangeAccess);
        rhs.m_size = 0;
        return *this;
    }

    void Buffer::InitializeBuffer(std::size_t size, bool initMemory)
    {
        // not sure if this is needed again at some point.
        assert(!GetHandle());

        m_size = size;
        m_bufferRangeAccess = BufferAccessRangeList{m_size};
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

    void Buffer::CopyBufferAsync(std::size_t srcOffset, Buffer& dstBuffer, std::size_t dstOffset, std::size_t size,
                                 CommandBuffer& cmdBuffer)
    {
        assert(m_usage & vk::BufferUsageFlagBits::eTransferSrc);
        assert(dstBuffer.m_usage & vk::BufferUsageFlagBits::eTransferDst);
        assert(srcOffset + size <= m_size);
        assert(dstOffset + size <= dstBuffer.m_size);

        PipelineBarrier barrier{m_device};
        AccessBarrierRange(false, srcOffset, size, vk::AccessFlagBits2KHR::eTransferRead,
                           vk::PipelineStageFlagBits2KHR::eTransfer, barrier);
        dstBuffer.AccessBarrierRange(false, dstOffset, size, vk::AccessFlagBits2KHR::eTransferWrite,
                                     vk::PipelineStageFlagBits2KHR::eTransfer, barrier);
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
        auto submitFence =
            CommandBuffer::endSingleTimeSubmit(copyQueue, transferCmdBuffer, waitSemaphores, signalSemaphores);
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

    void Buffer::AccessBarrier(bool isDynamic, vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                               PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, GetHandle(), isDynamic, access, pipelineStages);
    }

    void Buffer::AccessBarrierRange(bool isDynamic, std::size_t offset, std::size_t range, vk::AccessFlags2KHR access,
                                    vk::PipelineStageFlags2KHR pipelineStages, PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(BufferRange{this, offset, range}, GetHandle(), isDynamic, access, pipelineStages);
    }

    vk::Buffer Buffer::GetBuffer(bool isDynamic, vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                 PipelineBarrier& barrier)
    {
        barrier.AddSingleBarrier(this, GetHandle(), isDynamic, access, pipelineStages);
        return GetHandle();
    }

    BufferRange Buffer::GetBufferRange(bool isDynamic, std::size_t offset, std::size_t range,
                                       vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                       PipelineBarrier& barrier)
    {
        BufferRange result{this, offset, range};
        barrier.AddSingleBarrier(result, GetHandle(), isDynamic, access, pipelineStages);
        return result;
    }

    // const vk::Buffer* Buffer::GetBufferPtr(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
    //                                  PipelineBarrier& barrier)
    // {
    //     barrier.AddSingleBarrier(this, GetHandle(), access, pipelineStages);
    //     return GetHandlePtr();
    // }

    vk::MemoryRequirements Buffer::GetMemoryRequirements() const
    {
        return m_device->GetHandle().getBufferMemoryRequirements(GetHandle());
    }

    vk::DeviceOrHostAddressConstKHR Buffer::GetDeviceAddressConst(vk::AccessFlags2KHR access,
                                                                  vk::PipelineStageFlags2KHR pipelineStages,
                                                                  PipelineBarrier& barrier)
    {
        assert(IsShaderDeviceAddress());
        barrier.AddSingleBarrier(this, GetHandle(), false, access, pipelineStages);
        vk::BufferDeviceAddressInfo bufferAddressInfo{GetHandle()};
        return m_device->GetHandle().getBufferAddress(bufferAddressInfo);
    }

    vk::DeviceOrHostAddressKHR Buffer::GetDeviceAddress(vk::AccessFlags2KHR access,
                                                        vk::PipelineStageFlags2KHR pipelineStages,
                                                        PipelineBarrier& barrier)
    {
        assert(IsShaderDeviceAddress());
        barrier.AddSingleBarrier(this, GetHandle(), false, access, pipelineStages);
        vk::BufferDeviceAddressInfo bufferAddressInfo{GetHandle()};
        return m_device->GetHandle().getBufferAddress(bufferAddressInfo);
    }

    void Buffer::BindMemory(vk::DeviceMemory deviceMemory, std::size_t offset)
    {
        m_device->GetHandle().bindBufferMemory(GetHandle(), deviceMemory, offset);
    }

    std::span<BufferAccessRange> Buffer::GetOverlappingRanges(std::size_t offset, std::size_t range)
    {
        return m_bufferRangeAccess.GetOverlappingRanges(offset, range);
    }

    void Buffer::SetAccess(std::size_t offset, std::size_t range, vk::AccessFlags2KHR access,
                           vk::PipelineStageFlags2KHR pipelineStages, unsigned int queueFamily)
    {
        m_bufferRangeAccess.SetAccess(offset, range, access, pipelineStages, queueFamily);
    }

    void Buffer::ReduceRanges() { m_bufferRangeAccess.ReduceRanges(); }

    std::span<BufferAccessRange> BufferAccessRangeList::GetOverlappingRanges(std::size_t offset, std::size_t range)
    {
        std::size_t firstIndex = 0;
        for (; firstIndex < m_bufferRangeAccess.size(); ++firstIndex) {
            const auto& bufferRange = m_bufferRangeAccess[firstIndex];
            if (offset >= bufferRange.m_offset + bufferRange.m_range) continue;
            break;
        }

        if (m_bufferRangeAccess[firstIndex].m_offset < offset) {
            auto newRange = m_bufferRangeAccess[firstIndex].SplitAt(offset);
            firstIndex += 1;
            m_bufferRangeAccess.insert(m_bufferRangeAccess.begin() + firstIndex, newRange);
        }

        std::size_t lastIndex = firstIndex;
        auto rangeEnd = offset + range;
        for (; lastIndex < m_bufferRangeAccess.size(); ++lastIndex) {
            const auto& bufferRange = m_bufferRangeAccess[lastIndex];
            if (rangeEnd > bufferRange.m_offset + bufferRange.m_range) continue;
            break;
        }

        if (m_bufferRangeAccess[lastIndex].m_offset + m_bufferRangeAccess[lastIndex].m_range > rangeEnd) {
            auto newRange = m_bufferRangeAccess[lastIndex].SplitAt(rangeEnd);
            lastIndex += 1;
            m_bufferRangeAccess.insert(m_bufferRangeAccess.begin() + lastIndex, newRange);
        }

        return std::span{m_bufferRangeAccess.begin() + firstIndex, m_bufferRangeAccess.begin() + lastIndex};
    }

    void BufferAccessRangeList::SetAccess(std::size_t offset, std::size_t range, vk::AccessFlags2KHR access,
                                          vk::PipelineStageFlags2KHR pipelineStages, unsigned int queueFamily)
    {
        std::size_t firstIndex = 0;
        for (; firstIndex < m_bufferRangeAccess.size(); ++firstIndex) {
            const auto& bufferRange = m_bufferRangeAccess[firstIndex];
            if (offset >= bufferRange.m_offset + bufferRange.m_range) continue;
            break;
        }

        assert(m_bufferRangeAccess[firstIndex].m_offset == offset);

        std::size_t lastIndex = firstIndex;
        auto rangeEnd = offset + range;
        for (; lastIndex < m_bufferRangeAccess.size(); ++lastIndex) {
            const auto& bufferRange = m_bufferRangeAccess[lastIndex];
            m_bufferRangeAccess[lastIndex].SetAccess(access, pipelineStages, queueFamily);
            if (rangeEnd > bufferRange.m_offset + bufferRange.m_range) continue;
            break;
        }

        assert(m_bufferRangeAccess[lastIndex].m_offset + m_bufferRangeAccess[lastIndex].m_range == rangeEnd);
    }

    void BufferAccessRangeList::ReduceRanges()
    {
        for (std::size_t i = 1; i < m_bufferRangeAccess.size();) {
            auto& firstRange = m_bufferRangeAccess[i - 1];
            const auto& secondRange = m_bufferRangeAccess[i];
            if (firstRange.HasEqualAccess(secondRange)) {
                firstRange.m_range = secondRange.m_offset + secondRange.m_range - firstRange.m_offset;
                m_bufferRangeAccess.erase(m_bufferRangeAccess.begin() + i);
            } else {
                i += 1;
            }
        }
    }

    BufferAccessRange BufferAccessRange::SplitAt(std::size_t offset)
    {
        assert(offset > m_offset && m_offset + m_range > offset);
        auto totalRange = m_range;
        m_range = offset - m_offset;


        BufferAccessRange result = *this;
        result.m_offset = offset;
        result.m_range = m_offset + totalRange - offset;
        return result;
    }

}
