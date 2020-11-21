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

    AccelerationStructure::~AccelerationStructure() {}

    void AccelerationStructure::CreateAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& info)
    {
        m_vkAccelerationStructure = m_device->GetDevice().createAccelerationStructureKHRUnique(info);

        vk::AccelerationStructureMemoryRequirementsInfoKHR accStructureMemReqInfo{
            vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject, vk::AccelerationStructureBuildTypeKHR::eDevice,
            m_vkAccelerationStructure.get()};
        auto accStructureMemReq =
            m_device->GetDevice().getAccelerationStructureMemoryRequirementsKHR(accStructureMemReqInfo);

        vk::MemoryAllocateInfo memoryAllocateInfo{
            accStructureMemReq.memoryRequirements.size,
            vkfw_core::gfx::DeviceMemory::FindMemoryType(m_device, accStructureMemReq.memoryRequirements.memoryTypeBits,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal)};
        m_memory = m_device->GetDevice().allocateMemoryUnique(memoryAllocateInfo);

        vk::BindAccelerationStructureMemoryInfoKHR accStructureMemoryInfo{m_vkAccelerationStructure.get(),
                                                                          m_memory.get()};
        m_device->GetDevice().bindAccelerationStructureMemoryKHR(accStructureMemoryInfo);

        vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{m_vkAccelerationStructure.get()};
        m_handle = m_device->GetDevice().getAccelerationStructureAddressKHR(asDeviceAddressInfo);
    }

    std::unique_ptr<vkfw_core::gfx::DeviceBuffer> AccelerationStructure::CreateAccelerationStructureScratchBuffer() const
    {
        vk::AccelerationStructureMemoryRequirementsInfoKHR asMemoryReq{
            vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch,
            vk::AccelerationStructureBuildTypeKHR::eDevice, m_vkAccelerationStructure.get()};
        auto memReq = m_device->GetDevice().getAccelerationStructureMemoryRequirementsKHR(asMemoryReq);

        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> scratchBuffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device, vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        scratchBuffer->InitializeBuffer(memReq.memoryRequirements.size);

        return scratchBuffer;
    }

    void AccelerationStructure::BuildAccelerationStructure(
        const std::vector<vk::AccelerationStructureCreateGeometryTypeInfoKHR>& geometryTypeInfos,
        const std::vector<vk::AccelerationStructureGeometryKHR>& geometries,
        const vk::AccelerationStructureBuildOffsetInfoKHR* buildOffsets)
    {
        vk::AccelerationStructureCreateInfoKHR blasCreateInfo{ 0, m_type, m_flags, static_cast<std::uint32_t>(geometryTypeInfos.size()), geometryTypeInfos.data()};

        CreateAccelerationStructure(blasCreateInfo);
        auto scratchBuffer = CreateAccelerationStructureScratchBuffer();

        const vk::AccelerationStructureGeometryKHR* asGeometriesPtr = geometries.data();
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{m_type,
                                                                m_flags,
                                                                VK_FALSE,
                                                                {},
                                                                m_vkAccelerationStructure.get(),
                                                                VK_FALSE,
                                                                static_cast<std::uint32_t>(geometries.size()),
                                                                &asGeometriesPtr,
                                                                scratchBuffer->GetDeviceAddress()};

        if (m_device->GetDeviceRayTracingFeatures().rayTracingHostAccelerationStructureCommands) {
            if (m_device->GetDevice().buildAccelerationStructureKHR(
                    buildInfo,
                    vk::ArrayProxy<const vk::AccelerationStructureBuildOffsetInfoKHR* const>(1, &buildOffsets))
                != vk::Result::eSuccess) {
                spdlog::error("Could not build acceleration structure.");
                assert(false);
            }
        } else {
            auto vkAccStructureCmdBuffer = vkfw_core::gfx::CommandBuffers::beginSingleTimeSubmit(m_device, 0);
            vkAccStructureCmdBuffer->buildAccelerationStructureKHR(
                buildInfo, vk::ArrayProxy<const vk::AccelerationStructureBuildOffsetInfoKHR* const>(1, &buildOffsets));
            vkfw_core::gfx::CommandBuffers::endSingleTimeSubmitAndWait(m_device, vkAccStructureCmdBuffer.get(), 0, 0);
        }
    }

}
