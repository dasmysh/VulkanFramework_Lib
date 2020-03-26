/**
 * @file   constants.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Contains global constant definitions.
 */

#pragma once

#include <cstdint>

namespace vku {
    /** The engine name. */
    static const char* engineName = "VKFW";
    /** The engine version. */
    static const std::uint32_t engineVersionMajor = 0;
    static const std::uint32_t engineVersionMinor = 1;
    static const std::uint32_t engineVersionPatch = 0;
    static const std::uint32_t engineVersion = (((engineVersionMajor) << 22U) | ((engineVersionMinor) << 12U) | (engineVersionPatch));

    static const std::uint64_t defaultFenceTimeout = 100000000000U;
}
