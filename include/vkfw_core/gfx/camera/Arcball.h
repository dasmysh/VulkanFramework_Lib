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

namespace vku {
    class VKWindow;
}

namespace vku::gfx {

    /**
     * Helper class for generic arc-balls.
     */
    class Arcball final
    {
    public:
        Arcball(int button) noexcept;

        bool HandleMouse(int button, int action, const vku::VKWindow* sender) noexcept;
        glm::quat GetWorldRotation(double elapsedTime, const glm::quat& camPosOrientation) noexcept;

    private:
        glm::vec3 MousePositionToArcball(const glm::vec2& mousePosition) const;

        /** Holds the button to use. */
        const int button_;
        /** Holds whether the arc-ball is currently rotated. */
        bool arcballOn_;
        /** holds the current arc-ball position in normalized device coordinates. */
        glm::vec3 currentScreen_;
        /** holds the last arc-ball position in normalized device coordinates. */
        glm::vec3 lastScreen_;

    };
}
