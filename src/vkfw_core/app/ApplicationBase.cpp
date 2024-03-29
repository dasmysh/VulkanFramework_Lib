/**
 * @file   ApplicationBase.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Implements the application base class.
 */

#undef VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL
// NOLINTNEXTLINE
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 1

#pragma warning(push, 3)
#include <Windows.h>
#pragma warning(pop)

#include "app/ApplicationBase.h"
#include "app/VKWindow.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>
#include <set>
#include <string_view>
#include <cereal/archives/xml.hpp>
#include <glm/common.hpp>

#include "gfx/vk/LogicalDevice.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vkfw_core {

    /**
     * Logs the debug output of Vulkan.
     * @param flags the logs severity flags
     * @param objType the actual type of obj
     * @param obj the object that threw the message
     * @param location
     * @param code
     * @param layerPrefix
     * @param msg the debug message
     * @param userData the user supplied data
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugOutputCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                              void*) {
        using namespace std::literals;
        if ("Loader Message"sv == pCallbackData->pMessageIdName) return VK_FALSE;

        auto vkLogLevel = spdlog::level::level_enum::trace;
        if ((messageSeverity & static_cast<unsigned int>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) != 0) {
            vkLogLevel = spdlog::level::level_enum::debug;
        } else if ((messageSeverity & static_cast<unsigned int>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) != 0) {
            vkLogLevel = spdlog::level::level_enum::info;
        } else if ((messageSeverity & static_cast<unsigned int>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
                   != 0) {
            vkLogLevel = spdlog::level::level_enum::warn;
        } else if ((messageSeverity & static_cast<unsigned int>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) != 0) {
            vkLogLevel = spdlog::level::level_enum::err;
        }

        std::string_view messageTypeString;
        if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) {
            messageTypeString = "Performance";
        } else if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) {
            messageTypeString = "Validation";
        } else {
            messageTypeString = "General";
        }

        std::string objectsString = pCallbackData->objectCount == 0 ? "" : "\nVulkan Objects:";
        for (std::uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
            objectsString = fmt::format("{}\n  {}: 0x{:0<#16x} ()", objectsString,
                                        vk::to_string(vk::ObjectType(pCallbackData->pObjects[i].objectType)),
                                        pCallbackData->pObjects[i].objectHandle, pCallbackData->pObjects[i].pObjectName);
        }

        std::string queueLabelString = pCallbackData->queueLabelCount == 0 ? "" : "\nVulkan Queue Labels:";
        for (std::uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i) {
            queueLabelString = fmt::format("{}\n  {}", queueLabelString, pCallbackData->pQueueLabels[i].pLabelName);
        }

        spdlog::log(vkLogLevel, " [{}]: {}{}{}", messageTypeString, pCallbackData->pMessage, objectsString, queueLabelString);

        return VK_FALSE;
    }


    ApplicationBase::GLFWInitObject::GLFWInitObject() {
        glfwInit();
    }

    ApplicationBase::GLFWInitObject::~GLFWInitObject()
    {
        glfwTerminate();
    }
}

namespace vkfw_core::qf {

    int findQueueFamily(const vk::PhysicalDevice& device, const cfg::QueueCfg& desc, const vk::SurfaceKHR& surface = vk::SurfaceKHR())
    {
        vk::QueueFlags reqFlags;
        if (!(desc.m_graphics || desc.m_compute) && desc.m_transfer) { reqFlags |= vk::QueueFlagBits::eTransfer; }
        if (desc.m_graphics) { reqFlags |= vk::QueueFlagBits::eGraphics; }
        if (desc.m_compute) { reqFlags |= vk::QueueFlagBits::eCompute; }
        if (desc.m_sparseBinding) { reqFlags |= vk::QueueFlagBits::eSparseBinding; }

        auto queueProps = device.getQueueFamilyProperties();
        auto queueCount = static_cast<std::uint32_t>(queueProps.size());
        for (std::uint32_t i = 0; i < queueCount; i++) {
            if (queueProps[i].queueCount < desc.m_priorities.size()) { continue; }

            bool flagsFit = false;
            if (reqFlags == vk::QueueFlagBits::eTransfer
                && (queueProps[i].queueFlags == vk::QueueFlagBits::eTransfer
                    || queueProps[i].queueFlags == (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding))) {
                flagsFit = true;
            }
            if (reqFlags == vk::QueueFlagBits::eSparseBinding
                && (queueProps[i].queueFlags == vk::QueueFlagBits::eSparseBinding
                    || queueProps[i].queueFlags == (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding))) {
                flagsFit = true;
            }
            if (reqFlags == (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding)
                && queueProps[i].queueFlags == (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding)) {
                flagsFit = true;
            }
            if ((reqFlags != vk::QueueFlagBits::eTransfer && reqFlags != vk::QueueFlagBits::eSparseBinding
                 && reqFlags != (vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eSparseBinding))
                && queueProps[i].queueFlags & reqFlags) {
                flagsFit = true;
            }


            if (flagsFit) {
                if (surface && desc.m_graphics && (device.getSurfaceSupportKHR(i, surface) == 0U)) {
                    continue;
                }
                return i;
            }
            /*if (queueProps[i].queueFlags & reqFlags) {
                if (surface && desc.m_graphics && !device.getSurfaceSupportKHR(i, surface)) {
                    continue;
                }
                return i;
            }*/
        }
        return -1;
    }
}


