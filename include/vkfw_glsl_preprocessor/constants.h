/**
 * @file   constants.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.03.29
 *
 * @brief  Contains global constant definitions.
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace vkfw_glsl {

    /** The log file name. */
    constexpr const char* logFileName = "vkfw_glsl_preprocessor.log";
    /** Log file application tag. */
    constexpr const char* logTag = "vkfw_glsl";

    constexpr std::size_t max_shader_recursion_depth = 32;

#ifdef NDEBUG
    constexpr bool debug_build = false;
#else
    constexpr bool debug_build = true;
#endif

}
