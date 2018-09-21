/**
 * @file   primitives.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2015.07.28
 *
 * @brief  Definition of geometric primitives.
 */

#pragma once

#include <array>
#include <glm/glm.hpp>

namespace vku::math {

    template<typename real> using Seg2 = std::array<glm::tvec2<real, glm::highp>, 2>;
    template<typename real> using Seg3 = std::array<glm::tvec3<real, glm::highp>, 2>;

    template<typename real> using Tri2 = std::array<glm::tvec2<real, glm::highp>, 3>;
    template<typename real> using Tri3 = std::array<glm::tvec3<real, glm::highp>, 3>;

    template<typename real> struct Frustum {
        std::array<glm::tvec4<real, glm::highp>, 6> planes_;

        glm::tvec4<real, glm::highp>& left() { return planes_[0]; };
        const glm::tvec4<real, glm::highp>& left() const { return planes_[0]; };
        glm::tvec4<real, glm::highp>& rght() { return planes_[1]; };
        const glm::tvec4<real, glm::highp>& rght() const { return planes_[1]; };
        glm::tvec4<real, glm::highp>& topp() { return planes_[2]; };
        const glm::tvec4<real, glm::highp>& topp() const { return planes_[2]; };
        glm::tvec4<real, glm::highp>& bttm() { return planes_[3]; };
        const glm::tvec4<real, glm::highp>& bttm() const { return planes_[3]; };
        glm::tvec4<real, glm::highp>& nrpl() { return planes_[4]; };
        const glm::tvec4<real, glm::highp>& nrpl() const { return planes_[4]; };
        glm::tvec4<real, glm::highp>& farp() { return planes_[5]; };
        const glm::tvec4<real, glm::highp>& farp() const { return planes_[5]; };
    };

    template<typename real, int N, typename V> struct AABB {
        AABB() noexcept;
        AABB(const V& minValue, const V& maxValue);
        void SetMin(const V&);
        void SetMax(const V&);

        void Transform(const glm::tmat4x4<real, glm::highp>&);
        AABB NewFromTransform(const glm::tmat4x4<real, glm::highp>&) const;

        AABB Union(const AABB& other) const;

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

    template<typename real, int N, typename V, int I>
    struct AABBInternal {
        static void permuteMultiply(std::vector<V>& result, const glm::tmat4x4<real, glm::highp>& mat, const glm::tvec4<real, glm::highp>& v, const AABB<real, N, V>& aabb)
        {
            auto v0 = v;
            v0[I] = aabb.minmax_[0][I];
            auto v1 = v;
            v1[I] = aabb.minmax_[1][I];
            AABBInternal<real, N, V, I - 1>::permuteMultiply(result, mat, v0, aabb);
            AABBInternal<real, N, V, I - 1>::permuteMultiply(result, mat, v1, aabb);
        }
    };

    template<typename real, int N, typename V>
    struct AABBInternal<real, N, V, 0> {
        static void permuteMultiply(std::vector<V>& result, const glm::tmat4x4<real, glm::highp>& mat, const glm::tvec4<real, glm::highp>& v, const AABB<real, N, V>& aabb)
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
    inline void AABB<real, N, V>::Transform(const glm::tmat4x4<real, glm::highp>& mat)
    {
        std::vector<V> newCorners;
        AABBInternal<real, N, V, N - 1>::permuteMultiply(newCorners, mat, glm::tvec4<real, glm::highp>{1.0f}, *this);

        FromPoints(newCorners);
    }

    template<typename real, int N, typename V>
    inline AABB<real, N, V> AABB<real, N, V>::NewFromTransform(const glm::tmat4x4<real, glm::highp>& mat) const
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
        if (points.size() <= 0) return;

        minmax_[0] = glm::vec3(std::numeric_limits<float>::max());
        minmax_[1] = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& point : points) AddPoint(point);
    }

    template<typename real> using AABB2 = AABB<real, 2, glm::tvec2<real, glm::highp>>;
    template<typename real> using AABB3 = AABB<real, 3, glm::tvec3<real, glm::highp>>;
}
