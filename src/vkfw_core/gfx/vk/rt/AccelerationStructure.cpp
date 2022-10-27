/**
 * @file   AccelerationStructure.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.21
 *
 * @brief  Implementation of the acceleration structure class.
 */

#include "gfx/vk/rt/AccelerationStructure.h"
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/DescriptorSet.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructure::AccelerationStructure(vkfw_core::gfx::LogicalDevice* device, std::string_view name,
                                                 vk::AccelerationStructureTypeKHR type,
                                                 vk::BuildAccelerationStructureFlagsKHR flags)
        : VulkanObjectPrivateWrapper{device->GetHandle(), name, vk::UniqueAccelerationStructureKHR{}}
        , m_device{device}
        , m_type{type}
        , m_flags{flags}
        , m_buildBarrier{device}
    {
    }

    AccelerationStructure::AccelerationStructure(AccelerationStructure&& rhs) noexcept = default;

    AccelerationStructure& AccelerationStructure::operator=(AccelerationStructure&& rhs) noexcept = default;

    AccelerationStructure::~AccelerationStructure() {}

    void AccelerationStructure::AddGeometry(const vk::AccelerationStructureGeometryKHR& geometry,
                                            const vk::AccelerationStructureBuildRangeInfoKHR& buildRange)
    {
        m_geometries.emplace_back(geometry);
        m_buildRanges.emplace_back(buildRange);
    }

    void AccelerationStructure::BuildAccelerationStructure(CommandBuffer& cmdBuffer)
    {
        vk::AccelerationStructureBuildGeometryInfoKHR asBuildInfo{ m_type, m_flags, vk::BuildAccelerationStructureModeKHR::eBuild,
            nullptr, nullptr, static_cast<std::uint32_t>(m_geometries.size()), m_geometries.data() };

        std::vector<std::uint32_t> asPrimitiveCounts;
        asPrimitiveCounts.reserve(m_geometries.size());
        for (const auto& buildRange : m_buildRanges) {
            asPrimitiveCounts.push_back(buildRange.primitiveCount);
        }

        m_memoryRequirements = m_device->GetHandle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, asBuildInfo, asPrimitiveCounts);

        CreateAccelerationStructure(m_buildBarrier);

        m_scratchBuffer = CreateAccelerationStructureScratchBuffer();

        asBuildInfo.dstAccelerationStructure = GetHandle();
        asBuildInfo.scratchData = m_device->CalculateASScratchBufferBufferAlignment(
            m_scratchBuffer
                ->GetDeviceAddress(vk::AccessFlagBits2KHR::eAccelerationStructureReadKHR
                                       | vk::AccessFlagBits2KHR::eAccelerationStructureWriteKHR,
                                   vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuildKHR, m_buildBarrier)
                .deviceAddress);
        m_buildBarrier.Record(cmdBuffer);

        cmdBuffer.GetHandle().buildAccelerationStructuresKHR(asBuildInfo, m_buildRanges.data());
    }

    void AccelerationStructure::FinalizeBuild() { m_scratchBuffer = nullptr; }

    vk::DeviceAddress AccelerationStructure::GetAddressHandle(vk::AccessFlags2KHR access,
                                                              vk::PipelineStageFlags2KHR pipelineStages,
                                                              PipelineBarrier& barrier) const
    {
        m_buffer->AccessBarrier(false, access, pipelineStages, barrier);
        return m_handle;
    }

    void AccelerationStructure::CreateAccelerationStructure(PipelineBarrier& barrier)
    {
        m_buffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device, fmt::format("ASBuffer:{}", GetName()),
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        m_buffer->InitializeBuffer(m_memoryRequirements.accelerationStructureSize);

        vk::AccelerationStructureCreateInfoKHR asCreateInfo{
            {},
            m_buffer->GetBuffer(false, vk::AccessFlagBits2KHR::eAccelerationStructureWriteKHR,
                                vk::PipelineStageFlagBits2KHR::eAccelerationStructureBuildKHR, barrier),
            0,
            m_memoryRequirements.accelerationStructureSize,
            m_type};

        SetHandle(m_device->GetHandle(), m_device->GetHandle().createAccelerationStructureKHRUnique(asCreateInfo));

        vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{GetHandle()};
        m_handle = m_device->GetHandle().getAccelerationStructureAddressKHR(asDeviceAddressInfo);
    }

    void AccelerationStructure::AccessBarrier(vk::AccessFlags2KHR access, vk::PipelineStageFlags2KHR pipelineStages,
                                              PipelineBarrier& barrier) const
    {
        m_buffer->AccessBarrier(false, access, pipelineStages, barrier);
    }

    std::unique_ptr<vkfw_core::gfx::DeviceBuffer> AccelerationStructure::CreateAccelerationStructureScratchBuffer() const
    {
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> scratchBuffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device, fmt::format("ASScratchBuffer:{}", GetName()),
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        scratchBuffer->InitializeBuffer(
            m_device->CalculateASScratchBufferBufferAlignment(m_memoryRequirements.buildScratchSize));

        return scratchBuffer;
    }

}
