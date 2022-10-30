/**
 * @file   VulkanSyncResources.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.31
 *
 * @brief  Implementation the vulkan synchronization resources.
 */

#include "gfx/vk/wrappers/VulkanSyncResources.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx {

    void Fence::Wait(const LogicalDevice* device, std::uint64_t timeout) const
    {
        if (auto r = device->GetHandle().waitForFences(GetHandle(), VK_TRUE, timeout); r != vk::Result::eSuccess) {
            spdlog::error("Error while waiting for fence: {}.", vk::to_string(r));
            throw std::runtime_error("Error while waiting for fence.");
        }
    }

    bool Fence::IsSignaled(const LogicalDevice* device) const
    {
        if (vk::Result::eSuccess == device->GetHandle().getFenceStatus(GetHandle())) {
            return true;
        }
        return false;
    }

    void Fence::Reset(const LogicalDevice* device, std::string_view name)
    {
        device->GetHandle().resetFences(GetHandle());
        if (!name.empty()) {
            SetObjectName(device->GetHandle(), GetHandle(), name);
        }
    }

}
