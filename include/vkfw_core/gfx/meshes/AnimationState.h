/**
 * @file   AnimationState.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2019.03.26
 *
 * @brief  Definition of a class containing the state of an animated object.
 */

#pragma once

#include <vector>
#include <glm/mat4x4.hpp>
#include "gfx/meshes/Animation.h"

namespace vkfw_core::gfx {

    class MeshInfo;
    class SceneMeshNode;

    /**
     *  Contains the current animation state of an animated object.
     */
    class AnimationState final
    {
    public:
        AnimationState(const MeshInfo* mesh, const SubAnimationMapping& mappings,
                       std::size_t startingAnimationIndex = 0, bool isRepeating = true);

        void Play(double currentTime);
        /**
         *  Pauses the animation.
         *  @param currentTime the current timestamp.
         */
        void Pause(double currentTime) { m_isPlaying = false; m_pauseTime = static_cast<float>(currentTime); }
        /** Stops the animation. */
        void Stop() { m_isPlaying = false; m_currentPlayTime = 0.0f; m_pauseTime = 0.0f; }

        /** Accessor for animation speed. */
        [[nodiscard]] float GetSpeed() const { return m_animationPlaybackSpeed.at(m_animationIndex); }
        /** Accessor for animation frames per second. */
        [[nodiscard]] float GetFramesPerSecond() const
        {
            return m_animations.at(m_animationIndex).GetFramesPerSecond();
        }
        /** Accessor for animation duration. */
        [[nodiscard]] float GetDuration() const { return m_animations.at(m_animationIndex).GetDuration(); }
        /** Accessor for animation time. */
        [[nodiscard]] float GetTime() const { return m_currentPlayTime; }

        /** Checks whether the animation is playing. */
        [[nodiscard]] bool IsPlaying() const { return m_isPlaying; }
        /** Checks whether the animation is repeating. */
        [[nodiscard]] bool IsRepeating() const { return m_isRepeating; }

        /** Updates the current time. */
        bool UpdateTime(double currentTime);
        /** Sets the current play time (internal animation time). */
        void SetCurrentPlayTime(float currentPlayTime) { m_currentPlayTime = currentPlayTime; }
        /** Sets the current relative frame (internal animation time relative to the duration). */
        void SetCurrentFrameRelative(float relativeFrame) { m_currentPlayTime = relativeFrame * GetDuration(); }
        void ComputeAnimationsFinalBonePoses();

        /**
         *  Sets the current animation speed.
         *  @param speed the new animation speed.
         */
        void SetSpeed(float speed) { m_animationPlaybackSpeed[m_animationIndex] = speed; }
        /**
         *  Sets whether the animation should be repeated.
         *  @param repeat whether the animation should be repeated.
         */
        void SetRepeat(bool repeat) { m_isRepeating = repeat; }

        /**
         *  Returns the current animation index.
         *  @return the current animation index.
         */
        [[nodiscard]] std::size_t GetCurrentAnimationIndex() const { return m_animationIndex; }
        /**
         *  Sets the current animation index.
         *  @param animationIndex the new current animation index.
         */
        void SetCurrentAnimationIndex(std::size_t animationIndex) { m_animationIndex = animationIndex; }

        /** Returns the global bone pose for a node. */
        [[nodiscard]] const glm::mat4& GetGlobalBonePose(std::size_t index) const { return m_globalBonePoses[index]; }
        /** Returns the local bone pose for a node. */
        [[nodiscard]] const glm::mat4& GetLocalBonePose(std::size_t index) const { return m_localBonePoses[index]; }
        /** Returns the skinning matrices. */
        [[nodiscard]] const std::vector<glm::mat4>& GetSkinningMatrices() const { return m_skinned; }

    private:
        void ComputeGlobalBonePose(const SceneMeshNode* node);

        /** Holds the mesh to render. */
        const MeshInfo* m_mesh;
        /** The current animation id. **/
        std::size_t m_animationIndex;

        /** Holds the playback speed for each AnimationState. */
        std::vector<float> m_animationPlaybackSpeed;
        /** Holds the actual animation for each AnimationState. */
        std::vector<Animation> m_animations;


        /** Is the animation playing. */
        bool m_isPlaying = false;
        /** Is the animation repeating. */
        bool m_isRepeating = true;

        /** The starting time of the animation. */
        float m_startTime = 0.0f;
        /** The starting time of the current pause of the animation. */
        float m_pauseTime = 0.0f;
        /** The starting playback time of the animation. */
        float m_currentPlayTime = 0.0f;

        /** The local bone poses. */
        std::vector<glm::mat4> m_localBonePoses;
        /** The global bone poses. */
        std::vector<glm::mat4> m_globalBonePoses;
        /** The skinning matrices used for rendering. */
        std::vector<glm::mat4> m_skinned;
    };

}
