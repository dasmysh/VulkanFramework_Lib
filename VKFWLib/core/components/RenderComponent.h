/**
 * @file   RenderComponent.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  This class holds a pointer to the the renderable.
 */

#pragma once

#include "core/sceneobject/component/Component.h"
#include "gfx/renderer/renderable.h"

#include <memory>

namespace vku::gfx {
    class Renderable;
}

namespace vku {

    class RenderComponent : public Component<RenderComponent>
    {
    public:
        RenderComponent(std::shared_ptr<gfx::Renderable> renderable)
            : renderable_{ renderable } {}

        gfx::Renderable* GetRenderable() { return renderable_.get(); }

    private:
        /// The actual renderable used for rendering.
        std::shared_ptr<gfx::Renderable> renderable_;
    };
}
