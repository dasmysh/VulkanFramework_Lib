/**
 * @file   DescriptorSet.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.19
 *
 * @brief  Implementation of the DescriptorSet wrapper class.
 */

#include "gfx/vk/wrappers/DescriptorSet.h"
#include "gfx/vk/pipeline/DescriptorSetLayout.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/PipelineBarriers.h"
#include "gfx/vk/wrappers/PipelineLayout.h"
#include "gfx/vk/textures/Texture.h"
#include "gfx/vk/buffers/Buffer.h"
#include "gfx/vk/rt/AccelerationStructureGeometry.h"

namespace vkfw_core::gfx {

    DescriptorSet::DescriptorSet(const LogicalDevice* device, std::string_view name, vk::DescriptorSet descriptorSet)
        : VulkanObjectPrivateWrapper{device->GetHandle(), name, std::move(descriptorSet)}, m_barrier{device}
    {
    }

    void DescriptorSet::InitializeWrites(const LogicalDevice* device, const DescriptorSetLayout& layout)
    {
        m_barrier = PipelineBarrier{device};
        m_layoutBindings = layout.GetBindings();
        m_descriptorSetWrites.clear();
    }

    void DescriptorSet::WriteImageDescriptor(std::uint32_t binding, std::uint32_t arrayElement,
                                             std::span<Texture*> textures, const Sampler& sampler,
                                             vk::AccessFlags2KHR access, vk::ImageLayout layout)
    {
        auto [writeSet, pipelineStage] = WriteGeneralDescriptor(binding, arrayElement, textures.size());
        assert(DescriptorSetLayout::IsImageBindingType(writeSet.descriptorType));
        auto& imageWrite = AddImageWrite(textures.size());

        for (std::size_t i = 0; i < textures.size(); ++i) {
            imageWrite[i].sampler = sampler.GetHandle();
            imageWrite[i].imageView = textures[i]->GetImageView(access, pipelineStage, layout, m_barrier).GetHandle();
            imageWrite[i].imageLayout = layout;
        }

        writeSet.pImageInfo = imageWrite.data();
    }

    void DescriptorSet::WriteBufferDescriptor(std::uint32_t binding, std::uint32_t arrayElement,
                                              std::span<BufferRange> buffers,
                                              [[maybe_unused]] vk::AccessFlags2KHR access)
    {
        auto [writeSet, pipelineStage] = WriteGeneralDescriptor(binding, arrayElement, buffers.size());
        assert(DescriptorSetLayout::IsBufferBindingType(writeSet.descriptorType));
        auto& bufferWrite = AddBufferWrite(buffers.size());

        for (std::size_t i = 0; i < buffers.size(); ++i) {
            bufferWrite[i].buffer = buffers[i].m_buffer->GetBuffer(
                DescriptorSetLayout::IsDynamicBindingType(writeSet.descriptorType), access, pipelineStage, m_barrier);
            bufferWrite[i].offset = buffers[i].m_offset;
            bufferWrite[i].range = buffers[i].m_range;
        }

        writeSet.pBufferInfo = bufferWrite.data();
    }

    void DescriptorSet::WriteTexelBufferDescriptor(std::uint32_t binding, std::uint32_t arrayElement,
                                                   std::span<TexelBufferInfo> buffers,
                                                   [[maybe_unused]] vk::AccessFlags2KHR access)
    {
        auto [writeSet, pipelineStage] = WriteGeneralDescriptor(binding, arrayElement, buffers.size());
        assert(DescriptorSetLayout::IsTexelBufferBindingType(writeSet.descriptorType));
        auto& bufferWrite = std::get<std::vector<vk::BufferView>>(
            m_descriptorResourceWrites.emplace_back(std::vector<vk::BufferView>{}));

        bufferWrite.resize(buffers.size());
        for (std::size_t i = 0; i < buffers.size(); ++i) {
            buffers[i].m_bufferRange.m_buffer->AccessBarrier(false, access, pipelineStage, m_barrier);
            bufferWrite[i] = buffers[i].m_bufferView->GetHandle();
        }

        writeSet.pTexelBufferView = bufferWrite.data();
    }

    void DescriptorSet::WriteSamplerDescriptor(std::uint32_t binding, std::uint32_t arrayElement,
                                               std::span<const Sampler*> samplers)
    {
        auto [writeSet, pipelineStage] = WriteGeneralDescriptor(binding, arrayElement, samplers.size());
        assert(writeSet.descriptorType == vk::DescriptorType::eSampler);
        auto& imageWrite = AddImageWrite(samplers.size());

        for (std::size_t i = 0; i < samplers.size(); ++i) {
            imageWrite[i].sampler = samplers[i]->GetHandle();
            imageWrite[i].imageView = nullptr;
            imageWrite[i].imageLayout = vk::ImageLayout{};
        }

        writeSet.pImageInfo = imageWrite.data();
    }

