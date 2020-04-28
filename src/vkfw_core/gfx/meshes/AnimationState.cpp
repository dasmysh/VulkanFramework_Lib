/**
 * @file   AnimationState.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2019.03.26
 *
 * @brief  Definition of a class containing the state of an animated object.
 */

#include "gfx/meshes/AnimationState.h"
#include "gfx/meshes/MeshInfo.h"
#include "gfx/meshes/SceneMeshNode.h"

namespace vkfw_core::gfx {

    /**
     *  Constructor of AnimationState.
     *  @param mesh the mesh containing animation data.
     *  @param mappings the animation mapping to use.
     *  @param startingAnimationIndex the starting index of the animation.
     *  @param isRepeating whether the animation should be repeating.
     */
    AnimationState::AnimationState(const MeshInfo* mesh, const SubAnimationMapping& mappings,
                                   std::size_t startingAnimationIndex, bool isRepeating)
        :
        m_mesh{ mesh },
        m_animationIndex{ startingAnimationIndex },
        m_isRepeating{ isRepeating }
    {
        assert(mappings.size() > m_animationIndex && "there is no mapping for the starting-state");

        m_skinned.resize(m_mesh->GetNumberOfBones());

        for (const auto& mapping : mappings) {
            m_animations.emplace_back(m_mesh->GetAnimations()[mapping.m_animationIndex].GetSubSequence(
                mapping.m_name, mapping.m_startTime, mapping.m_endTime));
            m_animationPlaybackSpeed.emplace_back(mapping.m_playbackSpeed);
        }

        m_localBonePoses.resize(m_mesh->GetNodes().size());
        m_globalBonePoses.resize(m_mesh->GetNodes().size());
        for (auto i = 0U; i < m_localBonePoses.size(); ++i) {
            m_localBonePoses[i] = m_mesh->GetNodes()[i]->GetLocalTransform();
        }

    }

    /**
     *  Starts animation playback (again).
     *  @param currentTime the current timestamp.
     */
    void AnimationState::Play(double currentTime)
    {
        m_isPlaying = true;
        if (m_pauseTime != 0.0f) {
            m_startTime += static_cast<float>(currentTime) - m_pauseTime;
        }
        else {
            m_startTime = static_cast<float>(currentTime);
        }
        m_pauseTime = 0.0f;
    }

    /**
     *  Updates the animation state.
     *  @param currentTime the current timestamp.
     */
    bool AnimationState::UpdateTime(double currentTime)
    {
        if (!m_isPlaying) { return false; }

        // Advance time
        m_currentPlayTime = (static_cast<float>(currentTime) - m_startTime) * GetFramesPerSecond() * GetSpeed();

        bool didAnimationStopOrRepeat = false;
        // Modulate time to fit within the interval of the animation
        if (m_currentPlayTime < 0.0f) {
            if (IsRepeating()) {
                auto time = m_currentPlayTime / GetDuration();
                m_currentPlayTime = (time - glm::floor(time)) * GetDuration();
            }
            else {
                m_currentPlayTime = 0.0f;
                Pause(currentTime);
            }
            didAnimationStopOrRepeat = true;
        } else if (m_currentPlayTime > GetDuration()) {

            if (IsRepeating()) {
                m_currentPlayTime = glm::mod(m_currentPlayTime, GetDuration());
            }
            else {
                m_currentPlayTime = GetDuration();
                Pause(currentTime);
            }
            didAnimationStopOrRepeat = true;
        }
        return didAnimationStopOrRepeat;
    }

    /**
     *  Computes the final bone poses for an animation.
     */
    void AnimationState::ComputeAnimationsFinalBonePoses()
    {
        const auto& currentAnimation = m_animations[m_animationIndex];
        const auto& invBindPoseMatrices = m_mesh->GetInverseBindPoseMatrices();

        for (auto i = 0U; i < m_mesh->GetNodes().size(); ++i) {
            glm::mat4 pose;
            if (currentAnimation.ComputePoseAtTime(i, m_currentPlayTime, pose)) { m_localBonePoses[i] = pose; }
        }

        ComputeGlobalBonePose(m_mesh->GetRootNode());

        for (const auto& node : m_mesh->GetNodes()) {
            if (node->GetBoneIndex() == -1) {
                continue;
            }
            m_skinned[static_cast<std::size_t>(node->GetBoneIndex())] = m_globalBonePoses[node->GetNodeIndex()] * invBindPoseMatrices[static_cast<std::size_t>(node->GetBoneIndex())];
        }
    }

    void AnimationState::ComputeGlobalBonePose(const SceneMeshNode* node)
    {
        auto nodeParent = node->GetParent();
        while (nodeParent != nullptr && node->GetBoneIndex() != -1 && (nodeParent->GetName().empty())) {
            nodeParent = nodeParent->GetParent();
        }

        if (nodeParent == nullptr) {
            m_globalBonePoses[node->GetNodeIndex()] = m_localBonePoses[node->GetNodeIndex()];
        }
        else {
            m_globalBonePoses[node->GetNodeIndex()] =
                m_globalBonePoses[nodeParent->GetNodeIndex()] * m_localBonePoses[node->GetNodeIndex()];
        }

        for (auto i = 0U; i < node->GetNumberOfNodes(); ++i) {
            ComputeGlobalBonePose(node->GetChild(i));
        }
    }
}
