/**
 * @file   MemoryGroup.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.20
 *
 * @brief  Implementation of the MemoryGroup class.
 */

#include "gfx/vk/memory/MemoryGroup.h"
#include "gfx/vk/QueuedDeviceTransfer.h"

namespace vkfw_core::gfx {

    template<class T> struct always_false : std::false_type
    {
    };

    MemoryGroup::MemoryGroup(const LogicalDevice* device, std::string_view name,
                             const vk::MemoryPropertyFlags& memoryFlags)
        :
        DeviceMemoryGroup(device, name, memoryFlags), m_hostMemory{device, fmt::format("Host:{}", name),
                       memoryFlags | vk::MemoryPropertyFlagBits::eHostVisible
                           | vk::MemoryPropertyFlagBits::eHostCoherent}
    {
    }

    MemoryGroup::~MemoryGroup() = default;
    MemoryGroup::MemoryGroup(MemoryGroup&& rhs) noexcept = default;
    MemoryGroup& MemoryGroup::operator=(MemoryGroup&& rhs) noexcept = default;

    unsigned int MemoryGroup::AddBufferToGroup(std::string_view name, const vk::BufferUsageFlags& usage,
                                               const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = DeviceMemoryGroup::AddBufferToGroup(name, usage, queueFamilyIndices);
        m_hostBuffers.emplace_back(GetDevice(), fmt::format("Host:{}", name), vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlags(),
                                   queueFamilyIndices);
        return idx;
    }

    unsigned int MemoryGroup::AddBufferToGroup(std::string_view name, const vk::BufferUsageFlags& usage,
                                               std::size_t size, const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = DeviceMemoryGroup::AddBufferToGroup(name, usage, size, queueFamilyIndices);

        m_hostBuffers.emplace_back(GetDevice(), fmt::format("Host:{}", name), vk::BufferUsageFlagBits::eTransferSrc,
                                   vk::MemoryPropertyFlags(), queueFamilyIndices);
        m_hostBuffers.back().InitializeBuffer(size, false);

        return idx;
    }

    unsigned int MemoryGroup::AddBufferToGroup(std::string_view name, const vk::BufferUsageFlags& usage,
                                               std::size_t size,
                                               std::variant<void*, const void*> data,
                                               const std::function<void(void*)>& deleter,
                                               const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = AddBufferToGroup(name, usage, size, queueFamilyIndices);
        AddDataToBufferInGroup(idx, 0, size, data, deleter);
        return idx;
    }

