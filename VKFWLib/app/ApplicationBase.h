/**
 * @file   ApplicationBase.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Declares the application base class.
 */

#pragma once

#include "main.h"
// #include "core/sceneobject/SceneObjectManager.h"

namespace vku::cfg {
    class Configuration;

    std::vector<vk::SurfaceFormatKHR> GetVulkanSurfaceFormatsFromConfig(const WindowCfg& cfg);
    vk::PresentModeKHR GetVulkanPresentModeFromConfig(const WindowCfg& cfg);
    std::uint32_t GetVulkanAdditionalImageCountFromConfig(const WindowCfg& cfg);
}

namespace vku::gfx {
    class LogicalDevice;
}

namespace vku {

    class VKWindow;

    class ApplicationBase
    {
    public:
        VKUDllExport ApplicationBase(const std::string& applicationName, std::uint32_t applicationVersion, const std::string& configFileName);
        ApplicationBase(const ApplicationBase&) = delete;
        ApplicationBase(ApplicationBase&&) = delete;
        ApplicationBase& operator=(const ApplicationBase&) = delete;
        ApplicationBase& operator=(ApplicationBase&&) = delete;
        virtual VKUDllExport ~ApplicationBase();

        static ApplicationBase& instance() { return *instance_; };

        /** Starts the application. */
        VKUDllExport void StartRun();
        /** Checks if the application is still running. */
        VKUDllExport bool IsRunning() const;
        /** Make one application <em>step</em> (rendering etc.). */
        VKUDllExport void Step();
        /** Called if the application is to end running. */
        VKUDllExport void EndRun();

        bool IsPaused() const { return pause_; }
        bool IsGUIMode() const { return guiMode_; }
        VKUDllExport VKWindow* GetFocusedWindow();
        VKUDllExport VKWindow* GetWindow(unsigned int idx);
        // VKUDllExport const SceneObjectManager& GetSceneObjectManager() const { return sceneObjectManager_; }

        void SetPause(bool pause);

        virtual VKUDllExport bool HandleKeyboard(int key, int scancode, int action, int mods, VKWindow* sender);
        bool HandleMouse(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender);
        virtual bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender) = 0;
        void OnResize(unsigned int width, unsigned int height, const VKWindow* window);
        virtual void Resize(const glm::uvec2& screenSize, const VKWindow* window);

        const cfg::Configuration& GetConfig() const { return config_; };
        const std::vector<const char*>& GetVKValidationLayers() const { return vkValidationLayers_; }
        const vk::Instance& GetVKInstance() const { return *vkInstance_; }
        std::unique_ptr<gfx::LogicalDevice> CreateLogicalDevice(const cfg::WindowCfg& windowCfg,
            const vk::SurfaceKHR& surface = vk::SurfaceKHR()) const;
        // std::unique_ptr<gfx::LogicalDevice> CreateLogicalDevice(const cfg::WindowCfg& windowCfg, const vk::SurfaceKHR& surface) const;

    private:
        class GLFWInitObject
        {
        public:
            GLFWInitObject();
            ~GLFWInitObject();
        };

        GLFWInitObject forceGLFWInit_;

        /** Holds the applications instance. */
        static ApplicationBase* instance_;

        /** Holds the configuration file name. */
        std::string configFileName_;
        /** Holds the configuration for this application. */
        cfg::Configuration config_;
        /** Holds the windows. */
        std::vector<VKWindow> windows_;

        // application status
        /** <c>true</c> if application is paused. */
        bool pause_;
        /**  <c>true</c> if the application has stopped (i.e. the last scene has finished). */
        bool stopped_;
        /** The (global) time of the application. */
        double currentTime_;
        /** Time elapsed in the frame. */
        double elapsedTime_;
        /** Hold whether GUI mode is switched on. */
        bool guiMode_ = true;

        /** Holds the scene object manager. */
        // SceneObjectManager sceneObjectManager_;

    protected:
        /**
         * Moves a single frame.
         * @param time the time elapsed since the application started
         * @param elapsed the time elapsed since the last frame
         */
        virtual void FrameMove(float time, float elapsed, const VKWindow* window) = 0;
        /** Render the scene. */
        virtual void RenderScene(const VKWindow* window) = 0;
        /** Render the scenes GUI. */
        virtual void RenderGUI() = 0;

    private:
        void InitVulkan(const std::string& applicationName, std::uint32_t applicationVersion);
        static unsigned int ScorePhysicalDevice(const vk::PhysicalDevice& device);
        static bool CheckDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<std::string>& requiredExtensions);
        std::unique_ptr<gfx::LogicalDevice> CreateLogicalDevice(const cfg::WindowCfg& windowCfg,
            const vk::SurfaceKHR& surface, std::function<bool(const vk::PhysicalDevice&)> additionalDeviceChecks) const;
        PFN_vkVoidFunction LoadVKInstanceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory = false) const;

        void CheckVKInstanceExtensions(const std::vector<const char*>& enabledExtensions);
        void CheckVKInstanceLayers();

        /** Holds the Vulkan validation layers. */
        std::vector<const char*> vkValidationLayers_;
        /** Holds the Vulkan instance. */
        vk::UniqueInstance vkInstance_;
        /** Holds the dispatch loader for the instance. */
        vk::DispatchLoaderDynamic vkDispatchLoaderInst_;
        /** Holds the debug report callback. */
        vk::UniqueHandle<vk::DebugReportCallbackEXT, vk::DispatchLoaderDynamic> vkDebugReportCB_;
        /** Holds the physical devices. */
        std::map<unsigned int, vk::PhysicalDevice> vkPhysicalDevices_;
        /** Holds the physical device. */
        //vk::PhysicalDevice vkPhysicalDevice_;
        /** Holds the logical device. */
        //vk::Device vkDevice_;
        /** Holds the graphics queue. */
        //vk::Queue vkGraphicsQueue_;


    };
}
