/**
 * @file   PipelineBarriers.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.13
 *
 * @brief  Implementation the pipeline barrier helper classes.
 */

#include "gfx/vk/wrappers/PipelineBarriers.h"
#include "gfx/vk/wrappers/MemoryBoundResource.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/textures/Texture.h"
#include "gfx/vk/buffers/Buffer.h"

namespace vkfw_core::gfx {

    ImageBarrierInfo::ImageBarrierInfo(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout)
        : m_texture{texture}
        , m_dstLayout{dstImageLayout}
        , m_image{image}
        , m_subresourceRange{texture->GetValidAspects(), 0, texture->GetMipLevels(), 0, texture->GetPixelSize().w}
    {
    }

    std::pair<vk::ImageMemoryBarrier, vk::PipelineStageFlags>
    ImageBarrierInfo::CreateBarrier(const LogicalDevice* device, vk::AccessFlags dstAccess,
                                                           vk::PipelineStageFlags dstPipelineStages,
                                                           unsigned int dstQueueFamily) const
    {
        auto [srcAccess, srcPipelineStages, srcQueueFamily] = m_texture->GetPreviousAccess();
        auto srcDeviceQueueFamily = device->GetQueueInfo(srcQueueFamily).m_familyIndex;
        auto dstDeviceQueueFamily = device->GetQueueInfo(dstQueueFamily).m_familyIndex;

        vk::ImageMemoryBarrier imageBarrier{srcAccess,
                                            dstAccess,
                                            m_texture->GetImageLayout(),
                                            m_dstLayout,
                                            srcDeviceQueueFamily,
                                            dstDeviceQueueFamily,
                                            m_image,
                                            m_subresourceRange};
        m_texture->SetImageLayout(m_dstLayout);
        m_texture->SetAccess(dstAccess, dstPipelineStages, dstQueueFamily);
        return std::make_pair(imageBarrier, srcPipelineStages);
    }

    SingleResourcePipelineBarrier::SingleResourcePipelineBarrier(const LogicalDevice* device, ResourceType resource,
                                                                 vk::AccessFlags dstAccess,
                                                                 vk::PipelineStageFlags dstPipelineStages)
        : m_device{device}, m_resource{std::move(resource)}
        , m_dstAccess{dstAccess}
        , m_dstPipelineStages{dstPipelineStages}
    {
    }

    SingleResourcePipelineBarrier::SingleResourcePipelineBarrier(const LogicalDevice* device, Texture* texture,
                                                                 vk::Image image, vk::ImageLayout dstImageLayout,
                                                                 vk::AccessFlags dstAccess,
                                                                 vk::PipelineStageFlags dstPipelineStages)
        : SingleResourcePipelineBarrier{device, ImageBarrierInfo{texture, image, dstImageLayout}, dstAccess,
                                        dstPipelineStages}
    {
    }

    inline void SingleResourcePipelineBarrier::Record(const CommandBuffer& cmdBuffer)
    {
        if (std::holds_alternative<ImageBarrierInfo>(m_resource)) {
            const auto& barrierInfo = std::get<ImageBarrierInfo>(m_resource);
            auto [imageBarrier, srcPipelineStages] = barrierInfo.CreateBarrier(m_device, m_dstAccess, m_dstPipelineStages, cmdBuffer.GetQueueFamily());
            cmdBuffer.GetHandle().pipelineBarrier(srcPipelineStages, m_dstPipelineStages, vk::DependencyFlags{}, {}, {},
                                                  imageBarrier);
        } else if (std::holds_alternative<BufferBarrierInfo>(m_resource)) {
            [[maybe_unused]] auto& bufferInfo = std::get<BufferBarrierInfo>(m_resource);
            // auto [srcAccess, srcPipelineStages, srcQueueFamily] = bufferInfo.m_buffer->GetPreviousAccess();
            //
            // vk::BufferMemoryBarrier bufferBarrier{srcAccess,
            //                                     m_dstAccess,
            //                                     srcQueueFamily,
            //                                     m_dstQueueFamily,
            //                                     bufferInfo.m_vkBuffer,
            //                                     bufferInfo.m_offset, bufferInfo.m_size};
            // cmdBuffer.GetHandle().pipelineBarrier(srcPipelineStages, m_dstPipelineStages, vk::DependencyFlags{}, {}, bufferBarrier, {});
            // bufferInfo.m_buffer->SetAccess(m_dstAccess, m_dstPipelineStages, m_dstQueueFamily);
        }
    }

