/**
 * @file   UserControlledCamera.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Declaration of the camera base class.
 */

#pragma once

#include "gfx/camera/CameraBase.h"

namespace vku {
    class VKWindow;
}

namespace vku::gfx {

    /**
     * Represents a user controlled camera.
     */
    class UserControlledCamera : public CameraBase
    {
    public:
        UserControlledCamera(const glm::vec3& position, const glm::quat& orientation, const glm::mat4& projMatrix) noexcept :
            CameraBase{ position, orientation, projMatrix }
        {}
        UserControlledCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) noexcept :
            CameraBase{ viewMatrix, projMatrix }
        {}
        ~UserControlledCamera() override = default;

        virtual bool HandleMouse(int button, int action, float mouseWheelDelta, const VKWindow* sender) = 0;
        virtual void UpdateCamera(double elapsedTime, const VKWindow* sender) = 0;
    };
}
