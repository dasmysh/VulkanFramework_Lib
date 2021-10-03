/**
 * @file   LogicalDevice.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.10.20
 *
 * @brief  Declaration of the LogicalDevice class representing a Vulkan device.
 */

#pragma once

#include "main.h"
#include "core/concepts.h"
#include "gfx/vk/wrappers/Queue.h"
#include "gfx/vk/wrappers/CommandPool.h"
#include "gfx/vk/wrappers/Swapchain.h"

#include <glm/vec2.hpp>

namespace vkfw_core::gfx {
    class Texture2D;
}

namespace vkfw_core {
    class ShaderManager;
    using TextureManager = ResourceManager<gfx::Texture2D>;
}

namespace vkfw_core::cfg {
    class WindowCfg;
}

namespace vkfw_core::gfx {
    class GraphicsPipeline;
    class Framebuffer; // NOLINT
    class Buffer;      // NOLINT
    class Texture;
    class MemoryGroup;

    struct DeviceQueueDesc
    {
        DeviceQueueDesc() = default;
        DeviceQueueDesc(std::uint32_t familyIndex, std::vector<float> priorities) : m_familyIndex(familyIndex), m_priorities(std::move(priorities)) {}

        /** Holds the family index. */
        std::uint32_t m_familyIndex = 0;
        /** Holds the queues priorities. */
        std::vector<float> m_priorities;
    };

    class LogicalDevice final : public VulkanObjectWrapper<vk::UniqueDevice>
    {
    public:
        LogicalDevice(const cfg::WindowCfg& windowCfg, vk::PhysicalDevice phDevice,
                      std::vector<DeviceQueueDesc> queueDescs,
                      const std::vector<std::string>& requiredDeviceExtensions,
                      void* featuresNextChain, const Surface& surface = Surface{});
        LogicalDevice(const LogicalDevice&); // TODO: implement [10/30/2016 Sebastian Maisch]
        LogicalDevice(LogicalDevice&&) noexcept;
        LogicalDevice& operator=(const LogicalDevice&);
        LogicalDevice& operator=(LogicalDevice&&) noexcept;
        ~LogicalDevice();


        [[nodiscard]] vk::PhysicalDevice GetPhysicalDevice() const { return m_vkPhysicalDevice; }
        [[nodiscard]] const Queue& GetQueue(unsigned int familyIndex, unsigned int queueIndex) const
        {
            return m_queuesByRequestedFamily[familyIndex][queueIndex];
        }
        [[nodiscard]] const DeviceQueueDesc& GetQueueInfo(unsigned int familyIndex) const
        {
            return m_queueDescriptions[familyIndex];
        }
        [[nodiscard]] const CommandPool& GetCommandPool(unsigned int familyIndex) const
        {
            return *m_cmdPoolsByRequestedQFamily[familyIndex];
        }

        [[nodiscard]] CommandPool
        CreateCommandPoolForQueue(std::string_view name, unsigned int familyIndex,
                                  const vk::CommandPoolCreateFlags& flags = vk::CommandPoolCreateFlags()) const;
        std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const std::vector<std::string>& shaderNames,
            const glm::uvec2& size, unsigned int numBlendAttachments);

