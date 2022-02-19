/**
 * @file   CameraBase.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Declaration of the camera base class.
 */

#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include "core/math/primitives.h"

namespace vkfw_core::gfx {

    /**
     * Represents a base camera.
     */
    class CameraBase
    {
    public:
        CameraBase(const glm::vec3& position, const glm::quat& orientation, const glm::mat4& projMatrix) noexcept;
        CameraBase(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) noexcept;
        CameraBase(const CameraBase&) = default;
        CameraBase(CameraBase&&) = default;
        CameraBase& operator=(const CameraBase&) = default;
        CameraBase& operator=(CameraBase&&) = default;
        virtual ~CameraBase();

        /** Returns the cameras view matrix. */
        [[nodiscard]] const glm::mat4& GetViewMatrix() const noexcept { return m_viewMatrix; }
        /** Returns the cameras projection matrix. */
        [[nodiscard]] const glm::mat4& GetProjMatrix() const noexcept { return m_projMatrix; }
        /** Returns the cameras position. */
        [[nodiscard]] const glm::vec3& GetPosition() const noexcept { return m_position; }
        /** Returns the cameras orientation. */
        [[nodiscard]] const glm::quat& GetOrientation() const noexcept { return m_orientation; }
        /** Returns the camera view frustum. */
        [[nodiscard]] const math::Frustum<float>& GetViewFrustum() const noexcept { return m_viewFrustum; }

    protected:
        /**
         *  Sets the cameras orientation.
         *  @param orientation new camera orientation.
         */
        void SetOrientation(const glm::quat& orientation);
        /**
         *  Sets the cameras position.
         *  @param position new camera position.
         */
        void SetPosition(const glm::vec3& position);
        /**
         *  Sets the cameras position and orientation.
         *  @param position new camera position.
         *  @param orientation new camera orientation.
         */
        void SetPositionOrientation(const glm::vec3& position, const glm::quat& orientation);
        /**
         *  Sets the cameras position.
         *  @param view new camera view matrix.
         */
        void SetViewMatrix(const glm::mat4& view);
        /**
         *  Sets the cameras position.
         *  @param proj new camera projection matrix.
         */
        void SetProjMatrix(const glm::mat4& proj);
        /**
         *  Sets the cameras position and orientation.
         *  @param position new camera position.
         *  @param orientation new camera orientation.
         *  @param proj new camera projection matrix.
         */
        void SetPositionOrientationProj(const glm::vec3& position, const glm::quat& orientation, const glm::mat4& proj);

    private:
        void UpdateView();
        void UpdatePositionOrientation();

        /** Holds the camera position. */
        glm::vec3 m_position;
        /** Holds the camera orientation. */
        glm::quat m_orientation;
        /** Holds the camera view matrix. */
        glm::mat4 m_viewMatrix;
        /** Holds the camera projection matrix. */
        glm::mat4 m_projMatrix;
        /** The camera view frustum. */
        math::Frustum<float> m_viewFrustum;
    };
}
