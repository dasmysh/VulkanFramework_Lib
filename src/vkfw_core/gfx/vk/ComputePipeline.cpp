/**
 * @file   ComputePipeline.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.30
 *
 * @brief  Implementation of a vulkan compute pipeline object.
 */

#include "gfx/vk/ComputePipeline.h"
#include "gfx/vk/LogicalDevice.h"
#include "core/resources/ShaderManager.h"

namespace vku::gfx {

    ComputePipeline::ComputePipeline(const std::string& shaderStageId, gfx::LogicalDevice* device) :
        Resource(shaderStageId, device)
    {
        throw std::runtime_error("NOT YET IMPLEMENTED!");
    }


    ComputePipeline::~ComputePipeline()
    {
    }
}