namespace vkfw_core::cfg {

    std::vector<vk::SurfaceFormatKHR> GetVulkanSurfaceFormatsFromConfig(const WindowCfg& cfg)
    {
        std::vector<vk::SurfaceFormatKHR> result;
        vk::SurfaceFormatKHR fmt;
        if (cfg.m_backbufferBits == 32) {
            if (cfg.m_useSRGB) {
                fmt.format = vk::Format::eR8G8B8A8Srgb;
                result.push_back(fmt);
                fmt.format = vk::Format::eB8G8R8A8Srgb;
                result.push_back(fmt);
            }
            else {
                fmt.format = vk::Format::eR8G8B8A8Unorm;
                result.push_back(fmt);
                fmt.format = vk::Format::eB8G8R8A8Unorm;
                result.push_back(fmt);
            }
        }

        constexpr std::size_t BITS_RGB8 = 24;
        if (cfg.m_backbufferBits == BITS_RGB8) {
            if (cfg.m_useSRGB) {
                fmt.format = vk::Format::eR8G8B8Srgb;
                result.push_back(fmt);
                fmt.format = vk::Format::eB8G8R8Srgb;
                result.push_back(fmt);
            }
            else {
                fmt.format = vk::Format::eR8G8B8Unorm;
                result.push_back(fmt);
                fmt.format = vk::Format::eB8G8R8Unorm;
                result.push_back(fmt);
            }
        }

        if (cfg.m_backbufferBits == 16) {
            if (!cfg.m_useSRGB) {
                fmt.format = vk::Format::eR5G6B5UnormPack16;
                result.push_back(fmt);
                fmt.format = vk::Format::eR5G5B5A1UnormPack16;
                result.push_back(fmt);
                fmt.format = vk::Format::eB5G6R5UnormPack16;
                result.push_back(fmt);
                fmt.format = vk::Format::eB5G5R5A1UnormPack16;
                result.push_back(fmt);
            }
        }

        return result;
    }

    vk::PresentModeKHR GetVulkanPresentModeFromConfig(const WindowCfg& cfg)
    {
        vk::PresentModeKHR presentMode = {};
        if (cfg.m_swapOptions == cfg::SwapOptions::DOUBLE_BUFFERING) { presentMode = vk::PresentModeKHR::eImmediate; }
        if (cfg.m_swapOptions == cfg::SwapOptions::DOUBLE_BUFFERING_VSYNC) { presentMode = vk::PresentModeKHR::eFifo; }
        if (cfg.m_swapOptions == cfg::SwapOptions::TRIPLE_BUFFERING) { presentMode = vk::PresentModeKHR::eMailbox; }
        return presentMode;
    }

