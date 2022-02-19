/**
 * @file   FreeCamera.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Implementation of the free flight camera.
 */

#include "gfx/camera/FreeCamera.h"

#include "app/VKWindow.h"
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vkfw_core::gfx {

    /**
     *  Constructor.
     *  @param theCamPos the cameras initial position.
     *  @param cameraHelper the camera helper class.
     *  @param speed the initial speed of camera movement.
     */
    FreeCamera::FreeCamera(const glm::vec3& position, const glm::quat& orientation,
        float fovY, float aspectRatio, float zNear, float zFar, double speed) noexcept :
        UserControlledCamera(position, orientation, glm::perspective(fovY, aspectRatio, zNear, zFar)),
        m_currentPY{ 0.0f, 0.0f },
        m_currentMousePosition{ 0.0f, 0.0f },
        m_moveSpeed{speed},
        m_firstRun{ true },
        m_fovY{ fovY },
        m_zNear{ zNear },
        m_zFar{ zFar }
    {
    }

    bool FreeCamera::HandleMouse(int, int, float, const VKWindow*)
    {
        return false;
    }

    /**
     *  Updates the camera position and rotation.
     *  @param elapsedTime the time elapsed since the last frame.
     *  @param sender the application to retrieve key and mouse inputs from.
     */
    bool FreeCamera::UpdateCamera(double elapsedTime, const VKWindow* sender)
    {
        bool result = false;
        glm::vec3 camMove{ 0.0f };
        if (sender->IsKeyPressed(GLFW_KEY_W)) {
            camMove -= glm::vec3(0.0f, 0.0f, 1.0f);
            result = true;
        }
        if (sender->IsKeyPressed(GLFW_KEY_A)) {
            camMove -= glm::vec3(1.0f, 0.0f, 0.0f);
            result = true;
        }
        if (sender->IsKeyPressed(GLFW_KEY_S)) {
            camMove += glm::vec3(0.0f, 0.0f, 1.0f);
            result = true;
        }
        if (sender->IsKeyPressed(GLFW_KEY_D)) {
            camMove += glm::vec3(1.0f, 0.0f, 0.0f);
            result = true;
        }
        if (sender->IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            camMove -= glm::vec3(0.0f, 1.0f, 0.0f);
            result = true;
        }
        if (sender->IsKeyPressed(GLFW_KEY_SPACE)) {
            camMove += glm::vec3(0.0f, 1.0f, 0.0f);
            result = true;
        }

        float moveLength = glm::length(camMove);
        if (moveLength > glm::epsilon<float>()) {
            camMove = (camMove / moveLength) * static_cast<float>(m_moveSpeed * elapsedTime);
        }
        auto camPos = GetPosition() + glm::inverse(GetOrientation()) * camMove;


        const double rotSpeed = 60.0;

        if (m_firstRun) {
            m_currentMousePosition = sender->GetMousePositionNormalized();
            m_firstRun = false;
            result = true;
        }

        auto previousMousePosition = m_currentMousePosition;
        m_currentMousePosition = sender->GetMousePositionNormalized();
        auto mouseDiff = m_currentMousePosition - previousMousePosition;
        if (glm::length(mouseDiff) > 0.0001f) { result = true; }

        auto pitch_delta = -static_cast<float>(mouseDiff.y * rotSpeed * elapsedTime);
        auto yaw_delta = static_cast<float>(mouseDiff.x * rotSpeed * elapsedTime);

        m_currentPY += glm::vec2(pitch_delta, yaw_delta);
        constexpr float ONE_MINUS_DELTA = 0.99f;
        m_currentPY.x = glm::clamp(m_currentPY.x, -glm::half_pi<float>() * ONE_MINUS_DELTA,
                                   glm::half_pi<float>() * ONE_MINUS_DELTA);
        auto newOrientation =
            glm::quat(glm::vec3(m_currentPY.x, 0.0f, 0.0f)) * glm::quat(glm::vec3(0.0f, m_currentPY.y, 0.0f));

        auto aspectRatio = static_cast<float>(sender->GetClientSize().x) / static_cast<float>(sender->GetClientSize().y);
        SetPositionOrientationProj(camPos, newOrientation, glm::perspective(m_fovY, aspectRatio, m_zNear, m_zFar));
        return result;
    }

    /**
     *  Set the speed of camera movement.
     *  @param speed new camera speed
     */
    void FreeCamera::SetMoveSpeed(double speed)
    {
        m_moveSpeed = speed;
    }

    /**
     *  Get the current speed of camera movement
     *  @return speed value
     */
    double FreeCamera::GetMoveSpeed()
    {
        return m_moveSpeed;
    }

}
