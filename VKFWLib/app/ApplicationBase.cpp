/**
 * @file   ApplicationBase.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Implements the application base class.
 */

#include "ApplicationBase.h"
#include "app/VKWindow.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#pragma warning(push, 3)
#include <Windows.h>
#pragma warning(pop)
#undef min
#undef max

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>
#include <set>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "gfx/vk/LogicalDevice.h"

namespace vk {
    // VK_EXT_debug_report
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT = nullptr;
    PFN_vkDebugReportMessageEXT DebugReportMessageEXT = nullptr;
}

namespace vku {

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
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugOutputCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
        uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {

        auto vkLogLevel = VK_GEN;
        if (flags | VK_DEBUG_REPORT_DEBUG_BIT_EXT) vkLogLevel = VK_DEBUG;
        else if (flags | VK_DEBUG_REPORT_INFORMATION_BIT_EXT) vkLogLevel = VK_INFO;
        else if (flags | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) vkLogLevel = VK_PERF_WARNING;
        else if (flags | VK_DEBUG_REPORT_WARNING_BIT_EXT) vkLogLevel = VK_WARNING;
        else if (flags | VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            vkLogLevel = VK_ERROR;
        }

        LOG(vkLogLevel) << " [" << layerPrefix << "] Code " << code << " : " << msg;

        return VK_FALSE;
    }


    ApplicationBase::GLFWInitObject::GLFWInitObject()
    {
        glfwInit();
    }

    ApplicationBase::GLFWInitObject::~GLFWInitObject()
    {
        glfwTerminate();
    }

    namespace qf {

        int findQueueFamily(const vk::PhysicalDevice& device, const cfg::QueueCfg& desc, const vk::SurfaceKHR& surface = vk::SurfaceKHR())
        {
            vk::QueueFlags reqFlags;
            if (!(desc.graphics_ || desc.compute_) && desc.transfer_) reqFlags |= vk::QueueFlagBits::eTransfer;
            if (desc.graphics_) reqFlags |= vk::QueueFlagBits::eGraphics;
            if (desc.compute_) reqFlags |= vk::QueueFlagBits::eCompute;
            if (desc.sparseBinding_) reqFlags |= vk::QueueFlagBits::eSparseBinding;

            auto queueProps = device.getQueueFamilyProperties();
            auto queueCount = static_cast<uint32_t>(queueProps.size());
            for (uint32_t i = 0; i < queueCount; i++) {
                if (queueProps[i].queueCount < desc.priorities_.size()) continue;
                if (queueProps[i].queueFlags & reqFlags) {
                    if (surface && desc.graphics_ && !device.getSurfaceSupportKHR(i, surface)) {
                        continue;
                    }
                    return i;
                }
            }
            return -1;
        }
    }


    namespace cfg {

        vk::SurfaceFormatKHR GetVulkanSurfaceFormatFromConfig(const WindowCfg& cfg)
        {
            auto format = vk::Format::eR8G8B8A8Unorm;
            if (cfg.backbufferBits_ == 32) format = vk::Format::eR8G8B8A8Unorm;
            if (cfg.backbufferBits_ == 24) format = vk::Format::eR8G8B8Unorm;
            if (cfg.backbufferBits_ == 16) format = vk::Format::eR5G6B5UnormPack16;
            vk::ColorSpaceKHR colorSpace = {};
            if (cfg.useSRGB_) colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
            return vk::SurfaceFormatKHR{ format, colorSpace };
        }

        vk::PresentModeKHR GetVulkanPresentModeFromConfig(const WindowCfg& cfg)
        {
            vk::PresentModeKHR presentMode = {};
            if (cfg.swapOptions_ == cfg::SwapOptions::DOUBLE_BUFFERING) presentMode = vk::PresentModeKHR::eImmediate;
            if (cfg.swapOptions_ == cfg::SwapOptions::DOUBLE_BUFFERING_VSYNC) presentMode = vk::PresentModeKHR::eFifo;
            if (cfg.swapOptions_ == cfg::SwapOptions::TRIPLE_BUFFERING) presentMode = vk::PresentModeKHR::eMailbox;
            return presentMode;
        }

