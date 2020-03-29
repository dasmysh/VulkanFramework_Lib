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
        [[nodiscard]] const glm::mat4& GetViewMatrix() const noexcept { return viewMatrix_; }
        /** Returns the cameras projection matrix. */
        [[nodiscard]] const glm::mat4& GetProjMatrix() const noexcept { return projMatrix_; }
        /** Returns the cameras position. */
        [[nodiscard]] const glm::vec3& GetPosition() const noexcept { return position_; }
        /** Returns the cameras orientation. */
        [[nodiscard]] const glm::quat& GetOrientation() const noexcept { return orientation_; }
        /** Returns the camera view frustum. */
        [[nodiscard]] const math::Frustum<float>& GetViewFrustum() const noexcept { return viewFrustum_; }

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
        glm::vec3 position_;
        /** Holds the camera orientation. */
        glm::quat orientation_;
        /** Holds the camera view matrix. */
        glm::mat4 viewMatrix_;
        /** Holds the camera projection matrix. */
        glm::mat4 projMatrix_;
        /** The camera view frustum. */
        math::Frustum<float> viewFrustum_;
    };
}
