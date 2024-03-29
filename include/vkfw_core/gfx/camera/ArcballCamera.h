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

namespace vkfw_core::gfx {

    /**
    * Represents a camera rotating around the origin.
    */
    class ArcballCamera final : public UserControlledCamera
    {
    public:
        ArcballCamera(const glm::vec3& position, float fovY, float aspectRatio, float zNear, float zFar) noexcept;
        ArcballCamera(const ArcballCamera&) = default;
        ArcballCamera(ArcballCamera&&) = default;
        ArcballCamera& operator=(const ArcballCamera&) = delete;
        ArcballCamera& operator=(ArcballCamera&&) = delete;
        ~ArcballCamera() override;

        bool HandleMouse(int button, int action, float mouseWheelDelta, const VKWindow* sender) override;
        bool UpdateCamera(double elapsedTime, const VKWindow* sender) override;

    private:
        /** The initial camera position. */
        glm::vec3 m_initialCameraPosition;
        /** Holds the arc-ball used for camera rotation. */
        Arcball m_camArcball;
        /** The cameras field of view (y-direction). */
        float m_fovY;
        /** Near clipping plane. */
        float m_zNear;
        /** Far clipping plane. */
        float m_zFar;
        /** Has the camera changed this frame. */
        bool m_hasCameraChangedThisFrame = true;
    };
}
