/**
 * @file   CameraBase.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Implementation of the camera base class.
 */

#include "gfx/camera/CameraBase.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vkfw_core::gfx {

    /**
     *  Constructor. View matrix constructed from position and orientation.
     *  @param position the cameras initial position.
     *  @param orientation the cameras initial orientation.
     *  @param projMatrix the cameras initial projection matrix.
     */
    CameraBase::CameraBase(const glm::vec3& position, const glm::quat& orientation, const glm::mat4& projMatrix) noexcept :
        m_position{ position },
        m_orientation{ orientation },
        m_viewMatrix{ 1.0f },
        m_projMatrix{ projMatrix },
        m_viewFrustum{ m_projMatrix * m_viewMatrix }
    {
        m_projMatrix[1][1] *= -1.0f;
        UpdateView();
    }

    /**
     *  Constructor. Position and orientation constructed from view matrix.
     *  @param viewMatrix the cameras initial view matrix.
     *  @param projMatrix the cameras initial projection matrix.
     */
    CameraBase::CameraBase(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) noexcept :
        m_position{ 0.0f },
        m_orientation{ },
        m_viewMatrix{ viewMatrix },
        m_projMatrix{ projMatrix },
        m_viewFrustum{ m_projMatrix * m_viewMatrix }
    {
        m_projMatrix[1][1] *= -1.0f;
        UpdatePositionOrientation();
    }

    CameraBase::~CameraBase() = default;

    void CameraBase::SetOrientation(const glm::quat& orientation)
    {
        m_orientation = orientation;
        UpdateView();
    }

    void CameraBase::SetPosition(const glm::vec3& position)
    {
        m_position = position;
        UpdateView();
    }

    void CameraBase::SetPositionOrientation(const glm::vec3& position, const glm::quat& orientation)
    {
        m_position = position;
        m_orientation = orientation;
        UpdateView();
    }

    void CameraBase::SetViewMatrix(const glm::mat4& view)
    {
        m_viewMatrix = view;
        UpdatePositionOrientation();
    }

    void CameraBase::SetProjMatrix(const glm::mat4& proj)
    {
        m_projMatrix = proj;
        m_projMatrix[1][1] *= -1.0f;
        m_viewFrustum = math::Frustum(m_projMatrix * m_viewMatrix);
    }

    void CameraBase::UpdateView()
    {
        m_viewMatrix = glm::mat4_cast(glm::inverse(m_orientation));
        m_viewMatrix *= glm::translate(glm::mat4(1.0f), -m_position);

        m_viewFrustum = math::Frustum(m_projMatrix * m_viewMatrix);
    }

    void CameraBase::UpdatePositionOrientation()
    {
        auto viewInv = glm::inverse(m_viewMatrix);
        m_orientation = glm::quat_cast(viewInv);
        m_position = glm::vec3(viewInv[3]);

        m_viewFrustum = math::Frustum(m_projMatrix * m_viewMatrix);
    }

    void CameraBase::SetPositionOrientationProj(const glm::vec3& position, const glm::quat& orientation, const glm::mat4& proj)
    {
        m_position = position;
        m_orientation = orientation;
        m_projMatrix = proj;
        m_projMatrix[1][1] *= -1.0f;
        UpdateView();
    }

}
