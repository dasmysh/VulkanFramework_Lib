/**
 * @file   ApplicationBase.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Implements the application base class.
 */

#include "ApplicationBase.h"
#include "app/VKWindow.h"

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

namespace vk {
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

    /**
     * Construct a new application.
     * @param window the applications main window
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

        for (auto& wc : config_.windows_) {
            windows_.emplace_back(wc.windowTitle_, wc);
            windows_.back().RegisterApplication(*this);
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
     * @param vkCode the key pressed
     * @param bKeyDown <code>true</code> if the key is pressed, else it is released
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
            }
        }

        // if (!handled && IsRunning() && !IsPaused()) handled = cameraView_->HandleKeyboard(key, scancode, action, mods, sender);

        return handled;
    }

    /**
     * Handles mouse input.
     * @param buttonAction mouse action flags, see RAWMOUSE.usButtonFlags
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
        std::vector<const char*> validationLayers;
        auto glfwExtensionCount = 0U;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (auto i = 0U; i < glfwExtensionCount; ++i) enabledExtensions.push_back(glfwExtensions[i]);

        auto useValidationLayers = config_.useValidationLayers_;
#ifndef NDEBUG
        useValidationLayers = true;
#endif
        if (useValidationLayers) {
            enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            // TODO: add debug markers extension? [10/19/2016 Sebastian Maisch]
            validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
        }

        LOG(INFO) << "VKExtensions:";
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

        LOG(INFO) << "VKLayers:";
        auto layers = vk::enumerateInstanceLayerProperties();
        for (const auto& layer : layers) LOG(INFO) << "- " << layer.layerName << "[SpecVersion:" << layer.specVersion << ",ImplVersion:" << layer.implementationVersion << "]";

        for (const auto& enabledLayer : validationLayers) {
            auto found = std::find_if(layers.begin(), layers.end(),
                [&enabledLayer](const vk::LayerProperties& layerProps) { return std::strcmp(enabledLayer, layerProps.layerName) == 0; });
            if (found == layers.end()) {
                LOG(FATAL) << "Layer needed (" << enabledLayer << ") is not available. Quitting.";
                throw std::runtime_error("Vulkan layer missing.");
            }
        }

        vk::ApplicationInfo appInfo{ applicationName.c_str(), applicationVersion, engineName, engineVersion, VK_API_VERSION_1_0 };
        vk::InstanceCreateInfo createInfo{vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
            static_cast<uint32_t>(enabledExtensions.size()), enabledExtensions.data() };

        vkInstance_ = vk::createInstance(createInfo);

        vk::DebugReportFlagsEXT drFlags(vk::DebugReportFlagBitsEXT::eError);
        drFlags |= vk::DebugReportFlagBitsEXT::eWarning;
#ifndef NDEBUG
        drFlags |= vk::DebugReportFlagBitsEXT::ePerformanceWarning;
        drFlags |= vk::DebugReportFlagBitsEXT::eInformation;
        drFlags |= vk::DebugReportFlagBitsEXT::eDebug;
#endif
        vk::DebugReportCallbackCreateInfoEXT drCreateInfo{ drFlags, DebugOutputCallback, this };

        vk::CreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(LoadVKFunction("vkCreateDebugReportCallbackEXT", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true));
        vk::DestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(LoadVKFunction("vkDestroyDebugReportCallbackEXT", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true));
        vk::DebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(LoadVKFunction("vkDebugReportMessageEXT", VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true));
        
        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        VkDebugReportCallbackEXT dbgReportCB = VK_NULL_HANDLE;
        auto result = vk::CreateDebugReportCallbackEXT(vkInstance_, &static_cast<const VkDebugReportCallbackCreateInfoEXT&>(drCreateInfo), nullptr, &dbgReportCB);
        if (result != VK_SUCCESS) {
            LOG(FATAL) << "Could not create DebugReportCallback (" << result << ").";
            throw std::runtime_error("Could not create DebugReportCallback.");
        }
        vkDebugReportCB_ = vk::DebugReportCallbackEXT(dbgReportCB);

        // TODO: debug markers? [10/19/2016 Sebastian Maisch]

        LOG(INFO) << "Initializing Vulkan... done.";
    }

    PFN_vkVoidFunction ApplicationBase::LoadVKFunction(const std::string& functionName, const std::string& extensionName, bool mandatory)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(static_cast<vk::Instance>(vkInstance_), functionName.c_str()));
        if (vk::CreateDebugReportCallbackEXT == nullptr) {
            if (mandatory) {
                LOG(FATAL) << "Could not load function '" << functionName << "' [" << extensionName << "].";
                throw std::runtime_error("Could not load mandatory function.");
            } else {
                LOG(WARNING) << "Could not load function '" << functionName << "' [" << extensionName << "].";
            }
        }

        return reinterpret_cast<PFN_vkVoidFunction>(func);
    }
}
