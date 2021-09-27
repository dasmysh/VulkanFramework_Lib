/**
 * @file   Sampler.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.17
 *
 * @brief  Definition of the Vulkan sampler object.
 */

#include "gfx/vk/textures/Sampler.h"

namespace vkfw_core::gfx {

    Sampler::Sampler()
    {
        //vk::SamplerCreateInfo samplerCreateInfo{ vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear,
        // vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, };
        assert(false && "Not Implemented!");
    }
}
