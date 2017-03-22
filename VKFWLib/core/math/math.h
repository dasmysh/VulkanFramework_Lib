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
     *  2D cross product.
     *  @param real the floating point type used.
     *  @param v0 first vector
     *  @param v1 second vector
     *  @return the 2D cross product.
     */
    template<typename real>
    real crossz(const glm::tvec2<real, glm::highp>& v0, const glm::tvec2<real, glm::highp>& v1) {
        return (v0.x * v1.y) - (v0.y * v1.x);
    }

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
}
