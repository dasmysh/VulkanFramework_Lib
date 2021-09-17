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
#include "gfx/vk/CommandBuffers.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructure::AccelerationStructure(vkfw_core::gfx::LogicalDevice* device,
                                                 vk::AccelerationStructureTypeKHR type,
                                                 vk::BuildAccelerationStructureFlagsKHR flags)
        : m_device{device}, m_type{type}, m_flags{flags}
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

        m_memoryRequirements = m_device->GetDevice().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, asBuildInfo, asPrimitiveCounts);

        CreateAccelerationStructure();

        auto scratchBuffer = CreateAccelerationStructureScratchBuffer();

        asBuildInfo.dstAccelerationStructure = m_vkAccelerationStructure.get();
        asBuildInfo.scratchData =
            m_device->CalculateASScratchBufferBufferAlignment(scratchBuffer->GetDeviceAddress().deviceAddress);

        if (m_device->GetDeviceAccelerationStructureFeatures().accelerationStructureHostCommands) {
            if (m_device->GetDevice().buildAccelerationStructuresKHR(vk::DeferredOperationKHR{}, asBuildInfo,
                                                                     m_buildRanges.data())
                != vk::Result::eSuccess) {
                spdlog::error("Could not build acceleration structure.");
                assert(false);
            }
        } else {
            auto vkAccStructureCmdBuffer = vkfw_core::gfx::CommandBuffers::beginSingleTimeSubmit(m_device, 0);
            vkAccStructureCmdBuffer->buildAccelerationStructuresKHR(asBuildInfo, m_buildRanges.data());
            vkfw_core::gfx::CommandBuffers::endSingleTimeSubmitAndWait(m_device, vkAccStructureCmdBuffer.get(), 0, 0);
        }
    }

    void AccelerationStructure::CreateAccelerationStructure()
    {
        m_buffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        m_buffer->InitializeBuffer(m_memoryRequirements.accelerationStructureSize);

        vk::AccelerationStructureCreateInfoKHR asCreateInfo{
            {}, m_buffer->GetBuffer(), 0, m_memoryRequirements.accelerationStructureSize, m_type};

        m_vkAccelerationStructure = m_device->GetDevice().createAccelerationStructureKHRUnique(asCreateInfo);

        vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{m_vkAccelerationStructure.get()};
        m_handle = m_device->GetDevice().getAccelerationStructureAddressKHR(asDeviceAddressInfo);
    }

    std::unique_ptr<vkfw_core::gfx::DeviceBuffer> AccelerationStructure::CreateAccelerationStructureScratchBuffer() const
    {
        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> scratchBuffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        scratchBuffer->InitializeBuffer(
            m_device->CalculateASScratchBufferBufferAlignment(m_memoryRequirements.buildScratchSize));

        return scratchBuffer;
    }

}
