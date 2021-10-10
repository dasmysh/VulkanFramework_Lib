/**
 * @file   FreeCamera.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Declaration of a free flight camera.
 */

#pragma once

#include "gfx/camera/UserControlledCamera.h"
#include <glm/gtc/quaternion.hpp>

namespace vkfw_core::gfx {

    /**
    * Represents a free moving camera.
    */
    class FreeCamera final : public UserControlledCamera
    {
    public:
        static constexpr double DEFAULT_CAMERA_SPEED = 30.0;
        FreeCamera(const glm::vec3& position, const glm::quat& orientation, float fovY, float aspectRatio, float zNear,
                   float zFar, double speed = DEFAULT_CAMERA_SPEED) noexcept;

        bool HandleMouse(int button, int action, float mouseWheelDelta, const VKWindow* sender) override;
        bool UpdateCamera(double elapsedTime, const VKWindow* sender) override;
        void SetMoveSpeed(double speed);
        double GetMoveSpeed();

    private:
        /** Holds the current pitch and yaw state. */
        glm::vec2 m_currentPY;
        /** Holds the current mouse position. */
        glm::vec2 m_currentMousePosition;
        /** Holds the current movement speed */
        double m_moveSpeed;
        /** Holds the flag for setting the previous mouse position. */
        bool m_firstRun;
        /** The cameras field of view (y-direction). */
        float m_fovY;
        /** Near clipping plane. */
        float m_zNear;
        /** Far clipping plane. */
        float m_zFar;
    };
}
