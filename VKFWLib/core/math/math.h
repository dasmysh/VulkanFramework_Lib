/**
 * @file   math.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2015.07.27
 *
 * @brief  Contains frequently used math constants and methods.
 */

#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace vku::math {

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
    inline uint32_t roundupPow2(uint32_t x)
    {
        if (x <= 1) return 2;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x + 1;
    }

    /**
     *  Returns the next power of two (even if x is already a power of two).
     *  @param x value to round up.
     *  @return the next power of two value.
     */
    inline uint32_t nextPow2(uint32_t x)
    {
        uint32_t j, k;
        // ReSharper disable CppUsingResultOfAssignmentAsCondition
        (j = x & 0xFFFF0000) || (j = x);
        (k = j & 0xFF00FF00) || (k = j);
        (j = k & 0xF0F0F0F0) || (j = k);
        (k = j & 0xCCCCCCCC) || (k = j);
        (j = k & 0xAAAAAAAA) || (j = k);
        // ReSharper restore CppUsingResultOfAssignmentAsCondition
        return j << 1;
    }


    /**
     *  Tests if a AABB3 is inside or intersected by a Frustum (culling test).
     *  @param real the floating point type used.
     *  @param f the frustum.
     *  @param b the box.
     */
    template<typename real> bool AABBInFrustumTest(const Frustum<real>& f, const AABB3<real>& b) {
        auto& bmax = b.minmax_[1];
        for (unsigned int i = 0; i < 6; ++i) {
            auto& plane = f.planes_[i];
            glm::vec3 p{ b.minmax_[0] };
            if (plane.x >= 0) p.x = bmax.x;
            if (plane.y >= 0) p.y = bmax.y;
            if (plane.z >= 0) p.z = bmax.z;

            // is the positive vertex outside?
            if ((glm::dot(glm::vec3(plane), p) + plane.w) < 0) return false;
        }
        return true;
    }
}
