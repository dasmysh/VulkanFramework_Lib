/**
 * @file   CameraBase.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Implementation of the camera base class.
 */

#include "CameraBase.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vku::gfx {

    /**
     *  Constructor. View matrix constructed from position and orientation.
     *  @param position the cameras initial position.
     *  @param orientation the cameras initial orientation.
     *  @param projMatrix the cameras initial projection matrix.
     */
    CameraBase::CameraBase(const glm::vec3& position, const glm::quat& orientation, const glm::mat4& projMatrix) noexcept :
        position_{ position },
        orientation_{ orientation },
        viewMatrix_{ 1.0f },
        projMatrix_{ projMatrix }
    {
        UpdateView();
        projMatrix_[1][1] *= -1.0f;
    }

    /**
     *  Constructor. Position and orientation constructed from view matrix.
     *  @param viewMatrix the cameras initial view matrix.
     *  @param projMatrix the cameras initial projection matrix.
     */
    CameraBase::CameraBase(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) noexcept :
        position_{ 0.0f },
        orientation_{ },
        viewMatrix_{ viewMatrix },
        projMatrix_{ projMatrix }
    {
        UpdatePositionOrientation();
        projMatrix_[1][1] *= -1.0f;
    }

    CameraBase::~CameraBase() = default;

    void CameraBase::SetOrientation(const glm::quat& orientation)
    {
        orientation_ = orientation;
        UpdateView();
    }

    void CameraBase::SetPosition(const glm::vec3& position)
    {
        position_ = position;
        UpdateView();
    }

    void CameraBase::SetPositionOrientation(const glm::vec3& position, const glm::quat& orientation)
    {
        position_ = position;
        orientation_ = orientation;
        UpdateView();
    }

    void CameraBase::SetViewMatrix(const glm::mat4& view)
    {
        viewMatrix_ = view;
        UpdatePositionOrientation();
    }

    void CameraBase::SetProjMatrix(const glm::mat4& proj)
    {
        projMatrix_ = proj;
        projMatrix_[1][1] *= -1.0f;
    }

    void CameraBase::UpdateView()
    {
        viewMatrix_ = glm::mat4_cast(orientation_);
        viewMatrix_ *= glm::translate(glm::mat4(1.0f), -position_);
    }

    void CameraBase::UpdatePositionOrientation()
    {
        auto viewInv = glm::inverse(viewMatrix_);
        orientation_ = glm::quat_cast(viewInv);
        position_ = glm::vec3(viewInv[3]);
    }

}
