/**
 * @file   primitives.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2015.07.28
 *
 * @brief  Definition of geometric primitives.
 */

#pragma once

#include <array>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace vkfw_core::math {

    /** Line for two dimensions using floats. */
    template<typename real> using Line2 = std::array<glm::tvec2<real, glm::highp>, 2>;
    /** Line for three dimensions using floats. */
    template<typename real> using Line3 = std::array<glm::tvec3<real, glm::highp>, 2>;

    /** Triangle for two dimensions using floats. */
    template<typename real> using Tri2 = std::array<glm::tvec2<real, glm::highp>, 3>;
    /** Triangle for three dimensions using floats. */
    template<typename real> using Tri3 = std::array<glm::tvec3<real, glm::highp>, 3>;

    /** Holds the information defining a frustum as used for culling. */
    template<typename real> struct Frustum {
        using mat4 = glm::tmat4x4<real, glm::highp>;
        using vec4 = glm::tvec4<real, glm::highp>;
        constexpr static std::size_t NUM_FRUSTUM_PLANES = 6;

        explicit Frustum(const mat4& mat) noexcept;

        /** The six planes defining the frustum. */
        std::array<vec4, NUM_FRUSTUM_PLANES> planes_;

        /** Returns the left plane of the frustum. */
        [[nodiscard]] vec4& left() { return planes_[0]; }
        /** Returns the left plane of the frustum. */
        [[nodiscard]] const vec4& left() const { return planes_[0]; }
        /** Returns the right plane of the frustum. */
        [[nodiscard]] vec4& right() { return planes_[1]; }
        /** Returns the right plane of the frustum. */
        [[nodiscard]] const vec4& right() const { return planes_[1]; }
        /** Returns the top plane of the frustum. */
        [[nodiscard]] vec4& top() { return planes_[2]; }
        /** Returns the top plane of the frustum. */
        [[nodiscard]] const vec4& top() const { return planes_[2]; }
        /** Returns the bottom plane of the frustum. */
        [[nodiscard]] vec4& bttm() { return planes_[3]; }
        /** Returns the bottom plane of the frustum. */
        [[nodiscard]] const vec4& bttm() const { return planes_[3]; }
        /** Returns the near plane of the frustum. */
        [[nodiscard]] vec4& npln() { return planes_[4]; }
        /** Returns the near plane of the frustum. */
        [[nodiscard]] const vec4& npln() const { return planes_[4]; }
        /** Returns the far plane of the frustum. */
        [[nodiscard]] vec4& fpln() { return planes_[5]; }
        /** Returns the far plane of the frustum. */
        [[nodiscard]] const vec4& fpln() const { return planes_[5]; }
    };

    template<typename real>
    Frustum<real>::Frustum(const Frustum<real>::mat4& mat) noexcept
        :
        planes_{
            glm::row(mat, 3) + glm::row(mat, 0), // left
            glm::row(mat, 3) - glm::row(mat, 0), // right
            glm::row(mat, 3) - glm::row(mat, 1), // top
            glm::row(mat, 3) + glm::row(mat, 1), // bottom
            glm::row(mat, 3) + glm::row(mat, 2), // near
            glm::row(mat, 3) - glm::row(mat, 2) // far
        }
    {
        // left() = glm::row(mat, 3) + glm::row(mat, 0);
        // right() = glm::row(mat, 3) - glm::row(mat, 0);
        // bttm() = glm::row(mat, 3) + glm::row(mat, 1);
        // top() = glm::row(mat, 3) - glm::row(mat, 1);
        // near() = glm::row(mat, 3) + glm::row(mat, 2);
        // far() = glm::row(mat, 3) - glm::row(mat, 2);

        left() /= glm::length(glm::vec3(left()));
        right() /= glm::length(glm::vec3(right()));
        bttm() /= glm::length(glm::vec3(bttm()));
        top() /= glm::length(glm::vec3(top()));
        npln() /= glm::length(glm::vec3(npln()));
        fpln() /= glm::length(glm::vec3(fpln()));
    }

    template<typename real, int N, typename V> struct AABB
    {
        using mat4 = glm::tmat4x4<real, glm::highp>;
        using vec4 = glm::tvec4<real, glm::highp>;

        AABB() noexcept;
        AABB(const V& minValue, const V& maxValue);
        void SetMin(const V&);
        void SetMax(const V&);

        void Transform(const mat4&);
        [[nodiscard]] AABB NewFromTransform(const mat4&) const;

        [[nodiscard]] AABB Union(const AABB& other) const;

        /** Contains the minimum and maximum points of the box. */
        void AddPoint(const V&);
        void FromPoints(const std::vector<V>&);

        std::array<V, 2> minmax_;
    };

    template<typename real, int N, typename V>
    inline AABB<real, N, V>::AABB() noexcept :
        minmax_{ V(std::numeric_limits<real>::max()), V(std::numeric_limits<real>::lowest()) }
    {
    }

    template<typename real, int N, typename V>
    inline AABB<real, N, V>::AABB(const V& minValue, const V& maxValue) :
        minmax_{ minValue, maxValue }
    {
    }

    template<typename real, int N, typename V>
    inline void AABB<real, N, V>::SetMin(const V& v)
    {
        minmax_[0] = v;
    }

    template<typename real, int N, typename V>
    inline void AABB<real, N, V>::SetMax(const V& v)
    {
        minmax_[1] = v;
    }

    template<typename real, int N, typename V, int I> struct AABBInternal
    {
        using mat4 = glm::tmat4x4<real, glm::highp>;
        using vec4 = glm::tvec4<real, glm::highp>;
        using AABB = AABB<real, N, V>;

        static void permuteMultiply(std::vector<V>& result, const mat4& mat, const vec4& v, const AABB& aabb)
        {
            auto v0 = v;
            v0[I] = aabb.minmax_[0][I];
            auto v1 = v;
            v1[I] = aabb.minmax_[1][I];
            AABBInternal<real, N, V, I - 1>::permuteMultiply(result, mat, v0, aabb);
            AABBInternal<real, N, V, I - 1>::permuteMultiply(result, mat, v1, aabb);
        }
    };

    template<typename real, int N, typename V> struct AABBInternal<real, N, V, 0>
    {
        using mat4 = glm::tmat4x4<real, glm::highp>;
        using vec4 = glm::tvec4<real, glm::highp>;
        using AABB = AABB<real, N, V>;

        static void permuteMultiply(std::vector<V>& result, const mat4& mat,
                                    const vec4& v,
                                    const AABB& aabb)
        {
            auto v0 = v;
            v0[0] = aabb.minmax_[0][0];
            auto v1 = v;
            v1[0] = aabb.minmax_[1][0];
            result.emplace_back(mat * v0);
            result.emplace_back(mat * v1);
        }
    };

    template<typename real, int N, typename V>
    inline void AABB<real, N, V>::Transform(const AABB<real, N, V>::mat4& mat)
    {
        std::vector<V> newCorners;
        AABBInternal<real, N, V, N - 1>::permuteMultiply(newCorners, mat, vec4{1.0f}, *this);

        FromPoints(newCorners);
    }

    template<typename real, int N, typename V>
    inline AABB<real, N, V> AABB<real, N, V>::NewFromTransform(const AABB<real, N, V>::mat4& mat) const
    {
        auto tmp = *this;
        tmp.Transform(mat);
        return tmp;
    }

    template<typename real, int N, typename V>
    inline AABB<real, N, V> AABB<real, N, V>::Union(const AABB& other) const
    {
        const auto newMin = glm::min(minmax_[0], other.minmax_[0]);
        const auto newMax = glm::max(minmax_[1], other.minmax_[1]);

        return AABB(newMin, newMax);
    }

    template<typename real, int N, typename V>
    inline void AABB<real, N, V>::AddPoint(const V& point)
    {
        minmax_[0] = glm::min(minmax_[0], point);
        minmax_[1] = glm::max(minmax_[1], point);
    }

    template<typename real, int N, typename V>
    inline void AABB<real, N, V>::FromPoints(const std::vector<V>& points)
    {
        if (points.empty()) { return; }

        minmax_[0] = glm::vec3(std::numeric_limits<float>::max());
        minmax_[1] = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& point : points) { AddPoint(point); }
    }

    template<typename real> using AABB2 = AABB<real, 2, glm::tvec2<real, glm::highp>>;
    template<typename real> using AABB3 = AABB<real, 3, glm::tvec3<real, glm::highp>>;
}
