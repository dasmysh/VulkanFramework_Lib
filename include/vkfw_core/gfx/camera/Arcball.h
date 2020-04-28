/**
 * @file   Arcball.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Declaration of an arc-ball.
 */

#pragma once

#include <glm/vec2.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace vkfw_core {
    class VKWindow;
}

namespace vkfw_core::gfx {

    /**
     * Helper class for generic arc-balls.
     */
    class Arcball final
    {
    public:
        explicit Arcball(int button) noexcept;

        bool HandleMouse(int button, int action, const vkfw_core::VKWindow* sender) noexcept;
        glm::quat GetWorldRotation(double elapsedTime, const glm::quat& camPosOrientation) noexcept;

    private:
        [[nodiscard]] static glm::vec3 MousePositionToArcball(const glm::vec2& mousePosition);

        /** Holds the button to use. */
        const int m_button;
        /** Holds whether the arc-ball is currently rotated. */
        bool m_arcballOn;
        /** holds the current arc-ball position in normalized device coordinates. */
        glm::vec3 m_currentScreen;
        /** holds the last arc-ball position in normalized device coordinates. */
        glm::vec3 m_lastScreen;

    };
}
