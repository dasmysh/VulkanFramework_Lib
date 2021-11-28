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

    vk::ImageMemoryBarrier2KHR ImageBarrierInfo::CreateBarrier(
        const LogicalDevice* device, std::vector<std::unique_ptr<PipelineBarrier>>& releaseBarriers,
        vk::AccessFlags2KHR dstAccess, vk::PipelineStageFlags2KHR dstPipelineStages, unsigned int dstQueueFamily) const
    {
        auto [srcAccess, srcPipelineStages, srcQueueFamily] = m_texture->GetPreviousAccess();
        if (srcQueueFamily == MemoryBoundResource::INVALID_QUEUE_FAMILY) { srcQueueFamily = dstQueueFamily; }
        const bool releaseBarrier = srcQueueFamily != dstQueueFamily && !releaseBarriers.empty();
        const bool acquireBarrier = srcQueueFamily != dstQueueFamily && releaseBarriers.empty();

        if (releaseBarrier) {
            dstAccess = vk::AccessFlagBits2KHR::eNone;
            dstPipelineStages = vk::PipelineStageFlagBits2KHR::eNone;
        } else if (acquireBarrier) {
            srcAccess = vk::AccessFlagBits2KHR::eNone;
            srcPipelineStages = vk::PipelineStageFlagBits2KHR::eNone;
        }

        auto srcDeviceQueueFamily = device->GetQueueInfo(srcQueueFamily).m_familyIndex;
        auto dstDeviceQueueFamily = device->GetQueueInfo(dstQueueFamily).m_familyIndex;

        vk::ImageMemoryBarrier2KHR imageBarrier{srcPipelineStages,
                                                srcAccess,
                                                dstPipelineStages,
                                                dstAccess,
                                                m_texture->GetImageLayout(),
                                                m_dstLayout,
                                                srcDeviceQueueFamily,
                                                dstDeviceQueueFamily,
                                                m_image,
                                                m_subresourceRange};

        if (releaseBarrier) {
            if (!releaseBarriers[srcQueueFamily]) {
                releaseBarriers[srcQueueFamily] = std::make_unique<PipelineBarrier>(device);
            }
            releaseBarriers[srcQueueFamily]->AddSingleBarrier(m_texture, m_image, m_dstLayout, dstAccess,
                                                              dstPipelineStages);
        } else {
            m_texture->SetImageLayout(m_dstLayout);
            m_texture->SetAccess(dstAccess, dstPipelineStages, dstQueueFamily);
        }
        return imageBarrier;
    }

    BufferBarrierInfo::BufferBarrierInfo(Buffer* buffer, vk::Buffer vkBuffer, bool isDynamic)
        : m_bufferRange{buffer, 0, buffer->GetSize()}
        , m_buffer{vkBuffer}, m_isDynamic{isDynamic}
    {
    }

    BufferBarrierInfo::BufferBarrierInfo(BufferRange bufferRange, vk::Buffer buffer, bool isDynamic)
        : m_bufferRange{bufferRange}, m_buffer{buffer}, m_isDynamic{isDynamic}
    {
    }

    void BufferBarrierInfo::AddBarriers(
        const LogicalDevice* device, std::vector<vk::BufferMemoryBarrier2KHR>& bufferBarriers,
        std::vector<std::unique_ptr<PipelineBarrier>>& releaseBarriers,
        vk::AccessFlags2KHR dstAccess, vk::PipelineStageFlags2KHR dstPipelineStages, unsigned int dstQueueFamily, std::uint32_t dynamicOffset) const
    {
        auto bufferRanges =
            m_bufferRange.m_buffer->GetOverlappingRanges(m_bufferRange.m_offset + dynamicOffset, m_bufferRange.m_range);
        for (const auto& bufferRange : bufferRanges) {
            auto [srcAccess, srcPipelineStages, srcQueueFamily] = bufferRange.GetPreviousAccess();
            std::size_t offset = bufferRange.m_offset, range = bufferRange.m_range;

            if (srcQueueFamily == MemoryBoundResource::INVALID_QUEUE_FAMILY) { srcQueueFamily = dstQueueFamily; }
            const bool releaseBarrier = srcQueueFamily != dstQueueFamily && !releaseBarriers.empty();
            const bool acquireBarrier = srcQueueFamily != dstQueueFamily && releaseBarriers.empty();

            if (releaseBarrier) {
                dstAccess = vk::AccessFlagBits2KHR::eNone;
                dstPipelineStages = vk::PipelineStageFlagBits2KHR::eNone;
            } else if (acquireBarrier) {
                srcAccess = vk::AccessFlagBits2KHR::eNone;
                srcPipelineStages = vk::PipelineStageFlagBits2KHR::eNone;
            }

            auto srcDeviceQueueFamily = device->GetQueueInfo(srcQueueFamily).m_familyIndex;
            auto dstDeviceQueueFamily = device->GetQueueInfo(dstQueueFamily).m_familyIndex;

            if (releaseBarrier) {
                if (!releaseBarriers[srcQueueFamily]) {
                    releaseBarriers[srcQueueFamily] = std::make_unique<PipelineBarrier>(device);
                }
                releaseBarriers[srcQueueFamily]->AddSingleBarrier(BufferRange{m_bufferRange.m_buffer, offset, range},
                                                                  m_buffer, false, dstAccess, dstPipelineStages);
            } else {
                m_bufferRange.m_buffer->SetAccess(offset, range, dstAccess, dstPipelineStages, dstQueueFamily);
            }
            bufferBarriers.emplace_back(srcPipelineStages, srcAccess, dstPipelineStages, dstAccess,
                                        srcDeviceQueueFamily, dstDeviceQueueFamily, m_buffer, offset, range);
        }
        m_bufferRange.m_buffer->ReduceRanges();
    }

    PipelineBarrier::PipelineBarrier(const LogicalDevice* device)
        : m_device{device}
    {
    }

    void PipelineBarrier::AddSingleBarrier(Texture* texture, vk::Image image, vk::ImageLayout dstImageLayout,
                                           vk::AccessFlags2KHR dstAccess, vk::PipelineStageFlags2KHR dstPipelineStages)
    {
        m_resources.emplace_back(ImageBarrierInfo{texture, image, dstImageLayout}, dstAccess, dstPipelineStages);
    }

    void PipelineBarrier::AddSingleBarrier(Buffer* buffer, vk::Buffer vkBuffer, bool isDynamic, vk::AccessFlags2KHR dstAccess,
                                           vk::PipelineStageFlags2KHR dstPipelineStages)
    {
        m_resources.emplace_back(BufferBarrierInfo{buffer, vkBuffer, isDynamic}, dstAccess, dstPipelineStages);
    }

    void PipelineBarrier::AddSingleBarrier(BufferRange bufferRange, vk::Buffer buffer, bool isDynamic,
                                           vk::AccessFlags2KHR dstAccess, vk::PipelineStageFlags2KHR dstPipelineStages)
    {
        m_resources.emplace_back(BufferBarrierInfo{bufferRange, buffer, isDynamic}, dstAccess, dstPipelineStages);
    }

    void PipelineBarrier::Record(CommandBuffer& cmdBuffer, const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets)
    {
        std::vector<vk::ImageMemoryBarrier2KHR> imageBarriers;
        std::vector<vk::BufferMemoryBarrier2KHR> bufferBarriers;
        unsigned int dstQueueFamily = cmdBuffer.GetQueueFamily();

        std::vector<std::unique_ptr<PipelineBarrier>> releaseBarriers;
        releaseBarriers.resize(m_device->GetWindowCfg().m_queues.size());
        Record(imageBarriers, bufferBarriers, releaseBarriers, dstQueueFamily, dynamicOffsets);

        if (bufferBarriers.empty() && imageBarriers.empty()) {
            return;
        }


        for (std::size_t i = 0; i < releaseBarriers.size(); ++i) {
            auto& releaseBarrier = releaseBarriers[i];
            if (releaseBarrier) {
                auto releaseCmdBuffer = std::make_shared<CommandBuffer>(
                    CommandBuffer::beginSingleTimeSubmit(m_device, "ReleaseBarrierCmdBuffer", "ReleaseBarrier",
                                                         m_device->GetCommandPool(static_cast<unsigned int>(i))));
                releaseBarrier->RecordRelease(*releaseCmdBuffer, dstQueueFamily);
                auto signalSemaphore = cmdBuffer.AddWaitSemaphore();
                std::array<vk::Semaphore, 1> signalSemaphoreArray = {signalSemaphore->GetHandle()};
                auto submitFence = CommandBuffer::endSingleTimeSubmit(
                    m_device->GetQueue(static_cast<unsigned int>(i), 0), *releaseCmdBuffer, {}, signalSemaphoreArray);

                m_device->GetResourceReleaser().AddResource(submitFence, releaseCmdBuffer);
                m_device->GetResourceReleaser().AddResource(submitFence, signalSemaphore);
            }
        }

        cmdBuffer.GetHandle().pipelineBarrier2KHR(
            vk::DependencyInfoKHR{vk::DependencyFlags{}, {}, bufferBarriers, imageBarriers});
    }

    void PipelineBarrier::RecordRelease(CommandBuffer& cmdBuffer, unsigned int dstQueueFamily)
    {
        std::vector<vk::ImageMemoryBarrier2KHR> imageBarriers;
        std::vector<vk::BufferMemoryBarrier2KHR> bufferBarriers;
        std::vector<std::unique_ptr<PipelineBarrier>> releaseBarriers;

        Record(imageBarriers, bufferBarriers, releaseBarriers, dstQueueFamily);

        if (bufferBarriers.empty() && imageBarriers.empty()) {
            return;
        }

        cmdBuffer.GetHandle().pipelineBarrier2KHR(
            vk::DependencyInfoKHR{vk::DependencyFlags{}, {}, bufferBarriers, imageBarriers});
    }

    void PipelineBarrier::Record(std::vector<vk::ImageMemoryBarrier2KHR>& imageBarriers,
                                 std::vector<vk::BufferMemoryBarrier2KHR>& bufferBarriers,
                                 std::vector<std::unique_ptr<PipelineBarrier>>& releaseBarriers,
                                 unsigned int dstQueueFamily, const vk::ArrayProxy<const std::uint32_t>& dynamicOffsets)
    {
        std::size_t dynamicIndex = 0;
        for (const auto& barrierEntryInfo : m_resources) {
            if (std::holds_alternative<ImageBarrierInfo>(barrierEntryInfo.m_resource)) {
                auto& barrierInfo = std::get<ImageBarrierInfo>(barrierEntryInfo.m_resource);
                auto imageBarrier = barrierInfo.CreateBarrier(m_device, releaseBarriers, barrierEntryInfo.m_dstAccess,
                                                              barrierEntryInfo.m_dstPipelineStages, dstQueueFamily);
                imageBarriers.emplace_back(std::move(imageBarrier));
            } else if (std::holds_alternative<BufferBarrierInfo>(barrierEntryInfo.m_resource)) {
                auto& barrierInfo = std::get<BufferBarrierInfo>(barrierEntryInfo.m_resource);
                std::uint32_t dynamicOffset = 0;
                if (barrierInfo.m_isDynamic && dynamicOffsets.size() > dynamicIndex) {
                    dynamicOffset = dynamicOffsets.data()[dynamicIndex];
                    dynamicIndex += 1;
                }
                barrierInfo.AddBarriers(m_device, bufferBarriers, releaseBarriers, barrierEntryInfo.m_dstAccess,
                                        barrierEntryInfo.m_dstPipelineStages, dstQueueFamily, dynamicOffset);
            }
        }
    }
}
