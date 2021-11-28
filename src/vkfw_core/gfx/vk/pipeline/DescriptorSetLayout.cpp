/**
 * @file   DescriptorSetLayout.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.12.01
 *
 * @brief  Implementation of the descriptor set layout helper class.
 */

#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include "gfx/vk/LogicalDevice.h"

namespace vkfw_core::gfx {

    DescriptorSetLayout::DescriptorSetLayout(std::string_view name)
        : VulkanObjectWrapper{nullptr, name, vk::UniqueDescriptorSetLayout{}}
    {
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& rhs) noexcept = default;

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& rhs) noexcept = default;

    DescriptorSetLayout::~DescriptorSetLayout() = default;

    void DescriptorSetLayout::AddBinding(std::uint32_t binding, vk::DescriptorType type, std::uint32_t count,
                                         vk::ShaderStageFlags stageFlags, const vk::Sampler* sampler /*= nullptr*/)
    {
        m_bindings.emplace_back(binding, type, count, stageFlags, sampler);
    }

    vk::DescriptorSetLayout DescriptorSetLayout::CreateDescriptorLayout(const LogicalDevice* device)
    {
        vk::DescriptorSetLayoutCreateInfo layoutInfo{vk::DescriptorSetLayoutCreateFlags{},
                                                     static_cast<std::uint32_t>(m_bindings.size()), m_bindings.data()};
        SetHandle(device->GetHandle(), device->GetHandle().createDescriptorSetLayoutUnique(layoutInfo));
        return GetHandle();
    }

    DescriptorPool DescriptorSetLayout::CreateDescriptorPool(const LogicalDevice* device, std::string_view name)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes;
        std::size_t setCount = 1;
        AddDescriptorPoolSizes(poolSizes, setCount);
        return DescriptorSetLayout::CreateDescriptorPool(device, name, poolSizes, setCount);
    }

    void DescriptorSetLayout::AddDescriptorPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes,
                                                     std::size_t setCount) const
    {
        for (const auto& binding : m_bindings) {
            bool found = false;
            for (auto& poolSize : poolSizes) {
                if (poolSize.type == binding.descriptorType) {
                    poolSize.descriptorCount += static_cast<std::uint32_t>(binding.descriptorCount * setCount);
                    found = true;
                    break;
                }
            }
            if (!found) {
                poolSizes.emplace_back(binding.descriptorType,
                                       static_cast<std::uint32_t>(binding.descriptorCount * setCount));
            }
        }
    }

    DescriptorPool DescriptorSetLayout::CreateDescriptorPool(
        const LogicalDevice* device, std::string_view name, const std::vector<vk::DescriptorPoolSize>& poolSizes, std::size_t setCount)
    {
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlags{}, static_cast<std::uint32_t>(setCount),
            static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data()};
        return DescriptorPool{device->GetHandle(), name,
                              device->GetHandle().createDescriptorPoolUnique(descriptorPoolCreateInfo)};
    }

    bool DescriptorSetLayout::IsBufferBindingType(vk::DescriptorType type)
    {
        return type == vk::DescriptorType::eStorageBuffer || type == vk::DescriptorType::eStorageBufferDynamic
               || type == vk::DescriptorType::eUniformBuffer || type == vk::DescriptorType::eUniformBufferDynamic;
    }

    bool DescriptorSetLayout::IsDynamicBindingType(vk::DescriptorType type)
    {
        return type == vk::DescriptorType::eStorageBufferDynamic || type == vk::DescriptorType::eUniformBufferDynamic;
    }

    bool DescriptorSetLayout::IsTexelBufferBindingType(vk::DescriptorType type)
    {
        return type == vk::DescriptorType::eStorageTexelBuffer || type == vk::DescriptorType::eUniformTexelBuffer;
    }

    bool DescriptorSetLayout::IsImageBindingType(vk::DescriptorType type)
    {
        return type == vk::DescriptorType::eCombinedImageSampler || type == vk::DescriptorType::eSampledImage
               || type == vk::DescriptorType::eStorageImage || type == vk::DescriptorType::eInputAttachment;
    }

}
