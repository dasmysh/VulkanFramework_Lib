/**
 * @file   UniformBufferObject.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2018.10.15
 *
 * @brief
 */

#include "gfx/vk/UniformBufferObject.h"
#include "gfx/vk/pipeline/DescriptorSetLayout.h"

namespace vkfw_core::gfx {

    UniformBufferObject::UniformBufferObject(const LogicalDevice* device, std::size_t singleSize, std::size_t numInstances) :
        m_device{ device },
        m_singleSize{ device->CalculateUniformBufferAlignment(singleSize) },
        m_numInstances{ numInstances }
    {

    }

    UniformBufferObject::UniformBufferObject(UniformBufferObject&& rhs) noexcept = default;

    UniformBufferObject& UniformBufferObject::operator=(UniformBufferObject&& rhs) noexcept = default;

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
    }

    void UniformBufferObject::AddUBOToBufferPrefill(MemoryGroup* memoryGroup, unsigned int bufferIndex,
                                                    std::size_t bufferOffset, std::size_t size,
                                                    const void* data)
    {
        m_memoryGroup = memoryGroup;
        m_bufferIdx = bufferIndex;
        m_bufferOffset = bufferOffset;

        m_memoryGroup->AddDataToBufferInGroup(m_bufferIdx, m_bufferOffset, size, data);
    }

    void UniformBufferObject::FillUploadCmdBuffer(CommandBuffer& cmdBuffer, std::size_t instanceIdx,
                                                  std::size_t size) const
    {
        auto offset = m_bufferOffset + (instanceIdx * m_singleSize);
        m_memoryGroup->FillUploadBufferCmdBuffer(m_bufferIdx, cmdBuffer, offset, size);
    }

    void UniformBufferObject::AddDescriptorLayoutBinding(DescriptorSetLayout& layout, vk::ShaderStageFlags shaderFlags,
                                                         bool isDynamicBuffer, std::uint32_t binding)
    {
        auto descType =
            isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        layout.AddBinding(binding, descType, 1, shaderFlags);
    }

    void UniformBufferObject::FillBufferRange(BufferRange& bufferRange) const
    {
        bufferRange.m_buffer = m_memoryGroup->GetBuffer(m_bufferIdx);
        bufferRange.m_offset = m_bufferOffset;
        bufferRange.m_range = m_singleSize;
    }

    void UniformBufferObject::FillBufferRanges(std::vector<BufferRange>& bufferRanges) const
    {
        auto& bufferRange = bufferRanges.emplace_back();
        FillBufferRange(bufferRange);
    }

    void UniformBufferObject::UpdateInstanceData(std::size_t instanceIdx, std::size_t size, const void* data) const
    {
        auto offset = m_bufferOffset + (instanceIdx * m_singleSize);
        m_memoryGroup->GetHostMemory()->CopyToHostMemory(m_memoryGroup->GetHostBufferOffset(m_bufferIdx) + offset, size,
                                                         data);
    }

}
