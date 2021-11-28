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
    class PipelineBarrier;

    struct BufferRange
    {
        Buffer* m_buffer = nullptr;
        std::size_t m_offset = 0;
        std::size_t m_range = 0;
    };

    struct ImageBarrierInfo
    {
        ImageBarrierInfo(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout);

        vk::ImageMemoryBarrier2KHR CreateBarrier(const LogicalDevice* device,
                                                 std::vector<std::unique_ptr<PipelineBarrier>>& releaseBarriers,
                                                 vk::AccessFlags2KHR dstAccess,
                                                 vk::PipelineStageFlags2KHR dstPipelineStages,
                                                 unsigned int dstQueueFamily) const;

        Texture* m_texture;
        vk::ImageLayout m_dstLayout;
        vk::Image m_image;
        vk::ImageSubresourceRange m_subresourceRange;
    };

    struct BufferBarrierInfo
    {
        BufferBarrierInfo(Buffer* buffer, vk::Buffer vkBuffer, bool isDynamic);
        BufferBarrierInfo(BufferRange bufferRange, vk::Buffer buffer, bool isDynamic);

        void AddBarriers(const LogicalDevice* device, std::vector<vk::BufferMemoryBarrier2KHR>& bufferBarriers,
                         std::vector<std::unique_ptr<PipelineBarrier>>& releaseBarriers, vk::AccessFlags2KHR dstAccess,
                         vk::PipelineStageFlags2KHR dstPipelineStages, unsigned int dstQueueFamily,
                         std::uint32_t dynamicOffset) const;

        BufferRange m_bufferRange;
        vk::Buffer m_buffer;
        bool m_isDynamic;
    };

    class PipelineBarrier
    {
    public:
        PipelineBarrier(const LogicalDevice* device);

        void AddSingleBarrier(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout,
                              vk::AccessFlags2KHR dstAccess, vk::PipelineStageFlags2KHR dstPipelineStages);
        void AddSingleBarrier(Buffer* buffer, vk::Buffer vkBuffer, bool isDynamic, vk::AccessFlags2KHR dstAccess,
                              vk::PipelineStageFlags2KHR dstPipelineStages);
        void AddSingleBarrier(BufferRange bufferRange, vk::Buffer buffer, bool isDynamic, vk::AccessFlags2KHR dstAccess,
                              vk::PipelineStageFlags2KHR dstPipelineStages);

        void Record(CommandBuffer& cmdBuffer, const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets = {});
        void RecordRelease(CommandBuffer& cmdBuffer, unsigned int dstQueueFamily);
        void Record(std::vector<vk::ImageMemoryBarrier2KHR>& imageBarriers,
                    std::vector<vk::BufferMemoryBarrier2KHR>& bufferBarriers,
                    std::vector<std::unique_ptr<PipelineBarrier>>& releaseBarriers, unsigned int dstQueueFamily,
                    const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets = {});

    private:
        const LogicalDevice* m_device;

        using ResourceType = std::variant<ImageBarrierInfo, BufferBarrierInfo>;
        struct ResourcesEntry
        {
            ResourceType m_resource;
            vk::AccessFlags2KHR m_dstAccess;
            vk::PipelineStageFlags2KHR m_dstPipelineStages;
        };
        std::vector<ResourcesEntry> m_resources;
    };
}
