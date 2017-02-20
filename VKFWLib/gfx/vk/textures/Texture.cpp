/**
 * @file   Texture.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.02.19
 *
 * @brief  Implementation of a texture object for Vulkan.
 */

#include "Texture.h"
#include "gfx/vk/LogicalDevice.h"
#include "gfx/vk/buffers/BufferGroup.h"

namespace vku { namespace gfx {

    TextureDescriptor TextureDescriptor::StagingTextureDesc(unsigned bytesPP, vk::Format format, vk::SampleCountFlagBits samples)
    {
        TextureDescriptor texDesc(bytesPP, format, samples);
        texDesc.createFlags_ = vk::ImageCreateFlags();
        texDesc.imageTiling_ = vk::ImageTiling::eLinear;
        texDesc.imageUsage_ = vk::ImageUsageFlagBits::eTransferSrc;
        texDesc.sharingMode_ = vk::SharingMode::eExclusive;
        texDesc.imageLayout_ = vk::ImageLayout::ePreinitialized;
    }

    Texture::Texture(const LogicalDevice* device, const glm::u64vec4& size, uint32_t mipLevels,
        const TextureDescriptor& desc, const std::vector<uint32_t>& queueFamilyIndices) :
        device_{ device },
        imageDeviceMemory_{ device, desc.memoryProperties_ },
        size_{ size.x * desc.bytesPP_, size.y, size.z, size.w },
        mipLevels_{ mipLevels },
        desc_{ desc },
        queueFamilyIndices_{ queueFamilyIndices }
    {
        assert(size_.x > 0);
        assert(size.y > 0);
        assert(size.z > 0);
        assert(size.w > 0);
        assert(mipLevels_ > 0);
        vk::ImageType type = vk::ImageType::e3D;
        if (size.z == 1 && size.y == 1) type = vk::ImageType::e1D;
        else if (size.z == 1) type = vk::ImageType::e2D;

        vk::ImageCreateInfo imgCreateInfo{ desc_.createFlags_, type, desc_.format_,
            vk::Extent3D(size.x, size_.y, size_.z), mipLevels, size_.w, desc_.samples_, desc_.imageTiling_,
            desc_.imageUsage_, desc_.sharingMode_, 0, nullptr, desc_.imageLayout_ };
        if (queueFamilyIndices_.size() > 0) {
            imgCreateInfo.setQueueFamilyIndexCount(static_cast<uint32_t>(queueFamilyIndices_.size()));
            imgCreateInfo.setPQueueFamilyIndices(queueFamilyIndices_.data());
        }

        vkImage_ = device_->GetDevice().createImage(imgCreateInfo);

        vk::MemoryRequirements memRequirements = device_->GetDevice().getImageMemoryRequirements(vkImage_);
        imageDeviceMemory_.InitializeMemory(memRequirements);
        imageDeviceMemory_.BindToTexture(*this, 0);
    }
}}
