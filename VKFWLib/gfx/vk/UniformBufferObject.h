/**
 * @file   UniformBufferObject.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2018.10.15
 *
 * @brief  Declaration of a class for uniform buffer objects.
 */

#pragma once

#include "gfx/vk/memory/MemoryGroup.h"

namespace vku::gfx {

    class UniformBufferObject
    {
    public:
        UniformBufferObject(const LogicalDevice* device, std::size_t singleSize, std::size_t numInstances = 1);
        UniformBufferObject(const UniformBufferObject&) = delete;
        UniformBufferObject& operator=(const UniformBufferObject&) = delete;
        UniformBufferObject(UniformBufferObject&&) noexcept;
        UniformBufferObject& operator=(UniformBufferObject&&) noexcept;
        ~UniformBufferObject();


        void AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex,
            std::size_t bufferOffset, std::size_t size, const void* data);
        void CreateLayout(vk::DescriptorPool descPool, vk::ShaderStageFlags shaderFlags, bool isDynamicBuffer = false, std::uint32_t binding = 0);
        void UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout, bool isDynamicBuffer = false, std::uint32_t binding = 0);
        void FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx, std::size_t size) const;
        void FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const;
        void UpdateInstanceData(std::size_t instanceIdx, std::size_t size, const void* data) const;
        void Bind(vk::CommandBuffer cmdBuffer, vk::PipelineBindPoint bindingPoint, vk::PipelineLayout pipelineLayout,
            std::uint32_t setIndex, std::size_t instanceIdx) const;

        std::size_t GetCompleteSize() const { return singleSize_ * numInstances_; }
        vk::DescriptorSetLayout GetDescriptorLayout() const { return descLayout_; }


        //////////////////////////////////////////////////////////////////////////
        // Convenience interface.
        //////////////////////////////////////////////////////////////////////////

        template<class ContentType>
        static UniformBufferObject Create(const LogicalDevice* device, std::size_t numInstances = 1);
        template<class ContentType>
        void AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex,
            std::size_t bufferOffset, const ContentType& data);
        template<class ContentType>
        void FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx) const;
        template<class ContentType>
        void UpdateInstanceData(std::size_t instanceIdx, const ContentType& data) const;

    private:
        void AllocateDescriptorSet(vk::DescriptorPool descPool);

        /** Holds the device. */
        const LogicalDevice* device_;
        /** Holds the memory group is in. */
        MemoryGroup* memoryGroup_ = nullptr;
        /** The index into the memory group. */
        unsigned int bufferIdx_ = MemoryGroup::INVALID_INDEX;
        /** The offset into the buffer. */
        std::size_t bufferOffset_ = 0;
        /** The size of a single instance of data. */
        std::size_t singleSize_;
        /** The number of instances. */
        std::size_t numInstances_;
        /** Contains the descriptor binding. */
        std::uint32_t descBinding_ = 0;
        /** Contains weather the descriptor type. */
        vk::DescriptorType descType_ = vk::DescriptorType::eUniformBuffer;
        /** The internal descriptor layout if created here. */
        vk::UniqueDescriptorSetLayout internalDescLayout_;
        /** The descriptor layout used. */
        vk::DescriptorSetLayout descLayout_ = vk::DescriptorSetLayout();
        /** The descriptor set of this buffer. */
        vk::DescriptorSet descSet_ = vk::DescriptorSet();
        /** The UBO descriptor info. */
        vk::DescriptorBufferInfo descInfo_;

    };

    template<class ContentType>
    UniformBufferObject vku::gfx::UniformBufferObject::Create(const LogicalDevice* device, std::size_t numInstances /*= 1*/)
    {
        return UniformBufferObject{ device, sizeof(ContentType), numInstances };
    }

    template<class ContentType>
    void vku::gfx::UniformBufferObject::AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex,
        std::size_t bufferOffset, const ContentType& data)
    {
        AddUBOToBuffer(memoryGroup, bufferIndex, bufferOffset, sizeof(ContentType), &data);
    }

    template<class ContentType>
    void vku::gfx::UniformBufferObject::FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx) const
    {
        FillUploadCmdBuffer(cmdBuffer, instanceIdx, sizeof(ContentType));
    }

    template<class ContentType>
    void vku::gfx::UniformBufferObject::UpdateInstanceData(std::size_t instanceIdx, const ContentType& data) const
    {
        UpdateInstanceData(instanceIdx, sizeof(ContentType), &data);
    }

}
