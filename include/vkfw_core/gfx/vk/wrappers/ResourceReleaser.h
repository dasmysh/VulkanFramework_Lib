/**
 * @file   ResourceReleaser.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.29
 *
 * @brief  Helper class to store resources for delayed release.
 */

#pragma once

#include "ReleaseableResource.h"
#include <memory>

namespace vkfw_core::gfx {

    class LogicalDevice;
    class Fence;

    class ResourceReleaser
    {
    public:
        ResourceReleaser(const LogicalDevice* device) : m_device{device} {}
        ~ResourceReleaser();

        std::shared_ptr<Fence> AddFence(std::string_view name);
        void AddResource(const std::shared_ptr<Fence>& fence, std::shared_ptr<const ReleaseableResource> resource);
        // void AddResources(const std::shared_ptr<const Fence>& fence, std::span<std::shared_ptr<const ReleaseableResource>> resources);
        void TryRelease();

    private:
        const LogicalDevice* m_device;
        std::unordered_map<std::shared_ptr<Fence>, std::vector<std::shared_ptr<const ReleaseableResource>>>
            m_releasableResources;
        std::vector<std::shared_ptr<Fence>> m_availableFences;
    };
}
