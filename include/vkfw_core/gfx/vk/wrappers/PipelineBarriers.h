/**
 * @file   PipelineBarriers.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.13
 *
 * @brief  Helper classes for creating pipeline barriers.
 */

#pragma once

#include "main.h"
#include <variant>


namespace vkfw_core::gfx {

    class MemoryBoundResource;
    class Buffer;
    class Texture;
    class CommandBuffer;
    class LogicalDevice;

    struct ImageBarrierInfo
    {
        ImageBarrierInfo(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout);

        std::pair<vk::ImageMemoryBarrier, vk::PipelineStageFlags>
        CreateBarrier(const LogicalDevice* device, vk::AccessFlags dstAccess, vk::PipelineStageFlags dstPipelineStages,
                      unsigned int dstQueueFamily) const;

        Texture* m_texture;
        vk::ImageLayout m_dstLayout;
        vk::Image m_image;
        vk::ImageSubresourceRange m_subresourceRange;
    };

    struct BufferBarrierInfo
    {
        Buffer* m_buffer;
        vk::Buffer m_vkBuffer;
        vk::DeviceSize m_offset;
        vk::DeviceSize m_size;
    };

    class SingleResourcePipelineBarrier
    {
    public:
        SingleResourcePipelineBarrier(const LogicalDevice* device, Texture* texture, vk::Image image,
                                      vk::ImageLayout dstImageLayout, vk::AccessFlags dstAccess,
                                      vk::PipelineStageFlags dstPipelineStages);
        SingleResourcePipelineBarrier(const LogicalDevice* device);

        void Record(const CommandBuffer& cmdBuffer);

    private:
        using ResourceType = std::variant<ImageBarrierInfo, BufferBarrierInfo>;

        SingleResourcePipelineBarrier(const LogicalDevice* device, ResourceType resource, vk::AccessFlags dstAccess,
                                      vk::PipelineStageFlags dstPipelineStages);

        const LogicalDevice* m_device;
        // needs info:
        // memory: skip
        // buffer:
        // - buffer
        // - offset
        // - size
        ResourceType m_resource;
        vk::AccessFlags m_dstAccess;
        vk::PipelineStageFlags m_dstPipelineStages;
    };

    class PipelineBarrier
    {
    public:
        PipelineBarrier(const LogicalDevice* device, vk::PipelineStageFlags dstPipelineStages);

        void AddSingleBarrier(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout,
                              vk::AccessFlags dstAccess, vk::PipelineStageFlags dstPipelineStages);
        void AddSingleBarrier();

        void Record(const CommandBuffer& cmdBuffer);

    private:
        const LogicalDevice* m_device;

        using ResourceType = std::variant<ImageBarrierInfo, BufferBarrierInfo>;
        struct ResourcesEntry
        {
            ResourceType m_resource;
            vk::AccessFlags m_dstAccess;
        };
        std::vector<ResourcesEntry> m_resources;
        vk::PipelineStageFlags m_dstPipelineStages;
    };
}
