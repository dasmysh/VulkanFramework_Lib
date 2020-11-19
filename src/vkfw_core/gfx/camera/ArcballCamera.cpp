/**
 * @file   ArcballCamera.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Implementation of an arc-ball camera.
 */

#include "gfx/camera/ArcballCamera.h"

#include "app/VKWindow.h"
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vkfw_core::gfx {

    /**
     *  Constructor.
     *  @param theCamPos the cameras initial position.
     *  @param cameraHelper the camera helper class.
     */
    ArcballCamera::ArcballCamera(const glm::vec3& position, float fovY, float aspectRatio, float zNear, float zFar) noexcept :
        UserControlledCamera{ glm::lookAt(position, glm::vec3{ 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }), glm::perspective(fovY, aspectRatio, zNear, zFar) },
        m_initialCameraPosition{ glm::normalize(position) },
        m_camArcball{ GLFW_MOUSE_BUTTON_1 },
        m_fovY{ fovY },
        m_zNear{ zNear },
        m_zFar{ zFar }
    {
    }

    ArcballCamera::~ArcballCamera() = default;

    /**
     *  Updates the camera parameters using the internal arc-ball.
     */
    void ArcballCamera::UpdateCamera(double elapsedTime, const VKWindow* sender)
    {
        const double mouseWheelSpeed = 8.0;
        float radius = glm::length(GetPosition());
        if (sender->IsKeyPressed(GLFW_KEY_W)) { radius -= static_cast<float>(mouseWheelSpeed * elapsedTime); }
        if (sender->IsKeyPressed(GLFW_KEY_S)) { radius += static_cast<float>(mouseWheelSpeed * elapsedTime); }

        radius = glm::max(radius, 0.0f);

        auto camOrient = GetOrientation();
        glm::quat camOrientStep = m_camArcball.GetWorldRotation(elapsedTime, camOrient);
        camOrient = camOrientStep * camOrient;
        glm::mat3 matOrient{ glm::mat3_cast(camOrient) };
        auto camPos = radius * (matOrient * glm::vec3(0.0f, 0.0f, 1.0f));

        auto aspectRatio = static_cast<float>(sender->GetClientSize().x) / static_cast<float>(sender->GetClientSize().y);
        SetPositionOrientationProj(camPos, camOrient,
                                   glm::perspective(m_fovY, aspectRatio, m_zNear, m_zFar));
    }

    /**
     *  Handles the mouse events for the camera.
     *  @param button the mouse button the event belongs to.
     *  @param action the mouse buttons action.
     *  @param mouseWheelDelta the change in the mouse-wheel rotation.
     *  @param sender the application to supply normalized screen coordinates.
     */
    bool ArcballCamera::HandleMouse(int button, int action, float mouseWheelDelta, const VKWindow* sender)
    {
        bool handled = m_camArcball.HandleMouse(button, action, sender);

        if (mouseWheelDelta != 0) {
            if (sender->IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
                SetPosition(GetPosition() - 0.01f * mouseWheelDelta * glm::normalize(GetPosition()));
            } else {
                constexpr float ARCBALL_ANGLE_SCALE = 0.03f;
                constexpr float ARCBALL_MAX_ANGLE = 80.0f;
                auto fov = m_fovY - mouseWheelDelta * glm::radians(ARCBALL_ANGLE_SCALE);
                m_fovY = glm::clamp(fov, glm::radians(1.0f), glm::radians(ARCBALL_MAX_ANGLE));
            }
            handled = true;
        }

        return handled;
    }

}