    void DescriptorSet::WriteAccelerationStructureDescriptor(
        std::uint32_t binding, std::uint32_t arrayElement,
        std::span<const rt::AccelerationStructureGeometry*> accelerationStructures)
    {
        auto [writeSet, pipelineStage] = WriteGeneralDescriptor(binding, arrayElement, accelerationStructures.size());
        assert(writeSet.descriptorType == vk::DescriptorType::eAccelerationStructureKHR);
        auto& asWrite = std::get<AccelerationStructureWriteInfo>(
            m_descriptorResourceWrites.emplace_back(AccelerationStructureWriteInfo{}));

        asWrite.second.resize(accelerationStructures.size());
        for (std::size_t i = 0; i < accelerationStructures.size(); ++i) {
            asWrite.second[i] = accelerationStructures[i]->GetTopLevelAccelerationStructure(
                vk::AccessFlagBits2KHR::eAccelerationStructureReadKHR, pipelineStage, m_barrier);
        }
        asWrite.first = std::make_unique<vk::WriteDescriptorSetAccelerationStructureKHR>(asWrite.second);

        writeSet.pNext = asWrite.first.get();
    }

    void DescriptorSet::FinalizeWrite(const LogicalDevice* device)
    {
        device->GetHandle().updateDescriptorSets(m_descriptorSetWrites, nullptr);
    }

    std::pair<vk::WriteDescriptorSet&, vk::PipelineStageFlags2KHR>
    DescriptorSet::WriteGeneralDescriptor(std::uint32_t binding, std::uint32_t arrayElement, std::size_t arraySize)
    {
        assert(arrayElement == 0 || arraySize == 1);
        const auto& bindingLayout = GetBindingLayout(binding);
        assert(bindingLayout.descriptorCount == arraySize);
        auto& writeSet =
            m_descriptorSetWrites.emplace_back(GetHandle(), bindingLayout.binding, arrayElement,
                                                  static_cast<std::uint32_t>(arraySize), bindingLayout.descriptorType);

        return std::pair<vk::WriteDescriptorSet&, vk::PipelineStageFlags2KHR>{
            writeSet, GetCorrespondingPipelineStage(bindingLayout.stageFlags)};
    }

    std::vector<vk::DescriptorImageInfo>& DescriptorSet::AddImageWrite(std::size_t elements)
    {
        auto& imageWrite = std::get<std::vector<vk::DescriptorImageInfo>>(
            m_descriptorResourceWrites.emplace_back(std::vector<vk::DescriptorImageInfo>{}));
        imageWrite.resize(elements);
        return imageWrite;
    }

    std::vector<vk::DescriptorBufferInfo>& DescriptorSet::AddBufferWrite(std::size_t elements)
    {
        auto& bufferWrite = std::get<std::vector<vk::DescriptorBufferInfo>>(
            m_descriptorResourceWrites.emplace_back(std::vector<vk::DescriptorBufferInfo>{}));
        bufferWrite.resize(elements);
        return bufferWrite;
    }

    void DescriptorSet::BindBarrier(CommandBuffer& cmdBuffer, const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets)
    {
        m_barrier.Record(cmdBuffer, dynamicOffsets);
        m_skipNextBindBarriers += 1;
    }

    void DescriptorSet::Bind(CommandBuffer& cmdBuffer, vk::PipelineBindPoint bindingPoint,
                             const PipelineLayout& pipelineLayout, std::uint32_t firstSet,
                             const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets)
    {
        if (m_skipNextBindBarriers == 0) {
            m_barrier.Record(cmdBuffer);
        } else {
            m_skipNextBindBarriers -= 1;
        }

        cmdBuffer.GetHandle().bindDescriptorSets(bindingPoint, pipelineLayout.GetHandle(), firstSet, GetHandle(),
                                                 dynamicOffsets);
    }

    const vk::DescriptorSetLayoutBinding& DescriptorSet::GetBindingLayout(std::uint32_t binding)
    {
        for (std::size_t i = 0; i < m_layoutBindings.size(); i++) {
            if (m_layoutBindings[i].binding == binding) { return m_layoutBindings[i]; }
        }
        throw std::runtime_error("Binding does not exist.");
    }

    vk::PipelineStageFlags2KHR DescriptorSet::GetCorrespondingPipelineStage(vk::ShaderStageFlags shaderStage)
    {
        vk::PipelineStageFlags2KHR pipelineStages;
        if (shaderStage & vk::ShaderStageFlagBits::eVertex) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eVertexShader;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eTessellationControl) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eTessellationControlShader;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eTessellationEvaluation) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eTessellationEvaluationShader;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eGeometry) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eGeometryShader;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eFragment) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eFragmentShader;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eCompute) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eComputeShader;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eAllGraphics) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eAllGraphics;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eAll) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eAllCommands;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eRaygenKHR) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eAnyHitKHR) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eClosestHitKHR) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eMissKHR) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eIntersectionKHR) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eCallableKHR) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eRayTracingShaderKHR;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eTaskNV) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eTaskShaderNV;
        }
        if (shaderStage & vk::ShaderStageFlagBits::eMeshNV) {
            pipelineStages = pipelineStages | vk::PipelineStageFlagBits2KHR::eMeshShaderNV;
        }

        if (pipelineStages == vk::PipelineStageFlags2KHR{}) {
            throw std::runtime_error{"Shader stage not supported."};
        } else {
            return pipelineStages;
        }
    }
}
