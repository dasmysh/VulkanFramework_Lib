/**
 * @file   VKRAIIWrapper.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.18
 *
 * @brief  Initialization of wrappers for Vulkan object to allow RAII patterns.
 */

#include "VKRAIIWrapper.h"

namespace vku {
    const vk::Instance InstanceObjectTraits::null_obj = vk::Instance();
}
