/**
 * @file   UniformBufferObject.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2018.10.15
 *
 * @brief  Declaration of a class for uniform buffer objects.
 */

#pragma once

#include "gfx/vk/memory/MemoryGroup.h"

namespace vkfw_core::gfx {

    class UniformBufferObject
    {
    public:
        UniformBufferObject(const LogicalDevice* device, std::size_t singleSize, std::size_t numInstances = 1);
        UniformBufferObject(const UniformBufferObject&) = delete;
        UniformBufferObject& operator=(const UniformBufferObject&) = delete;
        UniformBufferObject(UniformBufferObject&&) noexcept;
        UniformBufferObject& operator=(UniformBufferObject&&) noexcept;
        ~UniformBufferObject();


        void AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex, std::size_t bufferOffset,
                            std::size_t size, const void* data);
        void AddUBOToBufferPrefill(MemoryGroup* memoryGroup, unsigned int bufferIndex, std::size_t bufferOffset,
                                   std::size_t size, const void* data);
        void CreateLayout(vk::DescriptorPool descPool, const vk::ShaderStageFlags& shaderFlags, bool isDynamicBuffer = false, std::uint32_t binding = 0);
        void UseLayout(vk::DescriptorPool descPool, vk::DescriptorSetLayout usedLayout, bool isDynamicBuffer = false, std::uint32_t binding = 0);
        void FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx, std::size_t size) const;
        void FillDescriptorSetWrite(vk::WriteDescriptorSet& descWrite) const;
        void UpdateInstanceData(std::size_t instanceIdx, std::size_t size, const void* data) const;
        void Bind(vk::CommandBuffer cmdBuffer, vk::PipelineBindPoint bindingPoint, vk::PipelineLayout pipelineLayout,
            std::uint32_t setIndex, std::size_t instanceIdx) const;

        [[nodiscard]] std::size_t GetCompleteSize() const { return m_singleSize * m_numInstances; }
        [[nodiscard]] vk::DescriptorSetLayout GetDescriptorLayout() const { return m_descLayout; }


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
        const LogicalDevice* m_device;
        /** Holds the memory group is in. */
        MemoryGroup* m_memoryGroup = nullptr;
        /** The index into the memory group. */
        unsigned int m_bufferIdx = MemoryGroup::INVALID_INDEX;
        /** The offset into the buffer. */
        std::size_t m_bufferOffset = 0;
        /** The size of a single instance of data. */
        std::size_t m_singleSize;
        /** The number of instances. */
        std::size_t m_numInstances;
        /** Contains the descriptor binding. */
        std::uint32_t m_descBinding = 0;
        /** Contains weather the descriptor type. */
        vk::DescriptorType m_descType = vk::DescriptorType::eUniformBuffer;
        /** The internal descriptor layout if created here. */
        vk::UniqueDescriptorSetLayout m_internalDescLayout;
        /** The descriptor layout used. */
        vk::DescriptorSetLayout m_descLayout = vk::DescriptorSetLayout();
        /** The descriptor set of this buffer. */
        vk::DescriptorSet m_descSet = vk::DescriptorSet();
        /** The UBO descriptor info. */
        vk::DescriptorBufferInfo m_descInfo;

    };

    template<class ContentType>
    UniformBufferObject vkfw_core::gfx::UniformBufferObject::Create(const LogicalDevice* device, std::size_t numInstances /*= 1*/)
    {
        return UniformBufferObject{ device, sizeof(ContentType), numInstances };
    }

    template<class ContentType>
    void vkfw_core::gfx::UniformBufferObject::AddUBOToBuffer(MemoryGroup* memoryGroup, unsigned int bufferIndex,
        std::size_t bufferOffset, const ContentType& data)
    {
        AddUBOToBuffer(memoryGroup, bufferIndex, bufferOffset, sizeof(ContentType), &data);
    }

    template<class ContentType>
    void vkfw_core::gfx::UniformBufferObject::FillUploadCmdBuffer(vk::CommandBuffer cmdBuffer, std::size_t instanceIdx) const
    {
        FillUploadCmdBuffer(cmdBuffer, instanceIdx, sizeof(ContentType));
    }

    template<class ContentType>
    void vkfw_core::gfx::UniformBufferObject::UpdateInstanceData(std::size_t instanceIdx, const ContentType& data) const
    {
        UpdateInstanceData(instanceIdx, sizeof(ContentType), &data);
    }

}
