/**
 * @file   serialization_helper.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.02.17
 *
 * @brief  Definition of helper functions for serializing objects.
 */

#pragma once

#include "main.h"
#include "core/math/primitives.h"
#include <cereal/cereal.hpp>

namespace cereal {

    template<class Archive>
    void serialize(Archive & ar, glm::vec2& g)
    {
        ar(make_nvp("x", g.x), make_nvp("y", g.y));
    }

    template<class Archive>
    void serialize(Archive & ar, glm::vec3& g)
    {
        ar(make_nvp("x", g.x), make_nvp("y", g.y), make_nvp("z", g.y));
    }

    template<class Archive>
    void serialize(Archive & ar, glm::vec4& g)
    {
        ar(make_nvp("x", g.x), make_nvp("y", g.y), make_nvp("z", g.y), make_nvp("w", g.w));
    }

    template<class Archive>
    void serialize(Archive & ar, glm::quat& g)
    {
        ar(make_nvp("x", g.x), make_nvp("y", g.y), make_nvp("z", g.y), make_nvp("w", g.w));
    }

    template<class Archive>
    void serialize(Archive & ar, glm::mat4& m)
    {
        ar(make_nvp("0", m[0]), make_nvp("1", m[1]), make_nvp("2", m[2]), make_nvp("3", m[3]));
    }

    template<class Archive, typename T>
    void serialize(Archive & ar, vku::math::AABB3<T>& aabb)
    {
        ar(make_nvp("min", aabb.minmax[0]), make_nvp("max", aabb.minmax[1]));
    }
}

