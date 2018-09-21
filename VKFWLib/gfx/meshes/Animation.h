/**
 * @file   Animation.h
 * @author Christian van Onzenoodt <christian.van-onzenoodt@uni-ulm.de>
 * @date   10.05.2017
 *
 * @brief  Data type for an animation.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include <cereal/cereal.hpp>
#include <cereal/types/complex.hpp>
#include <assimp/scene.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "core/serialization_helper.h"

namespace vku::gfx {

    using Time = float;

    /**
     *  A channel representing one bone/ node etc.
     *  Holds positions, rotations and scaling of a bone/node etc. at a specific
     *  timestamp.
     */
    struct Channel
    {
        std::vector<std::pair<Time, glm::vec3>> positionFrames_;
        std::vector<std::pair<Time, glm::quat>> rotationFrames_;
        std::vector<std::pair<Time, glm::vec3>> scalingFrames_;

        template <class Archive>
        void save(Archive& ar, const std::uint32_t version) const
        {
            ar(cereal::make_nvp("positionFrames", positionFrames_),
                cereal::make_nvp("rotationFrames", rotationFrames_),
                cereal::make_nvp("scalingFrames_", scalingFrames_));
        }

        template <class Archive>
        void load(Archive& ar, const std::uint32_t version)
        {
            ar(cereal::make_nvp("positionFrames", positionFrames_),
                cereal::make_nvp("rotationFrames", rotationFrames_),
                cereal::make_nvp("scalingFrames_", scalingFrames_));
        }
    };

    /** An animation for a model. */
    class Animation
    {
    public:
        Animation();
        Animation(aiAnimation* aiAnimation, const std::map<std::string, unsigned int>& boneNameToOffset);

        /** Returns the number of ticks per second. */
        float GetFramesPerSecond() const;
        /** Returns the duration of the animation in seconds. */
        float GetDuration() const;
        /** Returns the channels for each bone. */
        const std::vector<Channel>& GetChannels() const;
        /**
         *  Returns the channel of one bone.
         *  @param id id of the bone.
         */
        const Channel& GetChannel(std::size_t id) const;

        Animation GetSubSequence(Time start, Time end) const;

        glm::mat4 ComputePoseAtTime(std::size_t id, Time time) const;

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void save(Archive& ar, const std::uint32_t version) const
        {
            ar(cereal::make_nvp("channels", channels_),
                cereal::make_nvp("framesPerSecond", framesPerSecond_),
                cereal::make_nvp("duration", duration_));
        }

        template <class Archive>
        void load(Archive& ar, const std::uint32_t version)
        {
            ar(cereal::make_nvp("channels", channels_),
                cereal::make_nvp("framesPerSecond", framesPerSecond_),
                cereal::make_nvp("duration", duration_));
        }

        /// Holds the channels (position, rotation, scaling) for each bone.
        std::vector<Channel> channels_;
        /// Ticks per second.
        float framesPerSecond_ = 0.f;
        /// Duration of this animation.
        float duration_ = 0.f;
    };

    inline float Animation::GetFramesPerSecond() const { return framesPerSecond_; }

    inline float Animation::GetDuration() const { return duration_; }

    inline const std::vector<Channel>& Animation::GetChannels() const { return channels_; }

    inline const Channel& Animation::GetChannel(std::size_t id) const { return channels_.at(id); }

}

CEREAL_CLASS_VERSION(vku::gfx::Channel, 1)
CEREAL_CLASS_VERSION(vku::gfx::Animation, 1)