    PipelineBarrier::PipelineBarrier(const LogicalDevice* device, vk::PipelineStageFlags dstPipelineStages)
        : m_device{device}, m_dstPipelineStages{dstPipelineStages}
    {
    }

    void PipelineBarrier::AddSingleBarrier(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout,
                                           vk::AccessFlags dstAccess, vk::PipelineStageFlags dstPipelineStages)
    {
        m_dstPipelineStages |= dstPipelineStages;
        m_resources.emplace_back(ImageBarrierInfo{texture, image, dstImageLayout}, dstAccess);
    }

    void PipelineBarrier::Record(const CommandBuffer& cmdBuffer)
    {
        std::vector<vk::ImageMemoryBarrier> imageBarriers;
        std::vector<vk::BufferMemoryBarrier> bufferBarriers;
        vk::PipelineStageFlags totalSrcPipelineStages;

        Record(imageBarriers, bufferBarriers, totalSrcPipelineStages, cmdBuffer.GetQueueFamily());

        if (totalSrcPipelineStages == vk::PipelineStageFlags{} && m_dstPipelineStages == vk::PipelineStageFlags{} && bufferBarriers.empty() && imageBarriers.empty()) {
            return;
        }

        cmdBuffer.GetHandle().pipelineBarrier(totalSrcPipelineStages, m_dstPipelineStages, vk::DependencyFlags{}, {},
                                              bufferBarriers, imageBarriers);
    }

    void PipelineBarrier::Record(std::vector<vk::ImageMemoryBarrier>& imageBarriers,
                                 [[maybe_unused]] std::vector<vk::BufferMemoryBarrier>& bufferBarriers,
                                 vk::PipelineStageFlags& totalSrcPipelineStages, unsigned int queueFamily)
    {
        for (const auto& barrierEntryInfo : m_resources) {
            if (std::holds_alternative<ImageBarrierInfo>(barrierEntryInfo.m_resource)) {
                auto& barrierInfo = std::get<ImageBarrierInfo>(barrierEntryInfo.m_resource);
                auto [imageBarrier, srcPipelineStages] = barrierInfo.CreateBarrier(
                    m_device, barrierEntryInfo.m_dstAccess, m_dstPipelineStages, queueFamily);

                totalSrcPipelineStages |= srcPipelineStages;
                imageBarriers.emplace_back(std::move(imageBarrier));
            } else if (std::holds_alternative<BufferBarrierInfo>(barrierEntryInfo.m_resource)) {
                [[maybe_unused]] auto& bufferInfo = std::get<BufferBarrierInfo>(barrierEntryInfo.m_resource);
                // auto [srcAccess, srcPipelineStages, srcQueueFamily] = bufferInfo.m_buffer->GetPreviousAccess();
                //
                // vk::BufferMemoryBarrier bufferBarrier{srcAccess,
                //                                     m_dstAccess,
                //                                     srcQueueFamily,
                //                                     m_dstQueueFamily,
                //                                     bufferInfo.m_vkBuffer,
                //                                     bufferInfo.m_offset, bufferInfo.m_size};
                // cmdBuffer.GetHandle().pipelineBarrier(srcPipelineStages, m_dstPipelineStages, vk::DependencyFlags{}, {}, bufferBarrier, {});
                // bufferInfo.m_buffer->SetAccess(m_dstAccess, m_dstPipelineStages, m_dstQueueFamily);
            }
        }
    }
}
