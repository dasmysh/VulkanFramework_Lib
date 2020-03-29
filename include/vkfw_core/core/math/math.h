/**
 * @file   math.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2015.07.27
 *
 * @brief  Contains frequently used math constants and methods.
 */

#pragma once

#include "core/math/primitives.h"

#include <cmath>

namespace vkfw_core::math {

    /**
     *  Modulo operator that correctly simulates a ring.
     *  @param i the value to modulo
     *  @param N the modulo
     */
    inline int betterModulo(int i, int N)
    {
        return (i % N + N) % N;
    }

    /**
     *  Rounds up to the next power of two.
     *  @param x value to round up.
     *  @return the rounded up value.
     */
    inline std::uint32_t roundupPow2(std::uint32_t x)
    {
        if (x <= 1) { return 2; }
        --x;
        x |= x >> 1U;
        x |= x >> 2U;
        x |= x >> 4U;
        x |= x >> 8U;
        x |= x >> 16U;
        return x + 1;
    }

    /**
     *  Returns the next power of two (even if x is already a power of two).
     *  @param x value to round up.
     *  @return the next power of two value.
     */
    inline std::uint32_t nextPow2(std::uint32_t x)
    {
        std::uint32_t j;
        std::uint32_t k;
        // ReSharper disable CppUsingResultOfAssignmentAsCondition
        ((j = x & 0xFFFF0000) != 0) || ((j = x) != 0); // NOLINT(clang-diagnostic-unused-value,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
        ((k = j & 0xFF00FF00) != 0) || ((k = j) != 0); // NOLINT(clang-diagnostic-unused-value,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
        ((j = k & 0xF0F0F0F0) != 0) || ((j = k) != 0); // NOLINT(clang-diagnostic-unused-value,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
        ((k = j & 0xCCCCCCCC) != 0) || ((k = j) != 0); // NOLINT(clang-diagnostic-unused-value,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
        ((j = k & 0xAAAAAAAA) != 0) || ((j = k) != 0); // NOLINT(clang-diagnostic-unused-value,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
        // ReSharper restore CppUsingResultOfAssignmentAsCondition
        return j << 1U;
    }


    /**
     *  Tests if a AABB3 is inside or intersected by a Frustum (culling test).
     *  @param real the floating point type used.
     *  @param f the frustum.
     *  @param b the box.
     */
    template<typename real> bool AABBInFrustumTest(const Frustum<real>& f, const AABB3<real>& b) {
        auto& bmax = b.minmax_[1];
        for (const auto& plane : f.planes_) {
        // for (unsigned int i = 0; i < 6; ++i) {
            // auto& plane = f.planes_[i];
            glm::vec3 p{ b.minmax_[0] };
            if (plane.x >= 0) { p.x = bmax.x; }
            if (plane.y >= 0) { p.y = bmax.y; }
            if (plane.z >= 0) { p.z = bmax.z; }

            // is the positive vertex outside?
            if ((glm::dot(glm::vec3(plane), p) + plane.w) < 0) { return false; }
        }
        return true;
    }
}
