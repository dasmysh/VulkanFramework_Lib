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

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWrite(vk::DescriptorSet descriptorSet, std::uint32_t binding,
                                                          std::uint32_t arrayElement /*= 0*/)
    {
        vk::WriteDescriptorSet writeSet;
        for (std::size_t i = 0; i < m_bindings.size(); i++) {
            if (m_bindings[i].binding == binding) {
                writeSet.descriptorCount = 1;
                writeSet.descriptorType = m_bindings[i].descriptorType;
                writeSet.dstBinding = binding;
                writeSet.dstSet = descriptorSet;
                writeSet.dstArrayElement = arrayElement;
                return writeSet;
            }
        }
        assert(false && "binding not found");
        return writeSet;
    }

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWrite(vk::DescriptorSet descriptorSet, std::uint32_t binding,
                                                          const vk::DescriptorImageInfo* imageInfo,
                                                          std::uint32_t arrayElement /*= 0*/)
    {
        vk::WriteDescriptorSet writeSet = MakeWrite(descriptorSet, binding, arrayElement);
        assert(writeSet.descriptorType == vk::DescriptorType::eSampler
               || writeSet.descriptorType == vk::DescriptorType::eCombinedImageSampler
               || writeSet.descriptorType == vk::DescriptorType::eSampledImage
               || writeSet.descriptorType == vk::DescriptorType::eStorageImage
               || writeSet.descriptorType == vk::DescriptorType::eInputAttachment);

        writeSet.pImageInfo = imageInfo;
        return writeSet;
    }

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWrite(vk::DescriptorSet descriptorSet, std::uint32_t binding,
                                                          const vk::DescriptorBufferInfo* bufferInfo,
                                                          std::uint32_t arrayElement /*= 0*/)
    {
        vk::WriteDescriptorSet writeSet = MakeWrite(descriptorSet, binding, arrayElement);
        assert(writeSet.descriptorType == vk::DescriptorType::eStorageBuffer
               || writeSet.descriptorType == vk::DescriptorType::eStorageBufferDynamic
               || writeSet.descriptorType == vk::DescriptorType::eUniformBuffer
               || writeSet.descriptorType == vk::DescriptorType::eUniformBufferDynamic);

        writeSet.pBufferInfo = bufferInfo;
        return writeSet;
    }

    vk::WriteDescriptorSet
    DescriptorSetLayout::MakeWrite(vk::DescriptorSet descriptorSet, std::uint32_t binding,
                                   const vk::WriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo,
                                   std::uint32_t arrayElement /*= 0*/)
    {
        vk::WriteDescriptorSet writeSet = MakeWrite(descriptorSet, binding, arrayElement);
        assert(writeSet.descriptorType == vk::DescriptorType::eAccelerationStructureKHR);

        writeSet.pNext = accelerationStructureInfo;
        return writeSet;
    }

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWriteArray(vk::DescriptorSet descriptorSet,
                                                               std::uint32_t binding) const
    {
        vk::WriteDescriptorSet writeSet;
        for (std::size_t i = 0; i < m_bindings.size(); i++) {
            if (m_bindings[i].binding == binding) {
                writeSet.descriptorCount = m_bindings[i].descriptorCount;
                writeSet.descriptorType = m_bindings[i].descriptorType;
                writeSet.dstBinding = binding;
                writeSet.dstSet = descriptorSet;
                writeSet.dstArrayElement = 0;
                return writeSet;
            }
        }
        assert(false && "binding not found");
        return writeSet;
    }

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWriteArray(vk::DescriptorSet descriptorSet, std::uint32_t binding,
                                                               const vk::DescriptorImageInfo* imageInfo) const
    {
        vk::WriteDescriptorSet writeSet = MakeWriteArray(descriptorSet, binding);
        assert(writeSet.descriptorType == vk::DescriptorType::eSampler
               || writeSet.descriptorType == vk::DescriptorType::eCombinedImageSampler
               || writeSet.descriptorType == vk::DescriptorType::eSampledImage
               || writeSet.descriptorType == vk::DescriptorType::eStorageImage
               || writeSet.descriptorType == vk::DescriptorType::eInputAttachment);

        writeSet.pImageInfo = imageInfo;
        return writeSet;
    }

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWriteArray(vk::DescriptorSet descriptorSet, std::uint32_t binding,
                                                               const vk::DescriptorBufferInfo* bufferInfo) const
    {
        vk::WriteDescriptorSet writeSet = MakeWriteArray(descriptorSet, binding);
        assert(writeSet.descriptorType == vk::DescriptorType::eStorageBuffer
               || writeSet.descriptorType == vk::DescriptorType::eStorageBufferDynamic
               || writeSet.descriptorType == vk::DescriptorType::eUniformBuffer
               || writeSet.descriptorType == vk::DescriptorType::eUniformBufferDynamic);

        writeSet.pBufferInfo = bufferInfo;
        return writeSet;
    }

    vk::WriteDescriptorSet DescriptorSetLayout::MakeWriteArray(
        vk::DescriptorSet descriptorSet, std::uint32_t binding,
        const vk::WriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo) const
    {
        vk::WriteDescriptorSet writeSet = MakeWriteArray(descriptorSet, binding);
        assert(writeSet.descriptorType == vk::DescriptorType::eAccelerationStructureKHR);

        writeSet.pNext = accelerationStructureInfo;
        return writeSet;
    }

}
