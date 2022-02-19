/**
 * @file   Arcball.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Implementation of the arc-ball class.
 */

#include "gfx/camera/Arcball.h"

#include "app/VKWindow.h"
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>

namespace vkfw_core::gfx {

    /**
     *  Constructor.
     *  @param theButton the mouse button to use.
     */
    Arcball::Arcball(int theButton) noexcept :
        m_button{ theButton },
        m_arcballOn{ false },
        m_currentScreen{ 0.0f },
        m_lastScreen{ 0.0f }
    {
    }

    /**
     *  Handles the mouse input to the arc-ball.
     *  @param theButton the mouse button the event belongs to.
     *  @param action the mouse buttons action.
     *  @param sender the application to supply normalized screen coordinates.
     */
    bool Arcball::HandleMouse(int theButton, int action, const vkfw_core::VKWindow* sender) noexcept
    {
        bool handled = false;
        if (m_button == theButton && action == GLFW_PRESS) {
            m_arcballOn = true;
            m_lastScreen = m_currentScreen = MousePositionToArcball(sender->GetMousePositionNormalized());
            handled = true;
        } else if (m_arcballOn && sender->IsMouseButtonPressed(m_button)) {
            m_currentScreen = MousePositionToArcball(sender->GetMousePositionNormalized());
            handled = true;
        } else if (!sender->IsMouseButtonPressed(m_button)) {
            handled = m_arcballOn;
            m_arcballOn = false;
        }

        return handled;
    }

    /**
     *  Calculates the world rotation using the cameras position and orientation.
     *  @param elapsedTime the time elapsed since the last frame.
     *  @param camPosOrientation the cameras position and orientation.
     */
    glm::quat Arcball::GetWorldRotation(double elapsedTime, const glm::quat& camPosOrientation) noexcept
    {
        const double speed = 60.0;

        glm::quat result(1.0f, 0.0f, 0.0f, 0.0f);
        if (m_currentScreen != m_lastScreen) {
            auto angle =
                static_cast<float>(speed * elapsedTime * acos(glm::min(1.0f, glm::dot(m_lastScreen, m_currentScreen))));
            auto camAxis = glm::cross(m_lastScreen, m_currentScreen);
            auto worldAxis = glm::normalize(glm::rotate(camPosOrientation, camAxis));
            result = glm::angleAxis(-2.0f * angle, worldAxis);
            m_lastScreen = m_currentScreen;
        }
        return result;
    }

    /**
     *  Calculates the mouse position on the arc-ball.
     *  @param mousePosition the mouse position in normalized device coordinates.
     */
    glm::vec3 Arcball::MousePositionToArcball(const glm::vec2 & mousePosition)
    {
        glm::vec3 result{ mousePosition, 0.0f };
        result = glm::clamp(result, glm::vec3(-1.0f), glm::vec3(1.0f));

        float length_squared = glm::dot(result, result);
        if (length_squared <= 1.0f) {
            result.z = sqrtf(1.0f - length_squared);
        } else {
            result = glm::normalize(result);
        }
        return result;
    }
}
