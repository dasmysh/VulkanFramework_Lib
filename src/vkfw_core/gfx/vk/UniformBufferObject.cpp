/**
 * @file   UniformBufferObject.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2018.10.15
 *
 * @brief
 */

#include "gfx/vk/UniformBufferObject.h"

namespace vkfw_core::gfx {

    UniformBufferObject::UniformBufferObject(const LogicalDevice* device, std::size_t singleSize, std::size_t numInstances) :
        m_device{ device },
        m_singleSize{ device->CalculateUniformBufferAlignment(singleSize) },
        m_numInstances{ numInstances }
    {

    }

    UniformBufferObject::UniformBufferObject(UniformBufferObject&& rhs) noexcept :
        m_device{ rhs.m_device },
        m_memoryGroup{ rhs.m_memoryGroup },
        m_bufferIdx{ rhs.m_bufferIdx },
        m_bufferOffset{ rhs.m_bufferOffset },
        m_singleSize{ rhs.m_singleSize },
        m_numInstances{ rhs.m_numInstances },
        m_descBinding{ rhs.m_descBinding },
        m_descType{ rhs.m_descType },
        m_internalDescLayout{ std::move(rhs.m_internalDescLayout) },
        m_descLayout{ rhs.m_descLayout },
        m_descSet{ rhs.m_descSet },
        m_descInfo{ rhs.m_descInfo }
    {

    }

    UniformBufferObject& UniformBufferObject::operator=(UniformBufferObject&& rhs) noexcept
    {
        this->~UniformBufferObject();
        m_device = rhs.m_device;
        m_memoryGroup = rhs.m_memoryGroup;
        m_bufferIdx = rhs.m_bufferIdx;
        m_bufferOffset = rhs.m_bufferOffset;
        m_singleSize = rhs.m_singleSize;
        m_numInstances = rhs.m_numInstances;
        m_descBinding = rhs.m_descBinding;
        m_descType = rhs.m_descType;
        m_internalDescLayout = std::move(rhs.m_internalDescLayout);
        m_descLayout = rhs.m_descLayout;
        m_descSet = rhs.m_descSet;
        m_descInfo = rhs.m_descInfo;
        return *this;
    }

    UniformBufferObject::~UniformBufferObject() = default;

    void UniformBufferObject::AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex, std::size_t bufferOffset, std::size_t size,
                                             const void* data)
    {
        m_memoryGroup = memoryGroup;
        m_bufferIdx = bufferIndex;
        m_bufferOffset = bufferOffset;

        for (auto i = 0UL; i < m_numInstances; ++i) {
            m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, m_bufferOffset + (i * m_singleSize), size, data);
        }

        m_descInfo.buffer = m_memoryGroup->GetBuffer(m_bufferIdx)->GetBuffer();
        m_descInfo.offset = bufferOffset;
        m_descInfo.range = m_singleSize;
    }

    void UniformBufferObject::AddUBOToBufferPrefill(MemoryGroup* memoryGroup, unsigned int bufferIndex,
                                                    std::size_t bufferOffset, std::size_t size,
                                                    const void* data)
    {
        m_memoryGroup = memoryGroup;
        m_bufferIdx = bufferIndex;
        m_bufferOffset = bufferOffset;

        m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, m_bufferOffset, size, data);

        m_descInfo.buffer = m_memoryGroup->GetBuffer(m_bufferIdx)->GetBuffer();
        m_descInfo.offset = bufferOffset;
        m_descInfo.range = m_singleSize;
    }

    void UniformBufferObject::FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx, std::size_t size) const
    {
        auto offset = m_bufferOffset + (instanceIdx * m_singleSize);
        m_memoryGroup->FillUploadBufferCmdBuffer(m_bufferIdx, cmdBuffer, offset, size);
    }

    void UniformBufferObject::FillDescriptorLayoutBinding(vk::DescriptorSetLayoutBinding& uboLayoutBinding,
                                                          const vk::ShaderStageFlags& shaderFlags, bool isDynamicBuffer,
                                                          std::uint32_t binding) const
    {
        auto descType = isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.setBinding(binding);
        uboLayoutBinding.setDescriptorType(descType);
        uboLayoutBinding.setDescriptorCount(1);
        uboLayoutBinding.setStageFlags(shaderFlags);
    }

    void UniformBufferObject::CreateLayout(vk::DescriptorPool descPool, const vk::ShaderStageFlags& shaderFlags, bool isDynamicBuffer, std::uint32_t binding)
    {
        m_descType = isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        m_descBinding = binding;
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        FillDescriptorLayoutBinding(uboLayoutBinding, shaderFlags, isDynamicBuffer, binding);

        vk::DescriptorSetLayoutCreateInfo uboLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(), 1, &uboLayoutBinding };
        m_internalDescLayout = m_device->GetDevice().createDescriptorSetLayoutUnique(uboLayoutCreateInfo);
        m_descLayout = *m_internalDescLayout;
        AllocateDescriptorSet(descPool);
    }

    void UniformBufferObject::UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout, bool isDynamicBuffer, std::uint32_t binding)
    {
        m_descType = isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        m_descBinding = binding;
        m_descLayout = usedLayout;
        AllocateDescriptorSet(descPool);
    }

    void UniformBufferObject::UseDescriptorSet(vk::DescriptorSet descSet, vk::DescriptorSetLayout usedLayout,
                                               bool isDynamicBuffer /*= false*/, std::uint32_t binding /*= 0*/)
    {
        m_descType = isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        m_descBinding = binding;
        m_descLayout = usedLayout;
        m_descSet = descSet;
    }

    void UniformBufferObject::AllocateDescriptorSet(vk::DescriptorPool descPool)
    {
        vk::DescriptorSetAllocateInfo descSetAllocInfo{ descPool, 1, &m_descLayout };
        m_descSet = m_device->GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
    }

    void UniformBufferObject::FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const
    {
        descWrite.dstSet = m_descSet;
        descWrite.dstBinding = m_descBinding;
        descWrite.dstArrayElement = 0;
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = m_descType;
        descWrite.pBufferInfo = &m_descInfo;
    }

    void UniformBufferObject::UpdateInstanceData(std::size_t instanceIdx, std::size_t size, const void* data) const
    {
        auto offset = m_bufferOffset + (instanceIdx * m_singleSize);
        m_memoryGroup->GetHostMemory()->CopyToHostMemory(m_memoryGroup->GetHostBufferOffset(m_bufferIdx) + offset, size,
                                                         data);
    }

    void UniformBufferObject::Bind(vk::CommandBuffer cmdBuffer, vk::PipelineBindPoint bindingPoint, vk::PipelineLayout pipelineLayout,
        std::uint32_t setIndex, std::size_t instanceIdx) const
    {
        cmdBuffer.bindDescriptorSets(bindingPoint, pipelineLayout, setIndex, m_descSet, static_cast<std::uint32_t>(instanceIdx * m_singleSize));
    }

}
