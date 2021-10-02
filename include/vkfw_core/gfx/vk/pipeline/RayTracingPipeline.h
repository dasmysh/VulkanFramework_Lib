/**
 * @file   RayTracingPipeline.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.28
 *
 * @brief  Wrapper class for the vulkan ray tracing pipeline.
 */

#pragma once

#include "gfx/vk/wrappers/VulkanObjectWrapper.h"
#include "main.h"

namespace vkfw_core::gfx {

    class Shader;
    class HostBuffer;
    class PipelineLayout;

    class RayTracingPipeline : public VulkanObjectWrapper<vk::UniquePipeline>
    {
    public:
        RayTracingPipeline(const LogicalDevice* device, std::string_view name,
                           std::vector<std::shared_ptr<Shader>>&& shaders);
        ~RayTracingPipeline();

        void ResetShaders(std::vector<std::shared_ptr<Shader>>&& shaders);
        void CreatePipeline(std::uint32_t maxRecursionDepth, const PipelineLayout& pipelineLayout);
        const std::array<vk::StridedDeviceAddressRegionKHR, 4>& GetSBTDeviceAddresses() const { return m_sbtDeviceAddressRegions; }

    private:
        void InitializeShaderBindingTable();
        template<typename... Args> void AddShaderGroup(Args&&... args);
        static void ValidateShaderGroup(const vk::RayTracingShaderGroupCreateInfoKHR& shaderGroup);

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the shaders used in this pipeline. */
        std::vector<std::shared_ptr<Shader>> m_shaders;
        /** Holds the shader stages for pipeline creation. */
        std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
        /** Holds the shader groups used for raytracing. */
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        /** Holds contents of the 4 shader groups. */
        std::array<std::vector<std::uint32_t>, 4> m_shaderGroupIndexesByType;
        /** Offsets into the shader binding table for each group type. */
        std::array<vk::DeviceSize, 4> m_shaderGroupTypeOffset;
        /** Strided device address regions for each group type. */
        std::array<vk::StridedDeviceAddressRegionKHR, 4> m_sbtDeviceAddressRegions;

        /** Holds the shader binding table. */
        std::unique_ptr<vkfw_core::gfx::HostBuffer> m_shaderBindingTable;
    };

}
