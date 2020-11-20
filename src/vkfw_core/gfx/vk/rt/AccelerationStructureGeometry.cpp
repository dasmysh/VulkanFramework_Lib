/**
 * @file   AccelerationStructureGeometry.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Implementation of the acceleration structure geometry class.
 */

#include "gfx/vk//rt/AccelerationStructureGeometry.h"
#include "gfx/vk/buffers/DeviceBuffer.h"
#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/memory/DeviceMemory.h"
#include "gfx/vk/CommandBuffers.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx::rt {

    AccelerationStructureGeometry::AccelerationStructureGeometry(vkfw_core::gfx::LogicalDevice* device)
        : m_device{device}
    {
        auto features = m_device->GetPhysicalDevice()
                            .getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceRayTracingFeaturesKHR>();
        m_raytracingFeatures = features.get<vk::PhysicalDeviceRayTracingFeaturesKHR>();
    }

    AccelerationStructureGeometry::~AccelerationStructureGeometry() {}

    void AccelerationStructureGeometry::InitializeAccelerationStructure()
    {
        CreateBottomLevelAccelerationStructure();
        CreateTopLevelAccelerationStructure();
    }

    void AccelerationStructureGeometry::FillDescriptorLayoutBinding(vk::DescriptorSetLayoutBinding& asLayoutBinding,
                                                                    const vk::ShaderStageFlags& shaderFlags,
                                                                    std::uint32_t binding /*= 0*/) const
    {
        asLayoutBinding.setBinding(binding);
        asLayoutBinding.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        asLayoutBinding.setDescriptorCount(1);
        asLayoutBinding.setStageFlags(shaderFlags);
    }

    void AccelerationStructureGeometry::CreateLayout(vk::DescriptorPool descPool,
                                                     const vk::ShaderStageFlags& shaderFlags,
                                                     std::uint32_t binding /*= 0*/)
    {
        m_descBinding = binding;
        vk::DescriptorSetLayoutBinding asLayoutBinding;
        FillDescriptorLayoutBinding(asLayoutBinding, shaderFlags, binding);

        vk::DescriptorSetLayoutCreateInfo asLayoutCreateInfo{vk::DescriptorSetLayoutCreateFlags(), 1, &asLayoutBinding};
        m_internalDescLayout = m_device->GetDevice().createDescriptorSetLayoutUnique(asLayoutCreateInfo);
        m_descLayout = *m_internalDescLayout;
        AllocateDescriptorSet(descPool);
    }

    void AccelerationStructureGeometry::UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout,
                                                  std::uint32_t binding /*= 0*/)
    {
        m_descBinding = binding;
        m_descLayout = usedLayout;
        AllocateDescriptorSet(descPool);
    }

    void AccelerationStructureGeometry::UseDescriptorSet(vk::DescriptorSet descSet, vk::DescriptorSetLayout usedLayout,
                                                         std::uint32_t binding /*= 0*/)
    {
        m_descBinding = binding;
        m_descLayout = usedLayout;
        m_descSet = descSet;
    }

    void AccelerationStructureGeometry::FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const
    {
        descWrite.dstSet = m_descSet;
        descWrite.dstBinding = m_descBinding;
        descWrite.dstArrayElement = 0;
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
        descWrite.setPNext(&m_descriptorSetAccStructure);
    }

    void AccelerationStructureGeometry::AddTriangleGeometry(
        std::size_t primitiveCount, std::size_t vertexCount, std::size_t vertexSize,
        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress,
        vk::DeviceOrHostAddressConstKHR transformDeviceAddress /*= nullptr*/)
    {
        vk::Bool32 hasTransform = (transformDeviceAddress.hostAddress == nullptr) ? VK_FALSE : VK_TRUE;
        m_accelerationStructureCreateGeometryTypeInfo.emplace_back(
            vk::GeometryTypeKHR::eTriangles, static_cast<std::uint32_t>(primitiveCount), vk::IndexType::eUint32,
            static_cast<std::uint32_t>(vertexCount), vk::Format::eR32G32B32Sfloat, hasTransform);

        vk::AccelerationStructureGeometryTrianglesDataKHR asGeometryDataTriangles{
            vk::Format::eR32G32B32Sfloat, vertexBufferDeviceAddress, vertexSize, vk::IndexType::eUint32,
            indexBufferDeviceAddress, transformDeviceAddress};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataTriangles};

        m_accelerationStructureGeometries.emplace_back(vk::GeometryTypeKHR::eTriangles, asGeometryData,
                                                       vk::GeometryFlagBitsKHR::eOpaque);
        m_accelerationStructureBuildOffsets.emplace_back(static_cast<std::uint32_t>(primitiveCount), 0x0, 0, 0x0);
    }

    void AccelerationStructureGeometry::CreateBottomLevelAccelerationStructure()
    {
        vk::AccelerationStructureCreateInfoKHR blasCreateInfo{
            0, vk::AccelerationStructureTypeKHR::eBottomLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            static_cast<std::uint32_t>(m_accelerationStructureCreateGeometryTypeInfo.size()),
            m_accelerationStructureCreateGeometryTypeInfo.data()};
        m_BLAS = CreateAS(blasCreateInfo);

        vk::AccelerationStructureGeometryKHR* asGeometriesPtr = m_accelerationStructureGeometries.data();
        auto scratchBuffer = CreateASScratchBuffer(m_BLAS);

        vk::AccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            VK_FALSE,
            {},
            m_BLAS.as.get(),
            VK_FALSE,
            static_cast<std::uint32_t>(m_accelerationStructureGeometries.size()),
            &asGeometriesPtr,
            scratchBuffer->GetDeviceAddress()};

        // the parameter in buildAccelerationStructureKHR wants a pointer to n arrays of vk::AccelerationStructureBuildOffsetInfoKHR,
        // where n is the number of vk::AccelerationStructureBuildGeometryInfoKHR passed.
        auto accelerationStructureBuildOffsetsPtr = m_accelerationStructureBuildOffsets.data();

        if (m_raytracingFeatures.rayTracingHostAccelerationStructureCommands) {
            if (m_device->GetDevice().buildAccelerationStructureKHR(
                    asBuildGeometryInfo, vk::ArrayProxy<const vk::AccelerationStructureBuildOffsetInfoKHR* const>(
                                             1, &accelerationStructureBuildOffsetsPtr))
                != vk::Result::eSuccess) {
                spdlog::error("Could not build acceleration structure.");
                assert(false);
            }
        } else {
            auto vkAccStructureCmdBuffer = vkfw_core::gfx::CommandBuffers::beginSingleTimeSubmit(m_device, 0);
            vkAccStructureCmdBuffer->buildAccelerationStructureKHR(
                asBuildGeometryInfo, vk::ArrayProxy<const vk::AccelerationStructureBuildOffsetInfoKHR* const>(
                                         1, &accelerationStructureBuildOffsetsPtr));
            vkfw_core::gfx::CommandBuffers::endSingleTimeSubmitAndWait(m_device, vkAccStructureCmdBuffer.get(), 0,
                                                                       0);
        }
    }

    void AccelerationStructureGeometry::CreateTopLevelAccelerationStructure()
    {
        vk::AccelerationStructureCreateGeometryTypeInfoKHR tlasCreateGeometryInfo{
            vk::GeometryTypeKHR::eInstances, 1, vk::IndexType::eUint32, 0, vk::Format::eUndefined, VK_FALSE};
        vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{
            0, vk::AccelerationStructureTypeKHR::eTopLevel, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            1, &tlasCreateGeometryInfo};

        m_TLAS = CreateAS(tlasCreateInfo);

        vk::TransformMatrixKHR transform_matrix{std::array<std::array<float, 4>, 3>{
            std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f}, std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.0f},
            std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.0f}}};

        vk::AccelerationStructureInstanceKHR tlasInstance{
            transform_matrix, 0, 0xFF, 0, vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable, m_BLAS.handle};

        vkfw_core::gfx::HostBuffer instancesBuffer{m_device, vk::BufferUsageFlagBits::eShaderDeviceAddress};
        instancesBuffer.InitializeData(sizeof(vk::AccelerationStructureInstanceKHR),
                                       static_cast<const void*>(&tlasInstance));

        vk::AccelerationStructureGeometryInstancesDataKHR asGeometryDataInstances{
            VK_FALSE, instancesBuffer.GetDeviceAddressConst()};
        vk::AccelerationStructureGeometryDataKHR asGeometryData{asGeometryDataInstances};

        std::vector<vk::AccelerationStructureGeometryKHR> asGeometries;
        asGeometries.emplace_back(vk::GeometryTypeKHR::eInstances, asGeometryData, vk::GeometryFlagBitsKHR::eOpaque);
        vk::AccelerationStructureGeometryKHR* asGeometriesPtr = asGeometries.data();
        auto scratchBuffer = CreateASScratchBuffer(m_TLAS);

        vk::AccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{
            vk::AccelerationStructureTypeKHR::eTopLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            VK_FALSE,
            {},
            m_TLAS.as.get(),
            VK_FALSE,
            1,
            &asGeometriesPtr,
            scratchBuffer->GetDeviceAddress()};

        vk::AccelerationStructureBuildOffsetInfoKHR asBuildOffset{1, 0x0, 0, 0x0};
        std::vector<vk::AccelerationStructureBuildOffsetInfoKHR*> asBuildOffsets{&asBuildOffset};

        if (m_raytracingFeatures.rayTracingHostAccelerationStructureCommands) {
            if (m_device->GetDevice().buildAccelerationStructureKHR(
                    asBuildGeometryInfo,
                    vk::ArrayProxy<const vk::AccelerationStructureBuildOffsetInfoKHR* const>(1, asBuildOffsets.data()))
                != vk::Result::eSuccess) {
                spdlog::error("Could not build acceleration structure.");
                assert(false);
            }
        } else {
            auto vkAccStructureCmdBuffer = vkfw_core::gfx::CommandBuffers::beginSingleTimeSubmit(m_device, 0);
            vkAccStructureCmdBuffer->buildAccelerationStructureKHR(
                asBuildGeometryInfo,
                vk::ArrayProxy<const vk::AccelerationStructureBuildOffsetInfoKHR* const>(1, asBuildOffsets.data()));
            vkfw_core::gfx::CommandBuffers::endSingleTimeSubmitAndWait(m_device, vkAccStructureCmdBuffer.get(), 0,
                                                                       0);
        }

        m_descriptorSetAccStructure.setAccelerationStructureCount(1);
        m_descriptorSetAccStructure.setPAccelerationStructures(&*m_TLAS.as);
    }

    AccelerationStructureGeometry::AccelerationStructure AccelerationStructureGeometry::CreateAS(const vk::AccelerationStructureCreateInfoKHR& info)
    {
        AccelerationStructure result;

        result.as = m_device->GetDevice().createAccelerationStructureKHRUnique(info);

        vk::AccelerationStructureMemoryRequirementsInfoKHR accStructureMemReqInfo{
            vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject, vk::AccelerationStructureBuildTypeKHR::eDevice,
            result.as.get()};
        auto accStructureMemReq =
            m_device->GetDevice().getAccelerationStructureMemoryRequirementsKHR(accStructureMemReqInfo);

        vk::MemoryAllocateInfo memoryAllocateInfo{accStructureMemReq.memoryRequirements.size,
            vkfw_core::gfx::DeviceMemory::FindMemoryType(m_device, accStructureMemReq.memoryRequirements.memoryTypeBits,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal)};
        result.memory = m_device->GetDevice().allocateMemoryUnique(memoryAllocateInfo);

        vk::BindAccelerationStructureMemoryInfoKHR accStructureMemoryInfo{result.as.get(), result.memory.get()};
        m_device->GetDevice().bindAccelerationStructureMemoryKHR(accStructureMemoryInfo);

        vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{result.as.get()};
        result.handle = m_device->GetDevice().getAccelerationStructureAddressKHR(asDeviceAddressInfo);

        return result;
    }

    std::unique_ptr<vkfw_core::gfx::DeviceBuffer> AccelerationStructureGeometry::CreateASScratchBuffer(AccelerationStructure& as)
    {
        vk::AccelerationStructureMemoryRequirementsInfoKHR asMemoryReq{
            vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch,
            vk::AccelerationStructureBuildTypeKHR::eDevice, as.as.get()};
        auto memReq = m_device->GetDevice().getAccelerationStructureMemoryRequirementsKHR(asMemoryReq);

        std::unique_ptr<vkfw_core::gfx::DeviceBuffer> scratchBuffer = std::make_unique<vkfw_core::gfx::DeviceBuffer>(
            m_device, vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        scratchBuffer->InitializeBuffer(memReq.memoryRequirements.size);

        return scratchBuffer;
    }

    void AccelerationStructureGeometry::AllocateDescriptorSet(vk::DescriptorPool descPool)
    {
        vk::DescriptorSetAllocateInfo descSetAllocInfo{descPool, 1, &m_descLayout};
        m_descSet = m_device->GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
    }

}