        uint32_t GetVulkanAdditionalImageCountFromConfig(const WindowCfg& cfg)
        {
            if (cfg.swapOptions_ == SwapOptions::TRIPLE_BUFFERING) return 1;
            return 0;
        }
    }


    ApplicationBase* ApplicationBase::instance_ = nullptr;


    /**
     * Construct a new application.
     * @param applicationName the applications name.
     * @param applicationVersion the applications version.
     * @param configFileName the configuration file to use.
     */
    ApplicationBase::ApplicationBase(const std::string& applicationName, uint32_t applicationVersion, const std::string& configFileName) :
        pause_(true),
        stopped_(false),
        currentTime_(0.0),
        elapsedTime_(0.0)
    {
        LOG(DEBUG) << L"Trying to load configuration.";
        std::ifstream configFile(configFileName, std::ios::in);
        if (configFile.is_open()) {
            boost::archive::xml_iarchive ia(configFile);
            ia >> boost::serialization::make_nvp("configuration", config_);

            // always directly write configuration to update version.
            std::ofstream ofs(configFileName, std::ios::out);
            boost::archive::xml_oarchive oa(ofs);
            oa << boost::serialization::make_nvp("configuration", config_);
        }
        else {
            LOG(DEBUG) << L"Configuration file not found. Using standard config.";
        }

        InitVulkan(applicationName, applicationVersion);
        instance_ = this;

        for (auto& wc : config_.windows_) {
            windows_.emplace_back(wc);
            windows_.back().ShowWindow();
        }
    }

    ApplicationBase::~ApplicationBase()
    {
        if (vkDebugReportCB_) vk::DestroyDebugReportCallbackEXT(vkInstance_, vkDebugReportCB_, nullptr);
        if (vkInstance_) vkInstance_.destroy();
    }

    VKWindow* ApplicationBase::GetFocusedWindow()
    {
        VKWindow* focusWindow = nullptr;
        for (auto& w : windows_) if (w.IsFocused()) focusWindow = &w;
        return focusWindow;
    }

    void ApplicationBase::SetPause(bool pause)
    {
        LOG_IF(INFO, pause) << L"Begin pause";
        LOG_IF(INFO, !pause) << L"End pause";
        pause_ = pause;
    }