    void MemoryGroup::AddDataToBufferInGroup(unsigned int bufferIdx, std::size_t offset, std::size_t dataSize,
                                             std::variant<void*, const void*> data,
                                             const std::function<void(void*)>& deleter)
    {
        assert(std::holds_alternative<void*>(data) || !deleter);
        BufferContentsDesc bufferContentsDesc;
        bufferContentsDesc.m_bufferIdx = bufferIdx;
        bufferContentsDesc.m_offset = offset;
        bufferContentsDesc.m_size = dataSize;
        if (deleter) {

            bufferContentsDesc.m_data = data;
            bufferContentsDesc.m_deleter = deleter;
        } else {
            bufferContentsDesc.m_data = std::visit(
                [](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, void*>) {
                        return const_cast<const void*>(arg);
                    } else if constexpr (std::is_same_v<T, const void*>) {
                        return arg;
                    }
                    else static_assert(always_false<T>::value, "non-exhaustive visitor!");
                },
                data);
            bufferContentsDesc.m_deleter = nullptr;
        }
        m_bufferContents.push_back(bufferContentsDesc);
    }

    unsigned int MemoryGroup::AddTextureToGroup(std::string_view name, const TextureDescriptor& desc,
                                                vk::ImageLayout initialLayout, const glm::u32vec4& size,
                                                std::uint32_t mipLevels,
                                                const std::vector<std::uint32_t>& queueFamilyIndices)
    {
        auto idx = DeviceMemoryGroup::AddTextureToGroup(name, desc, initialLayout, size, mipLevels, queueFamilyIndices);

        TextureDescriptor stagingTexDesc{desc, vk::ImageUsageFlagBits::eTransferSrc};
        stagingTexDesc.m_imageTiling = vk::ImageTiling::eLinear;
        m_hostImages.emplace_back(GetDevice(), fmt::format("Host:{}", name), stagingTexDesc, initialLayout, queueFamilyIndices);
        m_hostImages.back().InitializeImage(size, mipLevels, false);

        return idx;
    }

    void MemoryGroup::AddDataToTextureInGroup(unsigned int textureIdx, const vk::ImageAspectFlags& aspectFlags,
                                              std::uint32_t mipLevel, std::uint32_t arrayLayer,
                                              const glm::u32vec3& size, std::variant<void*, const void*> data,
                                              const std::function<void(void*)>& deleter)
    {
        assert((deleter && std::holds_alternative<void*>(data))
               || (!deleter && std::holds_alternative<const void*>(data)));
        ImageContentsDesc imgContDesc;
        imgContDesc.m_imageIdx = textureIdx;
        imgContDesc.m_aspectFlags = aspectFlags;
        imgContDesc.m_mipLevel = mipLevel;
        imgContDesc.m_arrayLayer = arrayLayer;
        imgContDesc.m_size = size;
        if (deleter) {

            imgContDesc.m_data = data;
            imgContDesc.m_deleter = deleter;
        } else {
            imgContDesc.m_data = std::visit(
                [](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, void*>) {
                        return const_cast<const void*>(arg);
                    } else if constexpr (std::is_same_v<T, const void*>) {
                        return arg;
                    } else
                        static_assert(always_false<T>::value, "non-exhaustive visitor!");
                },
                data);
            imgContDesc.m_deleter = nullptr;
        }
        m_imageContents.push_back(imgContDesc);
    }

    void MemoryGroup::FinalizeDeviceGroup()
    {
        // check if all buffers are initialized first.
        for (std::size_t i_buffer = 0; i_buffer < m_hostBuffers.size(); ++i_buffer) {
            if (!m_hostBuffers[i_buffer]) {
                std::size_t bufferSize = 0;
                for (const auto& bufferContents : m_bufferContents) {
                    if (bufferContents.m_bufferIdx == i_buffer) {
                        if (bufferSize < bufferContents.m_offset + bufferContents.m_size) {
                            bufferSize = bufferContents.m_offset + bufferContents.m_size;
                        }
                    }
                }
                GetBuffer(static_cast<unsigned int>(i_buffer))->InitializeBuffer(bufferSize, false);
                m_hostBuffers[i_buffer].InitializeBuffer(bufferSize, false);
            }
        }

        InitializeHostMemory(GetDevice(), m_hostOffsets, m_hostBuffers, m_hostImages, m_hostMemory);
        BindHostObjects(m_hostOffsets, m_hostBuffers, m_hostImages, m_hostMemory);
        DeviceMemoryGroup::FinalizeDeviceGroup();
    }

    void MemoryGroup::TransferData(QueuedDeviceTransfer& transfer)
    {
        for (const auto& contentDesc : m_bufferContents) {
            m_hostMemory.CopyToHostMemory(
                m_hostOffsets[contentDesc.m_bufferIdx] + contentDesc.m_offset, contentDesc.m_size,
                                         contentDesc.m_deleter ? std::get<void*>(contentDesc.m_data)
                                                              : std::get<const void*>(contentDesc.m_data));
            if (contentDesc.m_deleter) { contentDesc.m_deleter( std::get<void*>(contentDesc.m_data)); }
        }
        for (const auto& contentDesc : m_imageContents) {
            vk::ImageSubresource imgSubresource{ contentDesc.m_aspectFlags, contentDesc.m_mipLevel, contentDesc.m_arrayLayer };
            auto subresourceLayout = m_hostImages[contentDesc.m_imageIdx].GetSubresourceLayout(imgSubresource);
            m_hostMemory.CopyToHostMemory(m_hostOffsets[contentDesc.m_imageIdx + m_hostBuffers.size()], glm::u32vec3(0),
                                         subresourceLayout, contentDesc.m_size,
                                         contentDesc.m_deleter ? std::get<void*>(contentDesc.m_data)
                                                              : std::get<const void*>(contentDesc.m_data));
            if (contentDesc.m_deleter) { contentDesc.m_deleter(std::get<void*>(contentDesc.m_data)); }
        }

        for (auto i = 0U; i < m_hostBuffers.size(); ++i) {
            transfer.AddTransferToQueue(m_hostBuffers[i], *GetBuffer(i));
        }
        for (auto i = 0U; i < m_hostImages.size(); ++i) {
            transfer.AddTransferToQueue(m_hostImages[i], *GetTexture(i));
        }

        m_bufferContents.clear();
        m_imageContents.clear();
    }

    void MemoryGroup::RemoveHostMemory()
    {
        m_hostBuffers.clear();
        m_hostImages.clear();
        m_hostMemory.~DeviceMemory();
        m_hostOffsets.clear();
    }

    void MemoryGroup::FillUploadBufferCmdBuffer(unsigned int bufferIdx, CommandBuffer& cmdBuffer,
        std::size_t offset, std::size_t dataSize)
    {
        m_hostBuffers[bufferIdx].CopyBufferAsync(offset, *GetBuffer(bufferIdx), offset, dataSize, cmdBuffer);
    }
}
