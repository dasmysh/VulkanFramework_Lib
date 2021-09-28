/**
 * @file   PipelineLayout.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.27
 *
 * @brief  Wrapper class for a vulkan descriptor set.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "main.h"

namespace vkfw_core::gfx {

    class DescriptorSet : public VulkanObjectWrapper<vk::DescriptorSet>
    {
    public:
        DescriptorSet(vk::Device device, std::string_view name, vk::DescriptorSet descriptorSet)
            : VulkanObjectWrapper{device, name, std::move(descriptorSet)}
        {
        }

        static std::vector<DescriptorSet> Initialize(vk::Device device, std::string_view name, const std::vector<vk::DescriptorSet>& descriptorSets)
        {
            std::vector<DescriptorSet> result;
            for (std::size_t i = 0; i < descriptorSets.size(); ++i) {
                result.emplace_back(device, fmt::format("{}-{}", name, i), descriptorSets[i]);
            }
            return result;
        }
    };
}