        [[nodiscard]] const cfg::WindowCfg& GetWindowCfg() const { return m_windowCfg; }
        [[nodiscard]] const vk::PhysicalDeviceProperties& GetDeviceProperties() const { return m_deviceProperties; }
        [[nodiscard]] const vk::PhysicalDeviceRayTracingPipelinePropertiesKHR&
        GetDeviceRayTracingPipelineProperties() const { assert(m_windowCfg.m_useRayTracing); return m_raytracingPipelineProperties; }
        [[nodiscard]] const vk::PhysicalDeviceAccelerationStructurePropertiesKHR&
        GetDeviceAccelerationStructureProperties() const { assert(m_windowCfg.m_useRayTracing); return m_accelerationStructureProperties; }
        [[nodiscard]] const vk::PhysicalDeviceFeatures& GetDeviceFeatures() const { return m_deviceFeatures; }
        [[nodiscard]] const vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& GetDeviceRayTracingPipelineFeatures() const { assert(m_windowCfg.m_useRayTracing); return m_raytracingPipelineFeatures; }
        [[nodiscard]] const vk::PhysicalDeviceAccelerationStructureFeaturesKHR& GetDeviceAccelerationStructureFeatures() const { assert(m_windowCfg.m_useRayTracing); return m_accelerationStructureFeatures; }
        [[nodiscard]] ShaderManager* GetShaderManager() const { return m_shaderManager.get(); }
        [[nodiscard]] TextureManager* GetTextureManager() const { return m_textureManager.get(); }
        [[nodiscard]] Texture2D* GetDummyTexture() const { return m_dummyTexture.get(); }

        [[nodiscard]] std::size_t CalculateUniformBufferAlignment(std::size_t size) const;
        [[nodiscard]] std::size_t CalculateStorageBufferAlignment(std::size_t size) const;
        [[nodiscard]] std::size_t CalculateASScratchBufferBufferAlignment(std::size_t size) const;
        [[nodiscard]] std::size_t CalculateSBTBufferAlignment(std::size_t size) const;
        [[nodiscard]] std::size_t CalculateSBTHandleAlignment(std::size_t size) const;
        [[nodiscard]] std::size_t CalculateBufferImageOffset(const Texture& second, std::size_t currentOffset) const;
        [[nodiscard]] std::size_t CalculateImageImageOffset(const Texture& first, const Texture& second,
                                                            std::size_t currentOffset) const;

        [[nodiscard]] std::pair<unsigned int, vk::Format>
        FindSupportedFormat(const std::vector<std::pair<unsigned int, vk::Format>>& candidates, vk::ImageTiling tiling,
                            const vk::FormatFeatureFlags& features) const;

    private:
        /** Holds the configuration of the window associated with this device. */
        const cfg::WindowCfg& m_windowCfg;
        /** Holds the physical device. */
        vk::PhysicalDevice m_vkPhysicalDevice;
        /** Holds the physical device limits. */
        vk::PhysicalDeviceLimits m_vkPhysicalDeviceLimits;
        /** Holds the queues by device queue family. */
        std::map<std::uint32_t, std::vector<vk::Queue>> m_vkQueuesByDeviceFamily;
        /** Holds a command pool for each device queue family. */
        std::map<std::uint32_t, CommandPool> m_vkCmdPoolsByDeviceQFamily;

        /** The properties of the device. */
        vk::PhysicalDeviceProperties m_deviceProperties;
        /** The ray tracing pipeline properties for the device. */
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_raytracingPipelineProperties;
        /** The acceleration structure properties for the device. */
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties;
        /** The features of the device. */
        vk::PhysicalDeviceFeatures m_deviceFeatures;
        /** The ray tracing pipeline features of the device. */
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR m_raytracingPipelineFeatures;
        /** The acceleration structure features of the device. */
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures;

        /** Holds the queue descriptions. */
        std::vector<DeviceQueueDesc> m_queueDescriptions;
        /** Holds the queues by requested queue family. */
        std::vector<std::vector<Queue>> m_queuesByRequestedFamily;
        /** Holds a command pool for each requested queue family. */
        std::vector<CommandPool*> m_cmdPoolsByRequestedQFamily;

        /** Holds the shader manager. */
        std::unique_ptr<ShaderManager> m_shaderManager;
        /** Holds the texture manager. */
        std::unique_ptr<TextureManager> m_textureManager;

        /** The memory group holding all dummy objects. */
        std::unique_ptr<MemoryGroup> m_dummyMemGroup;
        /** Holds the dummy texture. */
        std::shared_ptr<Texture2D> m_dummyTexture;

        bool m_singleQueueOnly = false;
    };
}
