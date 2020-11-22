/**
 * @file   BottomLevelAccelerationStructure.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.22
 *
 * @brief  Declaration of the bottom level acceleration structure class.
 */

#pragma once

#include "gfx/vk/rt/AccelerationStructure.h"

namespace vkfw_core::gfx::rt {

    class BottomLevelAccelerationStructure : public AccelerationStructure
    {
    public:
        BottomLevelAccelerationStructure(vkfw_core::gfx::LogicalDevice* device,
                                         vk::BuildAccelerationStructureFlagsKHR flags);
        BottomLevelAccelerationStructure(BottomLevelAccelerationStructure&& rhs) noexcept;
        BottomLevelAccelerationStructure& operator=(BottomLevelAccelerationStructure&& rhs) noexcept;
        ~BottomLevelAccelerationStructure() override;

        void AddTriangleGeometry(std::size_t primitiveCount, std::size_t vertexCount, std::size_t vertexSize,
                                 vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress,
                                 vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress,
                                 vk::DeviceOrHostAddressConstKHR transformDeviceAddress = nullptr);

    private:
    };
}
