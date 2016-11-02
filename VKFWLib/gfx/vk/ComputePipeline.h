/**
 * @file   ComputePipeline.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Declaration of a pipeline shader stage.
 */

#pragma once

#include "main.h"

namespace vku {
    namespace gfx {

        class Shader;

        class ComputePipeline final : public Resource
        {
        public:
            ComputePipeline(const std::string& shaderStageId, gfx::LogicalDevice* device);
            ComputePipeline(const ComputePipeline&);
            ComputePipeline& operator=(const ComputePipeline&);
            ComputePipeline(ComputePipeline&&) noexcept;
            ComputePipeline& operator=(ComputePipeline&&) noexcept;
            ~ComputePipeline();

        private:
            /** Holds all shaders in the program. */
            std::vector<std::shared_ptr<Shader>> shaders;
        };
    }
}

