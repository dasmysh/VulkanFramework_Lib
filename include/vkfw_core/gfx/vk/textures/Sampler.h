/**
 * @file   Sampler.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.17
 *
 * @brief  Declaration of a Vulkan sampler object.
 */

#pragma once

#include "main.h"

namespace vku::gfx {

    class Sampler
    {
    public:
        Sampler();

      private:
        /** Holds the Vulkan sampler. */
        vk::Sampler vkSampler_;
    };
}
