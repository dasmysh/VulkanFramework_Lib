/**
 * @file   TransformComponent.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Definition of the transform component of a scene object.
 */

#pragma once

#include "core/sceneobject/component/Component.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace vku {

    class TransformComponent : public Component<TransformComponent>
    {
    public:
        TransformComponent(const glm::vec3& pos = glm::vec3(0.0f),
            const glm::quat& rot = glm::quat(),
            const glm::vec3& scl = glm::vec3(1.0f));

        void SetPosition(const glm::vec3& pos) { translation_ = pos; UpdateMatrix(); }
        void SetRotation(const glm::quat& rot) { rotation_ = rot; UpdateMatrix(); }
        void SetScale(const glm::vec3& scl) { scale_ = scl; UpdateMatrix(); }

        const glm::vec3& GetPosition() const { return translation_; }
        const glm::quat& GetRotation() const { return rotation_; }
        const glm::vec3& GetScale() const { return scale_; }
        const glm::mat4& Matrix() const { return matrix_; }

        void Move(const glm::vec3& vec);
        void Rotate(const glm::quat& rot);
        void Rotate(const glm::vec3& euler);
        void Translate(const glm::vec3& vec);


    private:
        void UpdateMatrix();

        glm::mat4 matrix_;

        glm::quat rotation_;
        glm::vec3 translation_;
        glm::vec3 scale_;
    };

    inline TransformComponent::TransformComponent(const glm::vec3& trans,
        const glm::quat& rot, const glm::vec3& scl) :
        rotation_(rot), translation_(trans), scale_(scl)
    {
        UpdateMatrix();
    }

    inline void TransformComponent::UpdateMatrix()
    {

        matrix_ = glm::mat4_cast(rotation_);
        matrix_[0] *= scale_.x;
        matrix_[1] *= scale_.y;
        matrix_[2] *= scale_.z;
        matrix_[3] = glm::vec4(translation_, 1);
    }

    inline void TransformComponent::Move(const glm::vec3& vec)
    {
        translation_ += glm::mat3_cast(rotation_) * vec;
        UpdateMatrix();
    }

    inline void TransformComponent::Rotate(const glm::quat& rot)
    {
        rotation_ *= rot;
        UpdateMatrix();
    }

    inline void TransformComponent::Rotate(const glm::vec3& euler)
    {
        rotation_ = rotation_ * glm::quat(glm::vec3(euler.y, euler.x, euler.z));
        UpdateMatrix();
    }

    inline void TransformComponent::Translate(const glm::vec3& vec)
    {
        translation_ += vec;
        UpdateMatrix();
    }
}
