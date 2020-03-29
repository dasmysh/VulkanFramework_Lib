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
        device_{ device },
        singleSize_{ device->CalculateUniformBufferAlignment(singleSize) },
        numInstances_{ numInstances }
    {

    }

    UniformBufferObject::UniformBufferObject(UniformBufferObject&& rhs) noexcept :
        device_{ rhs.device_ },
        memoryGroup_{ rhs.memoryGroup_ },
        bufferIdx_{ rhs.bufferIdx_ },
        bufferOffset_{ rhs.bufferOffset_ },
        singleSize_{ rhs.singleSize_ },
        numInstances_{ rhs.numInstances_ },
        descBinding_{ rhs.descBinding_ },
        descType_{ rhs.descType_ },
        internalDescLayout_{ std::move(rhs.internalDescLayout_) },
        descLayout_{ rhs.descLayout_ },
        descSet_{ rhs.descSet_ },
        descInfo_{ rhs.descInfo_ }
    {

    }

    UniformBufferObject& UniformBufferObject::operator=(UniformBufferObject&& rhs) noexcept
    {
        this->~UniformBufferObject();
        device_ = rhs.device_;
        memoryGroup_ = rhs.memoryGroup_;
        bufferIdx_ = rhs.bufferIdx_;
        bufferOffset_ = rhs.bufferOffset_;
        singleSize_ = rhs.singleSize_;
        numInstances_ = rhs.numInstances_;
        descBinding_ = rhs.descBinding_;
        descType_ = rhs.descType_;
        internalDescLayout_ = std::move(rhs.internalDescLayout_);
        descLayout_ = rhs.descLayout_;
        descSet_ = rhs.descSet_;
        descInfo_ = rhs.descInfo_;
        return *this;
    }

    UniformBufferObject::~UniformBufferObject() = default;

    void UniformBufferObject::AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex, std::size_t bufferOffset, std::size_t size,
                                             std::variant<void*, const void*> data)
    {
        memoryGroup_ = memoryGroup;
        bufferIdx_ = bufferIndex;
        bufferOffset_ = bufferOffset;

        for (auto i = 0UL; i < numInstances_; ++i) {
            memoryGroup_->AddDataToBufferInGroup(bufferIdx_, bufferOffset_ + (i * singleSize_), size, data);
        }

        descInfo_.buffer = memoryGroup_->GetBuffer(bufferIdx_)->GetBuffer();
        descInfo_.offset = bufferOffset;
        descInfo_.range = singleSize_;
    }

    void UniformBufferObject::AddUBOToBufferPrefill(MemoryGroup* memoryGroup, unsigned int bufferIndex,
                                                    std::size_t bufferOffset, std::size_t size,
                                                    std::variant<void*, const void*> data)
    {
        memoryGroup_ = memoryGroup;
        bufferIdx_ = bufferIndex;
        bufferOffset_ = bufferOffset;

        memoryGroup_->AddDataToBufferInGroup(bufferIdx_, bufferOffset_, size, data);

        descInfo_.buffer = memoryGroup_->GetBuffer(bufferIdx_)->GetBuffer();
        descInfo_.offset = bufferOffset;
        descInfo_.range = singleSize_;
    }

    void UniformBufferObject::FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx, std::size_t size) const
    {
        auto offset = bufferOffset_ + (instanceIdx * singleSize_);
        memoryGroup_->FillUploadBufferCmdBuffer(bufferIdx_, cmdBuffer, offset, size);
    }

    void UniformBufferObject::CreateLayout(vk::DescriptorPool descPool, const vk::ShaderStageFlags& shaderFlags, bool isDynamicBuffer, std::uint32_t binding)
    {
        descType_ = isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        descBinding_ = binding;
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ descBinding_, descType_, 1, shaderFlags };

        vk::DescriptorSetLayoutCreateInfo uboLayoutCreateInfo{ vk::DescriptorSetLayoutCreateFlags(), 1, &uboLayoutBinding };
        internalDescLayout_ = device_->GetDevice().createDescriptorSetLayoutUnique(uboLayoutCreateInfo);
        descLayout_ = *internalDescLayout_;
        AllocateDescriptorSet(descPool);
    }

    void UniformBufferObject::UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout, bool isDynamicBuffer, std::uint32_t binding)
    {
        descType_ = isDynamicBuffer ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
        descBinding_ = binding;
        descLayout_ = usedLayout;
        AllocateDescriptorSet(descPool);
    }

    void UniformBufferObject::AllocateDescriptorSet(vk::DescriptorPool descPool)
    {
        vk::DescriptorSetAllocateInfo descSetAllocInfo{ descPool, 1, &descLayout_ };
        descSet_ = device_->GetDevice().allocateDescriptorSets(descSetAllocInfo)[0];
    }

    void UniformBufferObject::FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const
    {
        descWrite.dstSet = descSet_;
        descWrite.dstBinding = descBinding_;
        descWrite.dstArrayElement = 0;
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = descType_;
        descWrite.pBufferInfo = &descInfo_;
    }

    void UniformBufferObject::UpdateInstanceData(std::size_t instanceIdx, std::size_t size, const void* data) const
    {
        auto offset = bufferOffset_ + (instanceIdx * singleSize_);
        memoryGroup_->GetHostMemory()->CopyToHostMemory(memoryGroup_->GetHostBufferOffset(bufferIdx_) + offset, size, data);
    }

    void UniformBufferObject::Bind(vk::CommandBuffer cmdBuffer, vk::PipelineBindPoint bindingPoint, vk::PipelineLayout pipelineLayout,
        std::uint32_t setIndex, std::size_t instanceIdx) const
    {
        cmdBuffer.bindDescriptorSets(bindingPoint, pipelineLayout, setIndex, descSet_, static_cast<std::uint32_t>(instanceIdx * singleSize_));
    }

}
