/**
 * @file   constants.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Contains global constant definitions.
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace vkfw_core {
    /** The engine name. */
    constexpr std::string_view engineName = "VKFW";
    /** The engine version. */
    constexpr std::uint32_t engineVersionMajor = 0;
    constexpr std::uint32_t engineVersionMinor = 1;
    constexpr std::uint32_t engineVersionPatch = 0;
    constexpr std::uint32_t engineVersion = (((engineVersionMajor) << 22U) | ((engineVersionMinor) << 12U) | (engineVersionPatch));

    constexpr std::uint64_t defaultFenceTimeout = 100000000000U;

#ifdef NDEBUG
    constexpr bool debug_build = false;
#else
    constexpr bool debug_build = true;
#endif

#ifdef VKFW_DEBUG_PIPELINE
    constexpr bool use_debug_pipeline = true;
#else
    constexpr bool use_debug_pipeline = false;
#endif
}
