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

namespace vku::gfx {

    /**
     *  Constructor.
     *  @param theCamPos the cameras initial position.
     *  @param cameraHelper the camera helper class.
     *  @param speed the initial speed of camera movement.
     */
    FreeCamera::FreeCamera(const glm::vec3& position, const glm::quat& orientation,
        float fovY, float aspectRatio, float zNear, float zFar, double speed) noexcept :
        UserControlledCamera(position, orientation, glm::perspective(fovY, aspectRatio, zNear, zFar)),
        currentPY_{ 0.0f, 0.0f },
        currentMousePosition_{ 0.0f, 0.0f },
        moveSpeed_{speed},
        firstRun_{ true },
        fovY_{ fovY },
        zNear_{ zNear },
        zFar_{ zFar }
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
    void FreeCamera::UpdateCamera(double elapsedTime, const VKWindow* sender)
    {
        glm::vec3 camMove{ 0.0f };
        if (sender->IsKeyPressed(GLFW_KEY_W)) { camMove -= glm::vec3(0.0f, 0.0f, 1.0f); }
        if (sender->IsKeyPressed(GLFW_KEY_A)) { camMove -= glm::vec3(1.0f, 0.0f, 0.0f); }
        if (sender->IsKeyPressed(GLFW_KEY_S)) { camMove += glm::vec3(0.0f, 0.0f, 1.0f); }
        if (sender->IsKeyPressed(GLFW_KEY_D)) { camMove += glm::vec3(1.0f, 0.0f, 0.0f); }
        if (sender->IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) { camMove -= glm::vec3(0.0f, 1.0f, 0.0f); }
        if (sender->IsKeyPressed(GLFW_KEY_SPACE)) { camMove += glm::vec3(0.0f, 1.0f, 0.0f); }

        float moveLength = glm::length(camMove);
        if (moveLength > glm::epsilon<float>()) {
            camMove = (camMove / moveLength) * static_cast<float>(moveSpeed_ * elapsedTime);
        }
        auto camPos = GetPosition() + glm::inverse(GetOrientation()) * camMove;


        const double rotSpeed = 60.0;

        if (firstRun_) {
            currentMousePosition_ = sender->GetMousePositionNormalized();
            firstRun_ = false;
        }

        auto previousMousePosition = currentMousePosition_;
        currentMousePosition_ = sender->GetMousePositionNormalized();
        auto mouseDiff = currentMousePosition_ - previousMousePosition;

        auto pitch_delta = -static_cast<float>(mouseDiff.y * rotSpeed * elapsedTime);
        auto yaw_delta = static_cast<float>(mouseDiff.x * rotSpeed * elapsedTime);

        currentPY_ += glm::vec2(pitch_delta, yaw_delta);
        constexpr float ONE_MINUS_DELTA = 0.99f;
        currentPY_.x =
            glm::clamp(currentPY_.x, -glm::half_pi<float>() * ONE_MINUS_DELTA, glm::half_pi<float>() * ONE_MINUS_DELTA);
        auto newOrientation = glm::quat(glm::vec3(currentPY_.x, 0.0f, 0.0f)) * glm::quat(glm::vec3(0.0f, currentPY_.y, 0.0f));

        auto aspectRatio = static_cast<float>(sender->GetClientSize().x) / static_cast<float>(sender->GetClientSize().y);
        SetPositionOrientationProj(camPos, newOrientation, glm::perspective(fovY_, aspectRatio, zNear_, zFar_));
    }

    /**
     *  Set the speed of camera movement.
     *  @param speed new camera speed
     */
    void FreeCamera::SetMoveSpeed(double speed)
    {
        moveSpeed_ = speed;
    }

    /**
     *  Get the current speed of camera movement
     *  @return speed value
     */
    double FreeCamera::GetMoveSpeed()
    {
        return moveSpeed_;
    }

}
