/**
 * @file   ResourceReleaser.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.30
 *
 * @brief  Implementation of the resource releaser.
 */

#include "gfx/vk/wrappers/ResourceReleaser.h"
#include "gfx/vk/wrappers/VulkanSyncResources.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx {

    ResourceReleaser::~ResourceReleaser()
    {
        for (auto& fencedResources : m_releasableResources) {
            fencedResources.first->Wait(m_device, defaultFenceTimeout);
        }
    }

    std::shared_ptr<Fence> ResourceReleaser::AddFence(std::string_view name)
    {
        std::shared_ptr<Fence> fence;
        if (!m_availableFences.empty()) {
            fence = m_availableFences.back();
            fence->Reset(m_device, name);
            m_availableFences.pop_back();
        } else {
            fence = std::make_shared<Fence>(m_device->GetHandle(), name,
                                                 m_device->GetHandle().createFenceUnique(vk::FenceCreateInfo{}));
        }
        m_releasableResources[fence] = std::vector<std::shared_ptr<const ReleaseableResource>>{};
        return fence;
    }

    void ResourceReleaser::AddResource(const std::shared_ptr<Fence>& fence,
                                       std::shared_ptr<const ReleaseableResource> resource)
    {
        m_releasableResources[fence].emplace_back(resource);
    }

    void ResourceReleaser::TryRelease()
    {
        for (auto it = m_releasableResources.begin(); it != m_releasableResources.end();) {
            auto& fencedResources = *it;
            if (fencedResources.first->IsSignaled(m_device)) {
                if (fencedResources.first.use_count() == 1) { m_availableFences.emplace_back(fencedResources.first); }
                it = m_releasableResources.erase(it);
            } else {
                ++it;
            }
        }
    }

}
