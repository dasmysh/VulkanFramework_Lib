/**
 * @file   constants.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Contains global constant definitions.
 */

#pragma once

namespace vku {
    /** The engine name. */
    static const char* engineName = "VKFW";
    /** The engine version. */
    static const uint32_t engineVersionMajor = 0;
    static const uint32_t engineVersionMinor = 1;
    static const uint32_t engineVersionPatch = 0;
    static const uint32_t engineVersion = (((engineVersionMajor) << 22) | ((engineVersionMinor) << 12) | (engineVersionPatch));
}
