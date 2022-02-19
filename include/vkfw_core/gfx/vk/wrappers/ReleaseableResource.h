/**
 * @file   ReleaseableResource.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.10.29
 *
 * @brief  Base class for resources that need to wait for their release.
 */

#pragma once

#include <memory>
#include "main.h"

namespace vkfw_core::gfx {

    class ReleaseableResource
    {
    public:
        virtual ~ReleaseableResource() = default;
    };
}
