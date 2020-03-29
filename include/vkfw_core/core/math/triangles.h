/**
 * @file   triangles.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.07.27
 *
 * @brief  Definition triangle helper functions.
 */

#pragma once

#include "primitives.h"
#include <random>

namespace vkfw_core::math {

    // Triangle properties consist of:
    // - vec(circumscribed circle center, radius)
    // - vec t[1]-t[0]
    // - vec t[2]-t[0]
    // - vec(normal, length of normal=2*area)
    template<typename real>
    using TriangleProperties = std::tuple<glm::tvec4<real, glm::highp>,
        glm::tvec3<real, glm::highp>, glm::tvec3<real, glm::highp>, glm::tvec4<real, glm::highp>>;

    template<typename real>
    TriangleProperties<real> CalcTriangleProperties(const Tri3<real>& tri) {
        auto a = tri[1] - tri[0];
        auto b = tri[2] - tri[0];
        auto axb = glm::cross(a, b);
        auto la = glm::length(a);
        auto lb = glm::length(a);
        auto laxb = glm::length(axb);
        auto lamb = glm::length(a - b);
        auto laxb2 = 2.0f * laxb;

        auto r = (la*lb*lamb) / (laxb2);
        auto p0 = ((glm::cross(la*la*b - lb*lb*a, axb)) / (laxb * laxb2)) + tri[0];

        return std::make_tuple(glm::tvec4<real, glm::highp>(p0, r), a, b, glm::tvec4<real, glm::highp>(axb, laxb));
    }

    template<typename real>
    glm::tvec4<real, glm::highp> CalculateCircumsphere(const Tri3<real>& tri) {
        return std::get<0>(CalcTriangleProperties(tri));
    }

    template<typename real, typename GEN, typename DIST = std::uniform_real_distribution<real>>
    glm::tvec3<real, glm::highp> SampleTriangleBarycentric(GEN& randomGenerator)
    {
        DIST dist;
        auto sqrtR1 = glm::sqrt(dist(randomGenerator));
        auto r2 = dist(randomGenerator);
        return glm::tvec3<real, glm::highp>(1.0f - sqrtR1, sqrtR1 * (1.0f - r2), r2 * sqrtR1);
    }

    template<typename real, typename GEN, typename DIST = std::uniform_real_distribution<real>>
    glm::tvec3<real, glm::highp> SampleTriangleThirdBarycentric(GEN& randomGenerator)
    {
        // Idea: x > y (1); x > z (2) => solve for r1
        // (1) -> sqrtR1 < 1 / (2 - r2)
        // (2) -> sqrtR1 < 1 / (r2 + 1)
        // => r2 > 0.5 --> (2) else (1)
        DIST r2dist;
        auto r2 = r2dist(randomGenerator);
        auto r1Max = (r2 > static_cast<real>(0.5)) ? (static_cast<real>(1.0) / (r2 + static_cast<real>(1.0)))
            : (static_cast<real>(1.0) / (static_cast<real>(2.0) - r2));
        r1Max *= r1Max;

        DIST r1dist(0.0f, r1Max);
        auto sqrtR1 = glm::sqrt(r1dist(randomGenerator));
        return glm::tvec3<real, glm::highp>(1.0f - sqrtR1, sqrtR1 * (1.0f - r2), r2 * sqrtR1);
    }

    template<typename real, typename GEN, typename DIST = std::uniform_real_distribution<real>>
    glm::tvec3<real, glm::highp> SampleTriangle(const Tri3<real>& tri, std::mt19937& randomGenerator)
    {
        auto bc = SampleTriangleBarycentric<real, GEN, DIST>(randomGenerator);
        return bc.x * tri[0] + bc.y * tri[1] + bc.z * tri[2];
    }
}
