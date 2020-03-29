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
        DeviceQueueDesc(std::uint32_t familyIndex, std::vector<float> priorities) : familyIndex_(familyIndex), priorities_(std::move(priorities)) {}

        /** Holds the family index. */
        std::uint32_t familyIndex_ = 0;
        /** Holds the queues priorities. */
        std::vector<float> priorities_;
    };

    class LogicalDevice final
    {
    public:
        LogicalDevice(const cfg::WindowCfg& windowCfg, const vk::PhysicalDevice& phDevice,
            std::vector<DeviceQueueDesc> queueDescs, const vk::SurfaceKHR& surface = vk::SurfaceKHR());
        LogicalDevice(const LogicalDevice&); // TODO: implement [10/30/2016 Sebastian Maisch]
        LogicalDevice(LogicalDevice&&) noexcept;
        LogicalDevice& operator=(const LogicalDevice&);
        LogicalDevice& operator=(LogicalDevice&&) noexcept;
        ~LogicalDevice();


        [[nodiscard]] const vk::PhysicalDevice& GetPhysicalDevice() const { return vkPhysicalDevice_; }
        [[nodiscard]] const vk::Device& GetDevice() const { return *vkDevice_; }
        [[nodiscard]] const vk::Queue& GetQueue(unsigned int familyIndex, unsigned int queueIndex) const
        {
            return vkQueuesByRequestedFamily_[familyIndex][queueIndex];
        }
        [[nodiscard]] const DeviceQueueDesc& GetQueueInfo(unsigned int familyIndex) const
        {
            return queueDescriptions_[familyIndex];
        }
        [[nodiscard]] const vk::CommandPool& GetCommandPool(unsigned int familyIndex) const
        {
            return vkCmdPoolsByRequestedQFamily_[familyIndex];
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

        [[nodiscard]] const cfg::WindowCfg& GetWindowCfg() const { return windowCfg_; }
        [[nodiscard]] ShaderManager* GetShaderManager() const { return shaderManager_.get(); }
        [[nodiscard]] TextureManager* GetTextureManager() const { return textureManager_.get(); }
        [[nodiscard]] Texture2D* GetDummyTexture() const { return dummyTexture_.get(); }

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
        const cfg::WindowCfg& windowCfg_;
        /** Holds the physical device. */
        vk::PhysicalDevice vkPhysicalDevice_;
        /** Holds the physical device limits. */
        vk::PhysicalDeviceLimits vkPhysicalDeviceLimits_;
        /** Holds the actual device. */
        vk::UniqueDevice vkDevice_;
        /** Holds the queues by device queue family. */
        std::map<std::uint32_t, std::vector<vk::Queue>> vkQueuesByDeviceFamily_;
        /** Holds a command pool for each device queue family. */
        std::map<std::uint32_t, vk::UniqueCommandPool> vkCmdPoolsByDeviceQFamily_;

        /** Holds the queue descriptions. */
        std::vector<DeviceQueueDesc> queueDescriptions_;
        /** Holds the queues by requested queue family. */
        std::vector<std::vector<vk::Queue>> vkQueuesByRequestedFamily_;
        /** Holds a command pool for each requested queue family. */
        std::vector<vk::CommandPool> vkCmdPoolsByRequestedQFamily_;

        /** Holds whether debug markers are enabled. */
        bool enableDebugMarkers_ = false;

        // VK_EXT_debug_marker
        PFN_vkDebugMarkerSetObjectTagEXT fpDebugMarkerSetObjectTagEXT = nullptr;
        PFN_vkDebugMarkerSetObjectNameEXT fpDebugMarkerSetObjectNameEXT = nullptr;
        PFN_vkCmdDebugMarkerBeginEXT fpCmdDebugMarkerBeginEXT = nullptr;
        PFN_vkCmdDebugMarkerEndEXT fpCmdDebugMarkerEndEXT = nullptr;
        PFN_vkCmdDebugMarkerInsertEXT fpCmdDebugMarkerInsertEXT = nullptr;

        /** Holds the shader manager. */
        std::unique_ptr<ShaderManager> shaderManager_;
        /** Holds the texture manager. */
        std::unique_ptr<TextureManager> textureManager_;

        /** The memory group holding all dummy objects. */
        std::unique_ptr<MemoryGroup> dummyMemGroup_;
        /** Holds the dummy texture. */
        std::shared_ptr<Texture2D> dummyTexture_;

        bool singleQueueOnly_ = false;
    };
}
