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
#include <cereal/types/map.hpp>
#include <cereal/types/complex.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "core/serialization_helper.h"

struct aiAnimation;

namespace vkfw_core::gfx {

    using Time = float;

    /**
     *  A channel representing one bone/ node etc.
     *  Holds positions, rotations and scaling of a bone/node etc. at a specific
     *  timestamp.
     */
    struct Channel
    {
        std::vector<std::pair<Time, glm::vec3>> m_positionFrames;
        std::vector<std::pair<Time, glm::quat>> m_rotationFrames;
        std::vector<std::pair<Time, glm::vec3>> m_scalingFrames;

        template <class Archive>
        void save(Archive& ar, const std::uint32_t) const  // NOLINT
        {
            ar(cereal::make_nvp("positionFrames", m_positionFrames),
                cereal::make_nvp("rotationFrames", m_rotationFrames),
                cereal::make_nvp("scalingFrames_", m_scalingFrames));
        }

        template <class Archive>
        void load(Archive& ar, const std::uint32_t)  // NOLINT
        {
            ar(cereal::make_nvp("positionFrames", m_positionFrames),
                cereal::make_nvp("rotationFrames", m_rotationFrames),
                cereal::make_nvp("scalingFrames_", m_scalingFrames));
        }
    };

    /**
     *  Information about the animation.
     */
    struct AnimationInfo
    {
        AnimationInfo() = default;

        /**
         *  Constructor taking start, end and playback time and an index if multiple animations are present.
         *  @param name name for the animation (used for introspection only).
         *  @param animationIndex animation index if multiple animations are present.
         *  @param start the start time of the animation.
         *  @param end the end time of the animation.
         *  @param playback the playback time (speed) of the animation.
         */
        AnimationInfo(std::string name, std::size_t animationIndex, float start, float end, float playback = 1.0f)
            : m_name{ std::move(name) }, m_animationIndex { animationIndex }, m_startTime{ start }, m_endTime{ end }, m_playbackSpeed{ playback }
        {
        }

        /**
         *  Constructor taking start, end and playback time.
         *  @param name name for the animation (used for introspection only).
         *  @param start the start time of the animation.
         *  @param end the end time of the animation.
         *  @param playback the playback time (speed) of the animation.
         */
        AnimationInfo(const std::string& name, float start, float end, float playback = 1.0f)
            : AnimationInfo{name, 0, start, end, playback }
        {
        }

        /**
         *  Constructor taking start, end and playback time and an index if multiple animations are present.
         *  @param animationIndex animation index if multiple animations are present.
         *  @param start the start time of the animation.
         *  @param end the end time of the animation.
         *  @param playback the playback time (speed) of the animation.
         */
        AnimationInfo(std::size_t animationIndex, float start, float end, float playback = 1.0f)
            : AnimationInfo{ "", animationIndex, start, end, playback }
        {
        }

        /**
         *  Constructor taking start, end and playback time.
         *  @param start the start time of the animation.
         *  @param end the end time of the animation.
         *  @param playback the playback time (speed) of the animation.
         */
        AnimationInfo(float start, float end, float playback = 1.0f)
            : AnimationInfo{ "", 0, start, end, playback }
        {
        }

        /** Holds the animations name. */
        std::string m_name;
        /** Index of the animation. */
        std::size_t m_animationIndex = 0;
        /** Start time of the animation. */
        float m_startTime = 0.0f;
        /** End time of the animation. */
        float m_endTime = 1.0f;
        /** Playback speed of the animation. */
        float m_playbackSpeed = 1.0f;
    };

    /**
     *  Mapping between AnimationState and AnimationInfo.
     */
    using SubAnimationMapping = std::vector<AnimationInfo>;

    /** An animation for a model. */
    class Animation
    {
    public:
        Animation();
        explicit Animation(aiAnimation* aiAnimation);

        void FlattenHierarchy(std::size_t numNodes, const std::map<std::string, std::size_t>& nodeNamesMap);

        /** Returns the number of ticks per second. */
        [[nodiscard]] float GetFramesPerSecond() const;
        /** Returns the duration of the animation in seconds. */
        [[nodiscard]] float GetDuration() const;
        /** Returns the animations name. */
        [[nodiscard]] const std::string& GetName() const { return m_name; }
        /**
         *  Returns the channel of one bone.
         *  @param id id of the bone.
         */
        // const Channel& GetChannel(std::size_t id) const;

        [[nodiscard]] Animation GetSubSequence(const std::string& name, Time start, Time end) const;

        bool ComputePoseAtTime(std::size_t id, Time time, glm::mat4& pose) const;

    private:
        /** Needed for serialization */
        friend class cereal::access;

        template <class Archive>
        void save(Archive& ar, const std::uint32_t) const  // NOLINT
        {
            ar(cereal::make_nvp("name", m_name), cereal::make_nvp("channels", m_channelMap),
               cereal::make_nvp("framesPerSecond", m_framesPerSecond),
                cereal::make_nvp("duration", m_duration));
        }

        template <class Archive>
        void load(Archive& ar, const std::uint32_t)  // NOLINT
        {
            ar(cereal::make_nvp("name", m_name), cereal::make_nvp("channels", m_channelMap),
               cereal::make_nvp("framesPerSecond", m_framesPerSecond),
                cereal::make_nvp("duration", m_duration));
        }

        /** Holds the animations name. */
        std::string m_name;
        /** Holds the channels during loading. */
        std::map<std::string, Channel> m_channelMap;
        /** Holds the channels (position, rotation, scaling) for each node. */
        std::vector<Channel> m_channels;
        /** Ticks per second. */
        float m_framesPerSecond = 0.f;
        /** Duration of this animation. */
        float m_duration = 0.f;
    };

    inline float Animation::GetFramesPerSecond() const { return m_framesPerSecond; }

    inline float Animation::GetDuration() const { return m_duration; }

    // inline const std::vector<Channel>& Animation::GetChannels() const { return channels_; }
    //
    // inline const Channel& Animation::GetChannel(std::size_t id) const { return channels_.at(id); }

}

// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::Channel, 2)
// NOLINTNEXTLINE
CEREAL_CLASS_VERSION(vkfw_core::gfx::Animation, 2)
