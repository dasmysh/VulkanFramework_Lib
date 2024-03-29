/**
 * @file   RayTracingPipeline.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.28
 *
 * @brief  Wrapper class for the vulkan ray tracing pipeline.
 */

#pragma once

#include "gfx/vk/wrappers/VulkanObjectWrapper.h"
#include "gfx/vk/wrappers/PipelineBarriers.h"
#include "main.h"

namespace vkfw_core::gfx {

    class Shader;
    class HostBuffer;
    class PipelineLayout;

    class RayTracingPipeline : public VulkanObjectPrivateWrapper<vk::UniquePipeline>
    {
    public:
        struct RTShaderInfo
        {
            std::shared_ptr<Shader> shader;
            std::uint32_t shaderGroup;
        };

        RayTracingPipeline(const LogicalDevice* device, std::string_view name, std::vector<RTShaderInfo>&& shaders);
        ~RayTracingPipeline();

        void ResetShaders(std::vector<RTShaderInfo>&& shaders);
        void CreatePipeline(std::uint32_t maxRecursionDepth, const PipelineLayout& pipelineLayout);
        const std::array<vk::StridedDeviceAddressRegionKHR, 4>& GetSBTDeviceAddresses() const { return m_sbtDeviceAddressRegions; }
        void BindPipeline(CommandBuffer& cmdBuffer);

    private:
        void InitializeShaderBindingTable();
        static void ValidateShaderGroup(const vk::RayTracingShaderGroupCreateInfoKHR& shaderGroup);

        /** Holds the device. */
        const LogicalDevice* m_device;
        /** Holds the shaders used in this pipeline. */
        std::vector<RTShaderInfo> m_shaders;
        /** Holds the shader stages for pipeline creation. */
        std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
        /** Holds the shader groups used for raytracing. */
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        // /** Holds contents of the 4 shader groups. */
        std::array<std::vector<std::uint32_t>, 4> m_shaderGroupIndexesByType;
        /** Offsets into the shader binding table for each group type. */
        std::array<vk::DeviceSize, 4> m_shaderGroupTypeOffset;
        /** Size of a single entry in the shader binding table for each group type. */
        std::array<vk::DeviceSize, 4> m_shaderGroupTypeEntrySize;
        /** Strided device address regions for each group type. */
        std::array<vk::StridedDeviceAddressRegionKHR, 4> m_sbtDeviceAddressRegions;

        /** Holds the shader binding table. */
        std::unique_ptr<vkfw_core::gfx::HostBuffer> m_shaderBindingTable;
        /** Holds the shader binding table barrier. */
        PipelineBarrier m_barrier;
    };

}
