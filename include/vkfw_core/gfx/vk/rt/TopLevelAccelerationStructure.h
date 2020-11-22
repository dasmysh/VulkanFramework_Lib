/**
 * @file   TopLevelAccelerationStructure.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.22
 *
 * @brief  Declaration of the top level acceleration structure.
 */

#pragma once

#include "gfx/vk/buffers/HostBuffer.h"
#include "gfx/vk/rt/AccelerationStructure.h"

namespace vkfw_core::gfx::rt {

    class TopLevelAccelerationStructure : public AccelerationStructure
    {
    public:
        TopLevelAccelerationStructure(vkfw_core::gfx::LogicalDevice* device,
                                         vk::BuildAccelerationStructureFlagsKHR flags);
        TopLevelAccelerationStructure(TopLevelAccelerationStructure&& rhs) noexcept;
        TopLevelAccelerationStructure& operator=(TopLevelAccelerationStructure&& rhs) noexcept;
        ~TopLevelAccelerationStructure() override;

        void AddBottomLevelAccelerationStructureInstance(const vk::AccelerationStructureInstanceKHR& blasInstance);

        void BuildAccelerationStructure() override;

    private:
        /** Contains all the bottom level acceleration structure instances added. */
        std::vector<vk::AccelerationStructureInstanceKHR> m_blasInstances;
    };
}