    std::uint32_t GetVulkanAdditionalImageCountFromConfig(const WindowCfg& cfg)
    {
        if (cfg.m_swapOptions == SwapOptions::TRIPLE_BUFFERING) { return 1; }
        return 0;
    }
}

namespace vkfw_core {

    ApplicationBase* ApplicationBase::m_instance = nullptr;


    /**
     * Construct a new application.
     * @param applicationName the applications name.
     * @param applicationVersion the applications version.
     * @param configFileName the configuration file to use.
     */
    ApplicationBase::ApplicationBase(const std::string_view& applicationName, std::uint32_t applicationVersion,
                                     const std::string_view& configFileName,
                                     const std::vector<std::string>& requiredInstanceExtensions,
                                     const std::vector<std::string>& requiredDeviceExtensions,
                                     void* deviceFeaturesNextChain)
        :
        m_configFileName{ configFileName },
        m_pause(true),
        m_stopped(false),
        m_currentTime(0.0),
        m_elapsedTime(0.0)
    {
        spdlog::debug("Trying to load configuration.");
        {
            std::ifstream configFile(configFileName.data(), std::ios::in);
            if (configFile.is_open()) {
                auto ia = std::make_unique<cereal::XMLInputArchive>(configFile);
                (*ia) >> cereal::make_nvp("configuration", m_config);
            } else {
                spdlog::debug("Configuration file not found. Using standard config.");
            }
        }

        {
            // always directly write configuration to update version.
            std::ofstream ofs(configFileName.data(), std::ios::out);
            auto oa = std::make_unique<cereal::XMLOutputArchive>(ofs);
            (*oa) << cereal::make_nvp("configuration", m_config);
        }

        bool hasRayTracing = false;
        for (auto& wc : m_config.m_windows) {
            if (wc.m_useRayTracing) hasRayTracing = true;
        }

        InitVulkan(applicationName, applicationVersion, requiredInstanceExtensions);
        m_instance = this;

        // TODO: Check if the GUI works with multiple windows. [10/19/2018 Sebastian Maisch]
        bool first = true;
        for (auto& wc : m_config.m_windows) {
            m_windows.emplace_back(wc, first, requiredDeviceExtensions, deviceFeaturesNextChain);
            m_windows.back().ShowWindow();
            first = false;
        }
    }

    ApplicationBase::~ApplicationBase() noexcept
    {
        m_windows.clear();
        m_vkDebugUtilsMessenger.reset();
        m_vkInstance.reset();

        spdlog::debug("Exiting application. Saving configuration to file.");
        try {
            std::ofstream ofs(m_configFileName, std::ios::out);
            // NOLINTNEXTLINE
            auto oa = std::make_unique<cereal::XMLOutputArchive>(ofs);
            (*oa) << cereal::make_nvp("configuration", m_config);
        } catch (...) {
            spdlog::critical("Could not write configuration. Unknown exception.");
        }
    }

    VKWindow* ApplicationBase::GetFocusedWindow()
    {
        VKWindow* focusWindow = nullptr;
        for (auto& w : m_windows) {
            if (w.IsFocused()) { focusWindow = &w; }
        }
        return focusWindow;
    }

    VKWindow* ApplicationBase::GetWindow(unsigned int idx) { return &m_windows[idx]; }

    void ApplicationBase::SetPause(bool pause)
    {
        if (pause) {
            spdlog::info("Begin pause");
        } else {
            spdlog::info("End pause");
        }
        m_pause = pause;
    }

