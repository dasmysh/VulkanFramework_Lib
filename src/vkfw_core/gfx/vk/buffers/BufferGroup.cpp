/**
 * @file   BufferGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.11.27
 *
 * @brief  Implementation of the BufferGroup class.
 */

#include "gfx/vk/buffers/BufferGroup.h"
#include "gfx/vk/buffers/Buffer.h"
// ReSharper disable CppUnusedIncludeDirective
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/buffers/HostBuffer.h"
// ReSharper restore CppUnusedIncludeDirective
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vkfw_core::gfx {

    BufferGroup::BufferGroup(const LogicalDevice* device, const vk::MemoryPropertyFlags& memoryFlags) :
        m_device{ device },
        m_memoryProperties{ memoryFlags }
    {
    }


    BufferGroup::~BufferGroup()
    {
        m_hostBuffers.clear();
        m_hostBufferMemory.reset();
        m_deviceBuffers.clear();
        m_deviceBufferMemory.reset();
    }

    BufferGroup::BufferGroup(BufferGroup&& rhs) noexcept :
    m_device{ rhs.m_device },
        m_deviceBufferMemory{ std::move(rhs.m_deviceBufferMemory) },
        m_hostBufferMemory{ std::move(rhs.m_hostBufferMemory) },
        m_deviceBuffers{ std::move(rhs.m_deviceBuffers) },
        m_hostBuffers{ std::move(rhs.m_hostBuffers) },
        m_memoryProperties{ rhs.m_memoryProperties },
        m_bufferContents{ std::move(rhs.m_bufferContents) }
    {
    }

    BufferGroup& BufferGroup::operator=(BufferGroup&& rhs) noexcept
    {
        this->~BufferGroup();
        m_device = rhs.m_device;
        m_deviceBufferMemory = std::move(rhs.m_deviceBufferMemory);
        m_hostBufferMemory = std::move(rhs.m_hostBufferMemory);
        m_deviceBuffers = std::move(rhs.m_deviceBuffers);
        m_hostBuffers = std::move(rhs.m_hostBuffers);
        m_memoryProperties = rhs.m_memoryProperties;
        m_bufferContents = std::move(rhs.m_bufferContents);
        return *this;
    }

    unsigned BufferGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        m_deviceBuffers.emplace_back(m_device, vk::BufferUsageFlagBits::eTransferDst | usage, m_memoryProperties,
                                     queueFamilyIndices);
        m_deviceBuffers.back().InitializeBuffer(size, false);

        m_hostBuffers.emplace_back(m_device, vk::BufferUsageFlagBits::eTransferSrc, m_memoryProperties,
                                   queueFamilyIndices);
        m_hostBuffers.back().InitializeBuffer(size, false);

        m_bufferContents.emplace_back(0, nullptr);
        return static_cast<unsigned int>(m_deviceBuffers.size() - 1);
    }

    unsigned BufferGroup::AddBufferToGroup(const vk::BufferUsageFlags& usage, std::size_t size, const void* data, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(usage, size, queueFamilyIndices);
        m_bufferContents.back().first = size;
        m_bufferContents.back().second = data;
        return idx;
    }

    void BufferGroup::FinalizeGroup(QueuedDeviceTransfer* transfer)
    {
        vk::MemoryAllocateInfo deviceAllocInfo;
        vk::MemoryAllocateInfo hostAllocInfo;
        std::vector<std::uint32_t> deviceSizes;
        std::vector<std::uint32_t> hostSizes;
        for (auto i = 0U; i < m_deviceBuffers.size(); ++i) {
            FillAllocationInfo(&m_hostBuffers[i], hostAllocInfo, hostSizes);
            FillAllocationInfo(&m_deviceBuffers[i], deviceAllocInfo, deviceSizes);
        }
        m_hostBufferMemory = m_device->GetDevice().allocateMemoryUnique(hostAllocInfo);
        m_deviceBufferMemory = m_device->GetDevice().allocateMemoryUnique(deviceAllocInfo);

        auto deviceOffset = 0U;
        auto hostOffset = 0U;
        for (auto i = 0U; i < m_deviceBuffers.size(); ++i) {
            m_device->GetDevice().bindBufferMemory(m_hostBuffers[i].GetBuffer(), *m_hostBufferMemory, hostOffset);
            m_device->GetDevice().bindBufferMemory(m_deviceBuffers[i].GetBuffer(), *m_deviceBufferMemory, deviceOffset);
            if (transfer != nullptr) {
                auto deviceMem = m_device->GetDevice().mapMemory(*m_hostBufferMemory, hostOffset, m_bufferContents[i].first, vk::MemoryMapFlags());
                memcpy(deviceMem, m_bufferContents[i].second, m_bufferContents[i].first);
                m_device->GetDevice().unmapMemory(*m_hostBufferMemory);

                // hostBuffers_[i].UploadData(0, bufferContents_[i].first, bufferContents_[i].second);
                transfer->AddTransferToQueue(m_hostBuffers[i], m_deviceBuffers[i]);
            }
            hostOffset += hostSizes[i];
            deviceOffset += deviceSizes[i];

        }
    }

    void BufferGroup::FillAllocationInfo(Buffer* buffer, vk::MemoryAllocateInfo& allocInfo, std::vector<std::uint32_t>& sizes) const
    {
        auto memRequirements = m_device->GetDevice().getBufferMemoryRequirements(buffer->GetBuffer());
        if (allocInfo.allocationSize == 0) {
            allocInfo.memoryTypeIndex = DeviceMemory::FindMemoryType(m_device, memRequirements.memoryTypeBits,
                                                                     buffer->GetDeviceMemory().GetMemoryProperties());
        } else if (!DeviceMemory::CheckMemoryType(m_device, allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits,
                                                  buffer->GetDeviceMemory().GetMemoryProperties())) {
            spdlog::critical("BufferGroup memory type ({}) does not fit required memory type for buffer ({:#x}).", allocInfo.memoryTypeIndex, memRequirements.memoryTypeBits);
            throw std::runtime_error("BufferGroup memory type does not fit required memory type for buffer.");
        }

        sizes.push_back(static_cast<unsigned int>(memRequirements.size));
        allocInfo.allocationSize += memRequirements.size;
    }
}
