/**
 * @file   ArcballCamera.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Declaration of an arc-ball camera.
 */

#pragma once

#include "gfx/camera/Arcball.h"
#include "gfx/camera/UserControlledCamera.h"

namespace vku::gfx {

    /**
    * Represents a camera rotating around the origin.
    */
    class ArcballCamera final : public UserControlledCamera
    {
    public:
        ArcballCamera(const glm::vec3& camPos, float fovY, float aspectRatio, float zNear, float zFar) noexcept;
        virtual ~ArcballCamera() override;

        virtual bool HandleMouse(int button, int action, float mouseWheelDelta, const VKWindow* sender) override;
        virtual void UpdateCamera(double elapsedTime, const VKWindow* sender) override;

    private:
        /** The initial camera position. */
        glm::vec3 initialCameraPosition_;
        /** Holds the arc-ball used for camera rotation. */
        Arcball camArcball_;
        /** The cameras field of view (y-direction). */
        float fovY_;
        /** Near clipping plane. */
        float zNear_;
        /** Far clipping plane. */
        float zFar_;
    };
}
