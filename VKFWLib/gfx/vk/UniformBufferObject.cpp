/**
 * @file   UniformBufferObject.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2018.10.15
 *
 * @brief  
 */

#include "UniformBufferObject.h"

namespace vku::gfx {

    UniformBufferObject::UniformBufferObject(const LogicalDevice* device, std::size_t singleSize, std::size_t numInstances) :
        device_{ device },
        singleSize_{ device->CalculateUniformBufferAlignment(singleSize) },
        numInstances_{ numInstances }
    {

    }

    UniformBufferObject::UniformBufferObject(UniformBufferObject&& rhs) noexcept :
        device_{ std::move(rhs.device_) },
        memoryGroup_{ std::move(rhs.memoryGroup_) },
        bufferIdx_{ std::move(rhs.bufferIdx_) },
        bufferOffset_{ std::move(rhs.bufferOffset_) },
        singleSize_{ std::move(rhs.singleSize_) },
        numInstances_{ std::move(rhs.numInstances_) },
        descBinding_{ std::move(rhs.descBinding_) },
        descType_{ std::move(rhs.descType_) },
        internalDescLayout_{ std::move(rhs.internalDescLayout_) },
        descLayout_{ std::move(rhs.descLayout_) },
        descSet_{ std::move(rhs.descSet_) },
        descInfo_{ std::move(rhs.descInfo_) }
    {

    }

    UniformBufferObject& UniformBufferObject::operator=(UniformBufferObject&& rhs) noexcept
    {
        this->~UniformBufferObject();
        device_ = std::move(rhs.device_);
        memoryGroup_ = std::move(rhs.memoryGroup_);
        bufferIdx_ = std::move(rhs.bufferIdx_);
        bufferOffset_ = std::move(rhs.bufferOffset_);
        singleSize_ = std::move(rhs.singleSize_);
        numInstances_ = std::move(rhs.numInstances_);
        descBinding_ = std::move(rhs.descBinding_);
        descType_ = std::move(rhs.descType_);
        internalDescLayout_ = std::move(rhs.internalDescLayout_);
        descLayout_ = std::move(rhs.descLayout_);
        descSet_ = std::move(rhs.descSet_);
        descInfo_ = std::move(rhs.descInfo_);
        return *this;
    }

    UniformBufferObject::~UniformBufferObject() = default;

    void UniformBufferObject::AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex, std::size_t bufferOffset,
        std::size_t size, const void* data)
    {
        memoryGroup_ = memoryGroup;
        bufferIdx_ = bufferIndex;
        bufferOffset_ = bufferOffset;

        for (auto i = 0; i < numInstances_; ++i) {
            memoryGroup_->AddDataToBufferInGroup(bufferIdx_, bufferOffset_ + (i * singleSize_), size, data);
        }

        descInfo_.buffer = memoryGroup_->GetBuffer(bufferIdx_)->GetBuffer();
        descInfo_.offset = bufferOffset;
        descInfo_.range = singleSize_;
    }

    void UniformBufferObject::AddUBOToBufferPrefill(MemoryGroup* memoryGroup, unsigned int bufferIndex,
        std::size_t bufferOffset, std::size_t size, const void* data)
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

    void UniformBufferObject::CreateLayout(vk::DescriptorPool descPool, vk::ShaderStageFlags shaderFlags, bool isDynamicBuffer, std::uint32_t binding)
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
