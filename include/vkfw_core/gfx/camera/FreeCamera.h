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

namespace vku::gfx {

    /**
    * Represents a free moving camera.
    */
    class FreeCamera final : public UserControlledCamera
    {
    public:
        FreeCamera(const glm::vec3& position, const glm::quat& orientation,
            float fovY, float aspectRatio, float zNear, float zFar, double speed = 30.0) noexcept;

        virtual bool HandleMouse(int button, int action, float mouseWheelDelta, const VKWindow* sender) override;
        virtual void UpdateCamera(double elapsedTime, const VKWindow* sender) override;
        void SetMoveSpeed(double speed);
        double GetMoveSpeed();

    private:
        /** Holds the current pitch and yaw state. */
        glm::vec2 currentPY_;
        /** Holds the current mouse position. */
        glm::vec2 currentMousePosition_;
        /** Holds the current movement speed */
        double moveSpeed_;
        /** Holds the flag for setting the previous mouse position. */
        bool firstRun_;
        /** The cameras field of view (y-direction). */
        float fovY_;
        /** Near clipping plane. */
        float zNear_;
        /** Far clipping plane. */
        float zFar_;
    };
}
