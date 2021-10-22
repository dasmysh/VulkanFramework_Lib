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
        : VulkanObjectWrapper{device->GetHandle(), name, vk::UniqueAccelerationStructureKHR{}}
        , m_device{device}
        , m_type{type}
        , m_flags{flags}
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

    void AccelerationStructure::BuildAccelerationStructure()
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

        CreateAccelerationStructure();

        auto scratchBuffer = CreateAccelerationStructureScratchBuffer();

        asBuildInfo.dstAccelerationStructure = GetHandle();
        asBuildInfo.scratchData =
            m_device->CalculateASScratchBufferBufferAlignment(scratchBuffer->GetDeviceAddress().deviceAddress);

        if (m_device->GetDeviceAccelerationStructureFeatures().accelerationStructureHostCommands) {
            if (m_device->GetHandle().buildAccelerationStructuresKHR(vk::DeferredOperationKHR{}, asBuildInfo,
                                                                     m_buildRanges.data())
                != vk::Result::eSuccess) {
                spdlog::error("Could not build acceleration structure.");
                assert(false);
            }
        } else {
            auto accStructureCmdBuffer = vkfw_core::gfx::CommandBuffer::beginSingleTimeSubmit(
                m_device, fmt::format("{} BuildCMDBuffer", GetName()), fmt::format("{} BuildAS", GetName()),
                m_device->GetCommandPool(0));
            accStructureCmdBuffer.GetHandle().buildAccelerationStructuresKHR(asBuildInfo, m_buildRanges.data());
            vkfw_core::gfx::CommandBuffer::endSingleTimeSubmitAndWait(m_device, m_device->GetQueue(0, 0),
                                                                      accStructureCmdBuffer);
        }
    }

    void AccelerationStructure::FillBufferRange(BufferRange& bufferRange) const
    {
        bufferRange.m_buffer = m_buffer.get();
        bufferRange.m_offset = 0;
        bufferRange.m_range = m_buffer->GetSize();
    }

    void AccelerationStructure::CreateAccelerationStructure()
    {
        m_buffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device, fmt::format("ASBuffer:{}", GetName()),
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        m_buffer->InitializeBuffer(m_memoryRequirements.accelerationStructureSize);

        vk::AccelerationStructureCreateInfoKHR asCreateInfo{
            {}, m_buffer->GetHandle(), 0, m_memoryRequirements.accelerationStructureSize, m_type};

        SetHandle(m_device->GetHandle(), m_device->GetHandle().createAccelerationStructureKHRUnique(asCreateInfo));

        vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{GetHandle()};
        m_handle = m_device->GetHandle().getAccelerationStructureAddressKHR(asDeviceAddressInfo);
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
