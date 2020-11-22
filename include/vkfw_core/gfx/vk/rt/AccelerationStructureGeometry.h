/**
 * @file   AccelerationStructureGeometry.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.20
 *
 * @brief  Declaration of the acceleration structure geometry class.
 */

#pragma once

#include "main.h"
#include "gfx/vk/rt/AccelerationStructure.h"

namespace vkfw_core::gfx {
    class LogicalDevice;
    class DeviceBuffer;
}

namespace vkfw_core::gfx::rt {

    class AccelerationStructureGeometry
    {
    public:
        AccelerationStructureGeometry(vkfw_core::gfx::LogicalDevice* device);
        ~AccelerationStructureGeometry();

        void AddTriangleGeometry(std::size_t primitiveCount, std::size_t vertexCount, std::size_t vertexSize,
                                 vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                 vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress,
                                 vk::DeviceOrHostAddressConstKHR transformDeviceAddress = nullptr);
        void InitializeAccelerationStructure();

        void FillDescriptorLayoutBinding(vk::DescriptorSetLayoutBinding& asLayoutBinding,
                                         const vk::ShaderStageFlags& shaderFlags, std::uint32_t binding = 0) const;
        void CreateLayout(vk::DescriptorPool descPool, const vk::ShaderStageFlags& shaderFlags,
                          std::uint32_t binding = 0);
        void UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout, std::uint32_t binding = 0);
        void UseDescriptorSet(vk::DescriptorSet descSet, vk::DescriptorSetLayout usedLayout, std::uint32_t binding = 0);

        void FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const;

    private:

        void CreateBottomLevelAccelerationStructure();
        void CreateTopLevelAccelerationStructure();

        void AllocateDescriptorSet(vk::DescriptorPool descPool);

        /** The device to create the acceleration structures in. */
        vkfw_core::gfx::LogicalDevice* m_device;

        /** The bottom level acceleration structure for the scene. */
        AccelerationStructure m_BLAS;
        /** The top level acceleration structure for the scene. */
        AccelerationStructure m_TLAS;

        /** Contains the descriptor binding. */
        std::uint32_t m_descBinding = 0;
        /** The internal descriptor layout if created here. */
        vk::UniqueDescriptorSetLayout m_internalDescLayout;
        /** The descriptor layout used. */
        vk::DescriptorSetLayout m_descLayout = vk::DescriptorSetLayout();
        /** The descriptor set of this buffer. */
        vk::DescriptorSet m_descSet = vk::DescriptorSet();

        /** The descriptor set acceleration structure write info (needed to ensure a valid pointer). */
        vk::WriteDescriptorSetAccelerationStructureKHR m_descriptorSetAccStructure;
    };
}