    /**
     * Handles all keyboard input.
     * @param key the key pressed
     * @param scancode the pressed keys scancode
     * @param action the action of the key event
     * @param mods the key modificators
     * @param sender the window that send the keyboard messages
     */
    bool ApplicationBase::HandleKeyboard(int key, int scancode, int action, int mods, VKWindow* sender)
    {
        auto handled = false;
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                // TODO: close window or sth. [10/16/2016 Sebastian Maisch]
                handled = true;
                break;
            case GLFW_KEY_F2:
                guiMode_ = !guiMode_;
                handled = true;
                break;
            case GLFW_KEY_F9:
                // programManager_->RecompileAll();
                handled = true;
                break;
            default: break;
            }
        }

        // if (!handled && IsRunning() && !IsPaused()) handled = cameraView_->HandleKeyboard(key, scancode, action, mods, sender);

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
        if (IsRunning() && !IsPaused()) handled = HandleMouseApp(button, action, mods, mouseWheelDelta, sender);
        // if (!handled && IsRunning() && !IsPaused()) handled = cameraView_->HandleMouse(button, action, mods, mouseWheelDelta, sender);
        return handled;
    }

    /**
     * Handles resize events.
     */
    void ApplicationBase::OnResize(unsigned int width, unsigned int height)
    {
        glm::uvec2 screenSize(width, height);
        Resize(screenSize);
    }

    void ApplicationBase::Resize(const glm::uvec2&)
    {
    }

    void ApplicationBase::StartRun()
    {
        stopped_ = false;
        pause_ = false;
        currentTime_ = glfwGetTime();
    }

    bool ApplicationBase::IsRunning() const
    {
        return !stopped_ && !windows_[0].IsClosing();
    }

    void ApplicationBase::EndRun()
    {
        stopped_ = true;
    }

    void ApplicationBase::Step()
    {
        if (stopped_) {
            Sleep(500);
            return;
        }

        auto currentTime = glfwGetTime();
        elapsedTime_ = currentTime - currentTime_;
        currentTime_ = currentTime;
        glfwPollEvents();

        if (!this->pause_ && (!config_.pauseOnKillFocus_ || GetFocusedWindow())) {
            this->FrameMove(static_cast<float>(currentTime_), static_cast<float>(elapsedTime_));
            this->RenderScene();
        }

        /*ImGui_ImplGlfwGL3_NewFrame();
        if (guiMode_) {
            mainWin.BatchDraw([&](GLBatchRenderTarget & rt) {
                this->RenderGUI();
                ImGui::Render();
            });
        }
        mainWin.Present();*/
    }

    void ApplicationBase::InitVulkan(const std::string& applicationName, uint32_t applicationVersion)
    {
        LOG(INFO) << "Initializing Vulkan...";
        std::vector<const char*> enabledExtensions;
        auto glfwExtensionCount = 0U;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (auto i = 0U; i < glfwExtensionCount; ++i) enabledExtensions.push_back(glfwExtensions[i]);

        // ReSharper disable once CppInitializedValueIsAlwaysRewritten
        auto useValidationLayers = config_.useValidationLayers_;
#ifndef NDEBUG
        useValidationLayers = true;
#endif
        if (useValidationLayers) {
            enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            vkValidationLayers_.push_back("VK_LAYER_LUNARG_standard_validation");
        }

        {
            LOG(INFO) << "VK Instance Extensions:";
            auto extensions = vk::enumerateInstanceExtensionProperties();
            for (const auto& extension : extensions) LOG(INFO) << "- " << extension.extensionName << "[SpecVersion:" << extension.specVersion << "]";

            for (const auto& enabledExt : enabledExtensions) {
                auto found = std::find_if(extensions.begin(), extensions.end(),
                    [&enabledExt](const vk::ExtensionProperties& extProps) { return std::strcmp(enabledExt, extProps.extensionName) == 0; });
                if (found == extensions.end()) {
                    LOG(FATAL) << "Extension needed (" << enabledExt << ") is not available. Quitting.";
                    throw std::runtime_error("Vulkan extension missing.");
                }
            }
        }

        {
            LOG(INFO) << "VK Instance Layers:";
            auto layers = vk::enumerateInstanceLayerProperties();
            for (const auto& layer : layers) LOG(INFO) << "- " << layer.layerName << "[SpecVersion:" << layer.specVersion << ",ImplVersion:" << layer.implementationVersion << "]";

            for (const auto& enabledLayer : vkValidationLayers_) {
                auto found = std::find_if(layers.begin(), layers.end(),
                    [&enabledLayer](const vk::LayerProperties& layerProps) { return std::strcmp(enabledLayer, layerProps.layerName) == 0; });
                if (found == layers.end()) {
                    LOG(FATAL) << "Layer needed (" << enabledLayer << ") is not available. Quitting.";
                    throw std::runtime_error("Vulkan layer missing.");
                }
            }
        }

        {
            vk::ApplicationInfo appInfo{ applicationName.c_str(), applicationVersion, engineName, engineVersion, VK_API_VERSION_1_0 };
            vk::InstanceCreateInfo createInfo{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(vkValidationLayers_.size()), vkValidationLayers_.data(),
                static_cast<uint32_t>(enabledExtensions.size()), enabledExtensions.data() };

            vkInstance_ = vk::createInstance(createInfo);

            vk::CreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(LoadVKInstanceFunction("vkCreateDebugReportCallbackEXT", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true));
            vk::DestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(LoadVKInstanceFunction("vkDestroyDebugReportCallbackEXT", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true));
            vk::DebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(LoadVKInstanceFunction("vkDebugReportMessageEXT", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true));
        }

        vk::DebugReportFlagsEXT drFlags(vk::DebugReportFlagBitsEXT::eError);
        drFlags |= vk::DebugReportFlagBitsEXT::eWarning;
#ifndef NDEBUG
        drFlags |= vk::DebugReportFlagBitsEXT::ePerformanceWarning;
        drFlags |= vk::DebugReportFlagBitsEXT::eInformation;
        drFlags |= vk::DebugReportFlagBitsEXT::eDebug;
#endif
        vk::DebugReportCallbackCreateInfoEXT drCreateInfo{ drFlags, DebugOutputCallback, this };

        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        VkDebugReportCallbackEXT dbgReportCB = VK_NULL_HANDLE;
        auto result = vk::CreateDebugReportCallbackEXT(vkInstance_, &static_cast<const VkDebugReportCallbackCreateInfoEXT&>(drCreateInfo), nullptr, &dbgReportCB);
        if (result != VK_SUCCESS) {
            LOG(FATAL) << "Could not create DebugReportCallback (" << result << ").";
            throw std::runtime_error("Could not create DebugReportCallback.");
        }
        vkDebugReportCB_ = vk::DebugReportCallbackEXT(dbgReportCB);
        LOG(INFO) << "Vulkan instance created.";

        {
            auto physicalDeviceList = vkInstance_.enumeratePhysicalDevices();
            for (const auto& device : physicalDeviceList) {
                auto score = ScorePhysicalDevice(device);
                vkPhysicalDevices_[score] = device;
            }
        }

        LOG(INFO) << "Initializing Vulkan... done.";
    }

    std::unique_ptr<gfx::LogicalDevice> ApplicationBase::CreateLogicalDevice(const std::vector<cfg::QueueCfg>& queueDescs, const vk::SurfaceKHR& surface, std::function<bool(const vk::PhysicalDevice&)> additionalDeviceChecks) const
    {
        vk::PhysicalDevice physicalDevice;
        std::vector<gfx::DeviceQueueDesc> deviceQueueDesc;
        std::vector<std::string> requiredExtensions;
        if (surface) requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        auto foundDevice = false;
        for (const auto& device : vkPhysicalDevices_) {
            deviceQueueDesc.clear();

            if (!CheckDeviceExtensions(device.second, requiredExtensions)) continue;
            if (!additionalDeviceChecks(device.second)) continue;

            for (const auto& queueDesc : queueDescs) {
                auto queueFamilyIndex = qf::findQueueFamily(device.second, queueDesc, surface);
                if (queueFamilyIndex != -1) deviceQueueDesc.emplace_back(queueFamilyIndex, queueDesc.priorities_);
                else break;
            }

            if (deviceQueueDesc.size() == queueDescs.size()) {
                physicalDevice = device.second;
                foundDevice = true;
                break;
            }
        }

        if (!foundDevice) {
            LOG(FATAL) << "Could not find suitable Vulkan GPU.";
            throw std::runtime_error("Could not find suitable Vulkan GPU.");
        }

        return std::make_unique<gfx::LogicalDevice>(physicalDevice, deviceQueueDesc, surface);
    }

    std::unique_ptr<gfx::LogicalDevice> ApplicationBase::CreateLogicalDevice(const std::vector<cfg::QueueCfg>& queueDescs, const vk::SurfaceKHR& surface) const
    {
        return CreateLogicalDevice(queueDescs, surface, [](const vk::PhysicalDevice&) { return true; });
    }

    std::unique_ptr<gfx::LogicalDevice> ApplicationBase::CreateLogicalDevice(const cfg::WindowCfg& windowCfg, const vk::SurfaceKHR& surface) const
    {
        auto requestedFormat = cfg::GetVulkanSurfaceFormatFromConfig(windowCfg);
        auto requestedPresentMode = cfg::GetVulkanPresentModeFromConfig(windowCfg);
        auto requestedAdditionalImgCnt = cfg::GetVulkanAdditionalImageCountFromConfig(windowCfg);
        glm::uvec2 requestedExtend(windowCfg.windowWidth_, windowCfg.windowHeight_);

        return CreateLogicalDevice(windowCfg.queues_, surface, [&surface, &requestedFormat, &requestedPresentMode, &requestedAdditionalImgCnt, &requestedExtend](const vk::PhysicalDevice& device)
        {
            auto deviceSurfaceCaps = device.getSurfaceCapabilitiesKHR(surface);
            auto deviceSurfaceFormats = device.getSurfaceFormatsKHR(surface);
            auto presentModes = device.getSurfacePresentModesKHR(surface);
            auto formatSupported = false, presentModeSupported = false, sizeSupported = false, imageCountSupported = false;

            if (deviceSurfaceFormats.size() == 1 && deviceSurfaceFormats[0].format == vk::Format::eUndefined) formatSupported = true;
            else for (const auto& availableFormat : deviceSurfaceFormats) {
                if (availableFormat.format == requestedFormat.format && availableFormat.colorSpace == requestedFormat.colorSpace) formatSupported = true;
            }

            for (const auto& availablePresentMode : presentModes) {
                if (availablePresentMode == requestedPresentMode) presentModeSupported = true;
            }

            glm::uvec2 currentExtend(deviceSurfaceCaps.currentExtent.width, deviceSurfaceCaps.currentExtent.height);
            if (currentExtend == requestedExtend) sizeSupported = true;
            else {
                glm::uvec2 minExtend(deviceSurfaceCaps.minImageExtent.width, deviceSurfaceCaps.minImageExtent.height);
                glm::uvec2 maxExtend(deviceSurfaceCaps.maxImageExtent.width, deviceSurfaceCaps.maxImageExtent.height);
                auto actualExtent = glm::clamp(requestedExtend, minExtend, maxExtend);
                if (actualExtent == requestedExtend) sizeSupported = true;
            }

            auto imageCount = deviceSurfaceCaps.minImageCount + requestedAdditionalImgCnt;
            if (deviceSurfaceCaps.maxImageCount == 0 || imageCount <= deviceSurfaceCaps.maxImageCount) imageCountSupported = true;

            return formatSupported && presentModeSupported && sizeSupported && imageCountSupported;
        });
    }

    unsigned int ApplicationBase::ScorePhysicalDevice(const vk::PhysicalDevice& device)
    {
        auto deviceProperties = device.getProperties();
        auto deviceFeatures = device.getFeatures();
        auto deviceQueueFamilyProperties = device.getQueueFamilyProperties();

        LOG(INFO) << "Found physical device '" << deviceProperties.deviceName << "' [DriverVersion:" << deviceProperties.driverVersion << "].";
        auto score = 0U;

        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 1000;

        if (!(deviceFeatures.vertexPipelineStoresAndAtomics && deviceFeatures.fragmentStoresAndAtomics
            && deviceFeatures.geometryShader && deviceFeatures.tessellationShader && deviceFeatures.largePoints
            && deviceFeatures.shaderUniformBufferArrayDynamicIndexing && deviceFeatures.shaderStorageBufferArrayDynamicIndexing)) score = 0U;

        LOG(INFO) << "Scored: " << score;
        return score;
    }

    bool ApplicationBase::CheckDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<std::string>& requiredExtensions)
    {
        auto availableExtensions = device.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredDeviceExtensions(requiredExtensions.begin(), requiredExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredDeviceExtensions.erase(extension.extensionName);
        }

        return requiredDeviceExtensions.empty();
    }

    PFN_vkVoidFunction ApplicationBase::LoadVKInstanceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory) const
    {
        auto func = vkGetInstanceProcAddr(static_cast<vk::Instance>(vkInstance_), functionName.c_str());
        if (func == nullptr) {
            if (mandatory) {
                LOG(FATAL) << "Could not load instance function '" << functionName << "' [" << extensionName << "].";
                throw std::runtime_error("Could not load mandatory instance function.");
            }
            LOG(WARNING) << "Could not load instance function '" << functionName << "' [" << extensionName << "].";
        }

        return func;
    }
}
