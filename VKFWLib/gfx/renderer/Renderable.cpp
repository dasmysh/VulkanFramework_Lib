/**
 * @file   Renderable.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Renderable implementation.
 */

#include "Renderable.h"

namespace vku::gfx {

    Renderable::Renderable(bool has_indices)
        : has_indices_{ has_indices } {}

    Renderable::Renderable(const Renderable& rhs)
        : has_indices_{ rhs.has_indices_ } {}

    Renderable& Renderable::operator=(const Renderable& rhs)
    {
        if (this != &rhs) {
            this->~Renderable();
            has_indices_ = rhs.has_indices_;
        }
        return *this;
    }

    Renderable::Renderable(Renderable&& rhs) noexcept
        : has_indices_{ rhs.has_indices_ } {}

    Renderable& Renderable::operator=(Renderable&& rhs) noexcept
    {
        this->~Renderable();
        has_indices_ = rhs.has_indices_;
        return *this;
    }

    Renderable::~Renderable() = default;

    std::size_t Renderable::GetNumberOfNodes() const
    {
        return 1;
    }

    std::size_t Renderable::GetNumberOfMaterials() const
    {
        return 1;
    }

    std::size_t Renderable::GetNumberOfPartsInNode(std::size_t node_id) const
    {
        return 1;
    }

    glm::mat4 Renderable::GetLocalTransform(std::size_t node_id) const
    {
        return glm::mat4();
    }

}
