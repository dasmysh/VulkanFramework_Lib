/**
 * @file   LogicalDevice.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.10.20
 *
 * @brief  Declaration of the LogicalDevice class representing a Vulkan device.
 */

#pragma once

#include "main.h"

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

    class LogicalDevice final
    {
    public:
        LogicalDevice(const cfg::WindowCfg& windowCfg, const vk::PhysicalDevice& phDevice,
                      std::vector<DeviceQueueDesc> queueDescs,
                      const std::vector<std::string>& requiredDeviceExtensions,
                      void* featuresNextChain,
                      const vk::SurfaceKHR& surface = vk::SurfaceKHR());
        LogicalDevice(const LogicalDevice&); // TODO: implement [10/30/2016 Sebastian Maisch]
        LogicalDevice(LogicalDevice&&) noexcept;
        LogicalDevice& operator=(const LogicalDevice&);
        LogicalDevice& operator=(LogicalDevice&&) noexcept;
        ~LogicalDevice();


        [[nodiscard]] const vk::PhysicalDevice& GetPhysicalDevice() const { return m_vkPhysicalDevice; }
        [[nodiscard]] const vk::Device& GetDevice() const { return *m_vkDevice; }
        [[nodiscard]] const vk::Queue& GetQueue(unsigned int familyIndex, unsigned int queueIndex) const
        {
            return m_vkQueuesByRequestedFamily[familyIndex][queueIndex];
        }
        [[nodiscard]] const DeviceQueueDesc& GetQueueInfo(unsigned int familyIndex) const
        {
            return m_queueDescriptions[familyIndex];
        }
        [[nodiscard]] const vk::CommandPool& GetCommandPool(unsigned int familyIndex) const
        {
            return m_vkCmdPoolsByRequestedQFamily[familyIndex];
        }

        [[nodiscard]] vk::UniqueCommandPool
        CreateCommandPoolForQueue(unsigned int familyIndex,
                                  const vk::CommandPoolCreateFlags& flags = vk::CommandPoolCreateFlags()) const;
        std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const std::vector<std::string>& shaderNames,
            const glm::uvec2& size, unsigned int numBlendAttachments);

        VkResult DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT* tagInfo) const;
        VkResult DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT* nameInfo) const;
        void CmdDebugMarkerBeginEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const;
        void CmdDebugMarkerEndEXT(VkCommandBuffer cmdBuffer) const;
        void CmdDebugMarkerInsertEXT(VkCommandBuffer cmdBuffer, VkDebugMarkerMarkerInfoEXT* markerInfo) const;

        [[nodiscard]] const cfg::WindowCfg& GetWindowCfg() const { return m_windowCfg; }
        [[nodiscard]] const vk::PhysicalDeviceProperties& GetDeviceProperties() const { return m_deviceProperties; }
        [[nodiscard]] const vk::PhysicalDeviceRayTracingPropertiesKHR& GetDeviceRayTracingProperties() const { assert(m_windowCfg.m_useRayTracing); return m_raytracingProperties; }
        [[nodiscard]] const vk::PhysicalDeviceFeatures& GetDeviceFeatures() const { return m_deviceFeatures; }
        [[nodiscard]] const vk::PhysicalDeviceRayTracingFeaturesKHR& GetDeviceRayTracingFeatures() const { assert(m_windowCfg.m_useRayTracing); return m_raytracingFeatures; }
        [[nodiscard]] ShaderManager* GetShaderManager() const { return m_shaderManager.get(); }
        [[nodiscard]] TextureManager* GetTextureManager() const { return m_textureManager.get(); }
        [[nodiscard]] Texture2D* GetDummyTexture() const { return m_dummyTexture.get(); }

        [[nodiscard]] std::size_t CalculateUniformBufferAlignment(std::size_t size) const;
        [[nodiscard]] std::size_t CalculateBufferImageOffset(const Texture& second, std::size_t currentOffset) const;
        [[nodiscard]] std::size_t CalculateImageImageOffset(const Texture& first, const Texture& second,
                                                            std::size_t currentOffset) const;

        [[nodiscard]] std::pair<unsigned int, vk::Format>
        FindSupportedFormat(const std::vector<std::pair<unsigned int, vk::Format>>& candidates, vk::ImageTiling tiling,
                            const vk::FormatFeatureFlags& features) const;

    private:
        [[nodiscard]] PFN_vkVoidFunction LoadVKDeviceFunction(const std::string& functionName,
                                                              const std::string& extensionName,
                                                              bool mandatory = false) const;

        /** Holds the configuration of the window associated with this device. */
        const cfg::WindowCfg& m_windowCfg;
        /** Holds the physical device. */
        vk::PhysicalDevice m_vkPhysicalDevice;
        /** Holds the physical device limits. */
        vk::PhysicalDeviceLimits m_vkPhysicalDeviceLimits;
        /** Holds the actual device. */
        vk::UniqueDevice m_vkDevice;
        /** Holds the queues by device queue family. */
        std::map<std::uint32_t, std::vector<vk::Queue>> m_vkQueuesByDeviceFamily;
        /** Holds a command pool for each device queue family. */
        std::map<std::uint32_t, vk::UniqueCommandPool> m_vkCmdPoolsByDeviceQFamily;

        /** The properties of the device. */
        vk::PhysicalDeviceProperties m_deviceProperties;
        /** The raytracing properties for the device. */
        vk::PhysicalDeviceRayTracingPropertiesKHR m_raytracingProperties;
        /** The features of the device. */
        vk::PhysicalDeviceFeatures m_deviceFeatures;
        /** The raytracing features of the device. */
        vk::PhysicalDeviceRayTracingFeaturesKHR m_raytracingFeatures;

        /** Holds the queue descriptions. */
        std::vector<DeviceQueueDesc> m_queueDescriptions;
        /** Holds the queues by requested queue family. */
        std::vector<std::vector<vk::Queue>> m_vkQueuesByRequestedFamily;
        /** Holds a command pool for each requested queue family. */
        std::vector<vk::CommandPool> m_vkCmdPoolsByRequestedQFamily;

        /** Holds whether debug markers are enabled. */
        bool m_enableDebugMarkers = false;

        // VK_EXT_debug_marker
        PFN_vkDebugMarkerSetObjectTagEXT fpDebugMarkerSetObjectTagEXT = nullptr;
        PFN_vkDebugMarkerSetObjectNameEXT fpDebugMarkerSetObjectNameEXT = nullptr;
        PFN_vkCmdDebugMarkerBeginEXT fpCmdDebugMarkerBeginEXT = nullptr;
        PFN_vkCmdDebugMarkerEndEXT fpCmdDebugMarkerEndEXT = nullptr;
        PFN_vkCmdDebugMarkerInsertEXT fpCmdDebugMarkerInsertEXT = nullptr;

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
