/**
 * @file   PipelineLayout.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.27
 *
 * @brief  Wrapper class for a vulkan descriptor set.
 */

#pragma once

#include "VulkanObjectWrapper.h"
#include "PipelineBarriers.h"
#include "main.h"
#include <variant>

namespace vkfw_core::gfx {

    class LogicalDevice;
    class CommandBuffer;
    class PipelineLayout;
    class DescriptorSetLayout;
    class Texture;
    class Sampler;
    class Buffer;
    class BufferView;
    struct BufferRange;

    namespace rt {
        class AccelerationStructureGeometry;
    }

    struct TexelBufferInfo
    {
        BufferRange m_bufferRange;
        BufferView* m_bufferView;
    };

    class DescriptorSet : public VulkanObjectPrivateWrapper<vk::DescriptorSet>
    {
    public:
        DescriptorSet(const LogicalDevice* device, std::string_view name, vk::DescriptorSet descriptorSet);

        static std::vector<DescriptorSet> Initialize(const LogicalDevice* device, std::string_view name,
                                                     const std::vector<vk::DescriptorSet>& descriptorSets)
        {
            std::vector<DescriptorSet> result;
            for (std::size_t i = 0; i < descriptorSets.size(); ++i) {
                result.emplace_back(device, fmt::format("{}-{}", name, i), descriptorSets[i]);
            }
            return result;
        }

        void InitializeWrites(const LogicalDevice* device, const DescriptorSetLayout& layout);
        void WriteImageDescriptor(std::uint32_t binding, std::uint32_t arrayElement, std::span<Texture*> textures,
                                  const Sampler& sampler, vk::AccessFlags2KHR access, vk::ImageLayout layout);
        void WriteBufferDescriptor(std::uint32_t binding, std::uint32_t arrayElement, std::span<BufferRange> buffers,
                                   vk::AccessFlags2KHR access);
        void WriteTexelBufferDescriptor(std::uint32_t binding, std::uint32_t arrayElement,
                                        std::span<TexelBufferInfo> buffers, vk::AccessFlags2KHR access);
        void WriteSamplerDescriptor(std::uint32_t binding, std::uint32_t arrayElement, std::span<const Sampler*> samplers);
        void WriteAccelerationStructureDescriptor(
            std::uint32_t binding, std::uint32_t arrayElement,
            std::span<const rt::AccelerationStructureGeometry*> accelerationStructures);
        void FinalizeWrite(const LogicalDevice* device);

        void BindBarrier(CommandBuffer& cmdBuffer);
        void Bind(CommandBuffer& cmdBuffer, vk::PipelineBindPoint bindingPoint,
                  const PipelineLayout& pipelineLayout, std::uint32_t firstSet,
                  const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets = {});

    private:
        [[nodiscard]] std::pair<vk::WriteDescriptorSet&, vk::PipelineStageFlags2KHR>
        WriteGeneralDescriptor(std::uint32_t binding, std::uint32_t arrayElement, std::size_t arraySize);
        [[nodiscard]] std::vector<vk::DescriptorImageInfo>& AddImageWrite(std::size_t elements);
        [[nodiscard]] std::vector<vk::DescriptorBufferInfo>& AddBufferWrite(std::size_t elements);
        [[nodiscard]] const vk::DescriptorSetLayoutBinding& GetBindingLayout(std::uint32_t binding);
        static vk::PipelineStageFlags2KHR GetCorrespondingPipelineStage(vk::ShaderStageFlags shaderStage);

        std::vector<vk::DescriptorSetLayoutBinding> m_layoutBindings;

        using AccelerationStructureWriteInfo = std::pair<std::unique_ptr<vk::WriteDescriptorSetAccelerationStructureKHR>, std::vector<vk::AccelerationStructureKHR>>;
        using DescriptorWriteResourceType = std::variant<std::vector<vk::DescriptorImageInfo>, std::vector<vk::DescriptorBufferInfo>, std::vector<vk::BufferView>, AccelerationStructureWriteInfo>;

        std::vector<DescriptorWriteResourceType> m_descriptorResourceWrites;

        std::vector<vk::WriteDescriptorSet> m_descriptorSetWrites;
        /** Pipeline barrier for using this descriptor set. */
        gfx::PipelineBarrier m_barrier;
        /** Flags the descriptor set to skip the next bind barrier since it was already set from the framebuffer. */
        unsigned int m_skipNextBindBarriers = 0;
    };
}