    /**
     * Handles all keyboard input.
     * @param key the key pressed
     * @param scancode the pressed keys scancode
     * @param action the action of the key event
     * @param mods the key modificators
     * @param sender the window that send the keyboard messages
     */
    bool ApplicationBase::HandleKeyboard(int key, int, int action, int mods, VKWindow* sender)
    {
        auto handled = false;
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                if ((static_cast<unsigned int>(mods) & static_cast<unsigned int>(GLFW_MOD_CONTROL)) != 0) {
                    m_stopped = true;
                } else {
                    sender->CloseWindow();
                }
                handled = true;
                break;
            case GLFW_KEY_F2:
                m_guiMode = !m_guiMode;
                handled = true;
                break;
            case GLFW_KEY_F9:
                // TODO: recompile shaders [10/24/2016 Sebastian Maisch]
                // programManager_->RecompileAll();
                handled = true;
                break;
            default: break;
            }
        }

        // TODO if (!handled && IsRunning() && !IsPaused()) handled = cameraView_->HandleKeyboard(key, scancode, action, mods, sender);

        return handled;
    }

    /**
     * Handles mouse input.
     * @param button the mouse button to be handled
     * @param action the button action
     * @param mods the modificator keys pressed when button action was triggered
     * @param mouseWheelDelta the scroll wheel delta
     * @param sender the window that send the keyboard messages
     * @return whether the message was handled
     */
    bool ApplicationBase::HandleMouse(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender)
    {
        auto handled = false;
        if (IsRunning() && !IsPaused()) { handled = HandleMouseApp(button, action, mods, mouseWheelDelta, sender); }
        // TODO if (!handled && IsRunning() && !IsPaused()) handled = cameraView_->HandleMouse(button, action, mods, mouseWheelDelta, sender);
        return handled;
    }

    /**
     * Handles resize events.
     */
    void ApplicationBase::OnResize(unsigned int width, unsigned int height, VKWindow* window)
    {
        glm::uvec2 screenSize(width, height);
        Resize(screenSize, window);
    }

    void ApplicationBase::Resize(const glm::uvec2&, VKWindow*)
    {
    }

    void ApplicationBase::StartRun()
    {
        m_stopped = false;
        m_pause = false;
        m_currentTime = glfwGetTime();
    }

    bool ApplicationBase::IsRunning() const
    {
        return !m_stopped && !m_windows[0].IsClosing();
    }

    void ApplicationBase::EndRun()
    {
        m_stopped = true;
    }

    void ApplicationBase::Step()
    {
        if (m_stopped) {
            constexpr int HALF_SECOND = 500;
            Sleep(HALF_SECOND);
            return;
        }

        auto currentTime = glfwGetTime();
        m_elapsedTime = currentTime - m_currentTime;
        m_currentTime = currentTime;
        glfwPollEvents();

        for (auto& window : m_windows) {
            window.PrepareFrame();

            if (!this->m_pause && (!m_config.m_pauseOnKillFocus || (GetFocusedWindow() != nullptr))) {
                FrameMove(static_cast<float>(m_currentTime), static_cast<float>(m_elapsedTime), &window);
            }

            RenderScene(&window);
            if (IsGUIMode()) { RenderGUI(&window); }
            window.DrawCurrentCommandBuffer();
            window.SubmitFrame();
        }
    }

    void ApplicationBase::CheckVKInstanceExtensions(const std::vector<const char*>& enabledExtensions)
    {
        auto extensions = vk::enumerateInstanceExtensionProperties();
        if constexpr (verbose_feature_logging) {
            spdlog::info("VK Instance Extensions:");
            for (const auto& extension : extensions) {
                spdlog::info("- {}[SpecVersion: {}]", extension.extensionName, extension.specVersion);
            }
        }

        for (const auto& enabledExt : enabledExtensions) {
            auto found = std::find_if(extensions.begin(), extensions.end(),
                                      [&enabledExt](const vk::ExtensionProperties& extProps) { return std::strcmp(enabledExt, &extProps.extensionName[0]) == 0; });
            if (found == extensions.end()) {
                spdlog::critical("Extension needed ({}) is not available. Quitting.", enabledExt);
                throw std::runtime_error("Vulkan extension missing.");
            }
        }
    }

    void ApplicationBase::CheckVKInstanceLayers()
    {
        auto layers = vk::enumerateInstanceLayerProperties();
        if constexpr (verbose_feature_logging) {
            spdlog::info("VK Instance Layers:");
            for (const auto& layer : layers) {
                spdlog::info("- {}[SpecVersion: {}, ImplVersion: {}]", layer.layerName, layer.specVersion,
                             layer.implementationVersion);
            }
        }

        for (const auto& enabledLayer : m_vkValidationLayers) {
            auto found = std::find_if(layers.begin(), layers.end(),
                                      [&enabledLayer](const vk::LayerProperties& layerProps) { return std::strcmp(enabledLayer, &layerProps.layerName[0]) == 0; });
            if (found == layers.end()) {
                spdlog::critical("Layer needed ({}) is not available. Quitting.", enabledLayer);
                throw std::runtime_error("Vulkan layer missing.");
            }
        }
    }

    void ApplicationBase::InitVulkan(const std::string_view& applicationName, std::uint32_t applicationVersion,
                                     const std::vector<std::string>& requiredInstanceExtensions)
    {
        spdlog::info("Initializing Vulkan...");
        {
            vk::DynamicLoader dl;
            auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }

        std::vector<const char*> enabledExtensions;
        auto glfwExtensionCount = 0U;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        enabledExtensions.resize(glfwExtensionCount + requiredInstanceExtensions.size());
        // NOLINTNEXTLINE
        auto i = 0ULL;
        for (; i < glfwExtensionCount; ++i) { enabledExtensions[i] = glfwExtensions[i]; }
        for (; i < glfwExtensionCount + requiredInstanceExtensions.size(); ++i) {
            enabledExtensions[i] = requiredInstanceExtensions[i - glfwExtensionCount].c_str();
        }

        // NOLINTNEXTLINE
        auto useValidationLayers = m_config.m_useValidationLayers;
#ifndef NDEBUG
        useValidationLayers = true;
#endif
        if (useValidationLayers) {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            m_vkValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        CheckVKInstanceExtensions(enabledExtensions);
        CheckVKInstanceLayers();

        {
            // NOLINTNEXTLINE
            auto api_version = VK_API_VERSION_1_2;
            vk::ApplicationInfo appInfo{applicationName.data(), applicationVersion, engineName.data(), engineVersion,
                                        static_cast<std::uint32_t>(api_version)};
            vk::InstanceCreateInfo createInfo{ vk::InstanceCreateFlags(), &appInfo, static_cast<std::uint32_t>(m_vkValidationLayers.size()), m_vkValidationLayers.data(),
                static_cast<std::uint32_t>(enabledExtensions.size()), enabledExtensions.data() };

            m_vkInstance = vk::createInstanceUnique(createInfo);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkInstance.get());
        }

        if (useValidationLayers) {
            vk::DebugUtilsMessageSeverityFlagsEXT debugUtilsSeverities =
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            vk::DebugUtilsMessageTypeFlagsEXT debugUtilsTypes = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral;
#ifndef NDEBUG
            debugUtilsSeverities |=
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
            debugUtilsTypes |=
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
#endif

            vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo{
                {}, debugUtilsSeverities, debugUtilsTypes, DebugOutputCallback, this};
            m_vkDebugUtilsMessenger = m_vkInstance->createDebugUtilsMessengerEXTUnique(messengerCreateInfo);
        }
        spdlog::info("Vulkan instance created.");

        {
            auto physicalDeviceList = m_vkInstance->enumeratePhysicalDevices();
            for (const auto& device : physicalDeviceList) {
                auto score = ScorePhysicalDevice(device);
                m_vkPhysicalDevices[score] = device;
            }
        }

        spdlog::info("Initializing Vulkan... done.");
    }

    std::unique_ptr<gfx::LogicalDevice> ApplicationBase::CreateLogicalDevice(
        const cfg::WindowCfg& windowCfg, const std::vector<std::string>& requiredDeviceExtensions,
        void* featuresNextChain, const gfx::Surface& surface, const function_view<bool(const vk::PhysicalDevice&)>& additionalDeviceChecks) const
    {
        vk::PhysicalDevice physicalDevice;
        std::vector<gfx::DeviceQueueDesc> deviceQueueDesc;
        std::vector<std::string> requiredDeviceExtensionsInternal = requiredDeviceExtensions;
        if (surface) { requiredDeviceExtensionsInternal.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); }
        auto foundDevice = false;
        for (const auto& device : m_vkPhysicalDevices) {
            deviceQueueDesc.clear();

            if (!CheckDeviceExtensions(device.second, requiredDeviceExtensions)) { continue; }
            if (!additionalDeviceChecks(device.second)) { continue; }

            for (const auto& queueDesc : windowCfg.m_queues) {
                auto queueFamilyIndex = qf::findQueueFamily(device.second, queueDesc, surface.GetHandle());
                if (queueFamilyIndex != -1) {
                    deviceQueueDesc.emplace_back(queueFamilyIndex, queueDesc.m_priorities);
                }
                else {
                    break;
                }
            }

            if (deviceQueueDesc.size() == windowCfg.m_queues.size()) {
                physicalDevice = device.second;
                foundDevice = true;
                break;
            }
        }

        if constexpr (use_debug_pipeline) {
            if (m_vkPhysicalDevices.size() == 1) {
                auto devProps = (*m_vkPhysicalDevices.begin()).second.getProperties();
                if (devProps.pipelineCacheUUID[0] == 'r' && devProps.pipelineCacheUUID[1] == 'd'
                    && devProps.pipelineCacheUUID[2] == 'o' && devProps.pipelineCacheUUID[3] == 'c') {
                    physicalDevice = (*m_vkPhysicalDevices.begin()).second;
                    foundDevice = true;
                    while (deviceQueueDesc.size() < windowCfg.m_queues.size()) {
                        deviceQueueDesc.emplace_back(0, windowCfg.m_queues[deviceQueueDesc.size()].m_priorities);
                    }
                }
            }
        }

        if (!foundDevice) {
            spdlog::critical("Could not find suitable Vulkan GPU.");
            throw std::runtime_error("Could not find suitable Vulkan GPU.");
        }

        auto logicalDevice = std::make_unique<gfx::LogicalDevice>(windowCfg, physicalDevice, deviceQueueDesc,
                                                                  requiredDeviceExtensionsInternal, featuresNextChain, surface);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(logicalDevice->GetHandle());
        return std::move(logicalDevice);
    }

    /*std::unique_ptr<gfx::LogicalDevice> ApplicationBase::CreateLogicalDevice(const cfg::WindowCfg& windowCfg,
        const vk::SurfaceKHR& surface) const
    {
        return CreateLogicalDevice(windowCfg, surface, [](const vk::PhysicalDevice&) { return true; });
    }*/

    std::unique_ptr<gfx::LogicalDevice>
    ApplicationBase::CreateLogicalDevice(const cfg::WindowCfg& windowCfg,
                                         const std::vector<std::string>& requiredDeviceExtensions,
                                         void* featuresNextChain, const gfx::Surface& surface) const
    {
        auto requestedFormats = cfg::GetVulkanSurfaceFormatsFromConfig(windowCfg);
        std::sort(requestedFormats.begin(), requestedFormats.end(), [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0.format < f1.format; });
        auto requestedPresentMode = cfg::GetVulkanPresentModeFromConfig(windowCfg);
        auto requestedAdditionalImgCnt = cfg::GetVulkanAdditionalImageCountFromConfig(windowCfg);
        glm::uvec2 requestedExtend(windowCfg.m_windowWidth, windowCfg.m_windowHeight);

        // NOLINTNEXTLINE
        return CreateLogicalDevice(windowCfg, requiredDeviceExtensions, featuresNextChain, surface, [&surface, &requestedFormats, &requestedPresentMode, &requestedAdditionalImgCnt, &requestedExtend](const vk::PhysicalDevice& device) {
            // NOLINTNEXTLINE
            auto deviceSurfaceCaps = device.getSurfaceCapabilitiesKHR(surface.GetHandle());
            auto deviceSurfaceFormats = device.getSurfaceFormatsKHR(surface.GetHandle());
            auto presentModes = device.getSurfacePresentModesKHR(surface.GetHandle());
            auto formatSupported = false;
            auto presentModeSupported = false;
            auto sizeSupported = false;
            auto imageCountSupported = false;

            if (deviceSurfaceFormats.size() == 1 && deviceSurfaceFormats[0].format == vk::Format::eUndefined) {
                formatSupported = true;
            }
            else {
                std::sort(deviceSurfaceFormats.begin(), deviceSurfaceFormats.end(), [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0.format < f1.format; });
                std::vector<vk::SurfaceFormatKHR> formatIntersection;
                std::set_intersection(deviceSurfaceFormats.begin(), deviceSurfaceFormats.end(), requestedFormats.begin(), requestedFormats.end(),
                    std::back_inserter(formatIntersection), [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0 == f1; });
                if (!formatIntersection.empty()) {
                    formatSupported = true; // no color space check here as color space does not depend on the color space flag but the actual format.
                }
            }
            /*else for (const auto& availableFormat : deviceSurfaceFormats) {
                if (availableFormat.format == requestedFormat.format && availableFormat.colorSpace == requestedFormat.colorSpace) formatSupported = true;
            }*/

            for (const auto& availablePresentMode : presentModes) {
                if (availablePresentMode == requestedPresentMode) { presentModeSupported = true; }
            }

            glm::uvec2 currentExtend(deviceSurfaceCaps.currentExtent.width, deviceSurfaceCaps.currentExtent.height);
            if (currentExtend == requestedExtend) {
                sizeSupported = true;
            }
            else {
                glm::uvec2 minExtend(deviceSurfaceCaps.minImageExtent.width, deviceSurfaceCaps.minImageExtent.height);
                glm::uvec2 maxExtend(deviceSurfaceCaps.maxImageExtent.width, deviceSurfaceCaps.maxImageExtent.height);
                auto actualExtent = glm::clamp(requestedExtend, minExtend, maxExtend);
                if (actualExtent == requestedExtend) { sizeSupported = true; }
            }

            auto imageCount = deviceSurfaceCaps.minImageCount + requestedAdditionalImgCnt;
            if (deviceSurfaceCaps.maxImageCount == 0 || imageCount <= deviceSurfaceCaps.maxImageCount) {
                imageCountSupported = true;
            }

            return formatSupported && presentModeSupported && sizeSupported && imageCountSupported;
        });
    }

    unsigned int ApplicationBase::ScorePhysicalDevice(const vk::PhysicalDevice& device)
    {
        auto deviceProperties = device.getProperties();
        auto deviceFeatures = device.getFeatures();
        auto deviceQueueFamilyProperties = device.getQueueFamilyProperties();

        spdlog::info("Found physical device '{}' [DriverVersion: {}].", deviceProperties.deviceName, deviceProperties.driverVersion);
        auto score = 0U;

        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { score += 1000; }

        if (!((deviceFeatures.vertexPipelineStoresAndAtomics != 0) && (deviceFeatures.fragmentStoresAndAtomics != 0)
              && (deviceFeatures.geometryShader != 0) && (deviceFeatures.tessellationShader != 0) && (deviceFeatures.largePoints != 0)
              && (deviceFeatures.shaderUniformBufferArrayDynamicIndexing != 0)
              && (deviceFeatures.shaderStorageBufferArrayDynamicIndexing != 0))) {
            score = 0U;
        }

        spdlog::info("Scored: {}", score);
        return score;
    }

    bool ApplicationBase::CheckDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<std::string>& requiredExtensions)
    {
        auto availableExtensions = device.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredDeviceExtensions(requiredExtensions.begin(), requiredExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredDeviceExtensions.erase(std::string(&extension.extensionName[0]));
        }

        return requiredDeviceExtensions.empty();
    }

    PFN_vkVoidFunction ApplicationBase::LoadVKInstanceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory) const
    {
        auto func = vkGetInstanceProcAddr(*m_vkInstance, functionName.c_str());
        if (func == nullptr) {
            if (mandatory) {
                spdlog::critical("Could not load instance function '{}' [{}].", functionName, extensionName);
                throw std::runtime_error("Could not load mandatory instance function.");
            }
            spdlog::warn("Could not load instance function '{}' [{}].", functionName, extensionName);
        }

        return func;
    }
}
