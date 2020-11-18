/**
 * @file   ApplicationBase.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Declares the application base class.
 */

#pragma once

#include "main.h"

#include <glm/vec2.hpp>

#include <core/function_view.h>

namespace vkfw_core::cfg {
    class Configuration;

    std::vector<vk::SurfaceFormatKHR> GetVulkanSurfaceFormatsFromConfig(const WindowCfg& cfg);
    vk::PresentModeKHR GetVulkanPresentModeFromConfig(const WindowCfg& cfg);
    std::uint32_t GetVulkanAdditionalImageCountFromConfig(const WindowCfg& cfg);
}

namespace vkfw_core::gfx {
    class LogicalDevice;
}

namespace vkfw_core {

    class VKWindow;

    class ApplicationBase
    {
    public:
        ApplicationBase(const std::string_view& applicationName, std::uint32_t applicationVersion,
                        const std::string_view& configFileName,
                        const std::vector<std::string>& requiredInstanceExtensions = {},
                        const std::vector<std::string>& requiredDeviceExtensions = {},
                        void* deviceFeaturesNextChain = nullptr);
        ApplicationBase(const ApplicationBase&) = delete;
        ApplicationBase(ApplicationBase&&) = delete;
        ApplicationBase& operator=(const ApplicationBase&) = delete;
        ApplicationBase& operator=(ApplicationBase&&) = delete;
        virtual ~ApplicationBase() noexcept;

        static ApplicationBase& instance() { return *m_instance; };

        /** Starts the application. */
        void StartRun();
        /** Checks if the application is still running. */
        [[nodiscard]] bool IsRunning() const;
        /** Make one application <em>step</em> (rendering etc.). */
        void Step();
        /** Called if the application is to end running. */
        void EndRun();

        [[nodiscard]] bool IsPaused() const { return m_pause; }
        [[nodiscard]] bool IsGUIMode() const { return m_guiMode; }
        VKWindow* GetFocusedWindow();
        VKWindow* GetWindow(unsigned int idx);
        // VKUDllExport const SceneObjectManager& GetSceneObjectManager() const { return m_sceneObjectManager; }

        void SetPause(bool pause);

        virtual bool HandleKeyboard(int key, int scancode, int action, int mods, VKWindow* sender);
        bool HandleMouse(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender);
        virtual bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender) = 0;
        void OnResize(unsigned int width, unsigned int height, VKWindow* window);
        virtual void Resize(const glm::uvec2& screenSize, VKWindow* window);

        [[nodiscard]] const cfg::Configuration& GetConfig() const { return m_config; };
        [[nodiscard]] const std::vector<const char*>& GetVKValidationLayers() const { return m_vkValidationLayers; }
        [[nodiscard]] const vk::Instance& GetVKInstance() const { return *m_vkInstance; }
        [[nodiscard]] std::unique_ptr<gfx::LogicalDevice>
        CreateLogicalDevice(const cfg::WindowCfg& windowCfg, const std::vector<std::string>& requiredDeviceExtensions,
                            void* featuresNextChain, const vk::SurfaceKHR& surface = vk::SurfaceKHR()) const;
        // std::unique_ptr<gfx::LogicalDevice> CreateLogicalDevice(const cfg::WindowCfg& windowCfg, const vk::SurfaceKHR& surface) const;

    private:
        class GLFWInitObject
        {
        public:
            GLFWInitObject();
            GLFWInitObject(const GLFWInitObject&) = delete;
            GLFWInitObject(GLFWInitObject&&) = delete;
            const GLFWInitObject& operator=(const GLFWInitObject&) = delete;
            const GLFWInitObject& operator=(GLFWInitObject&&) = delete;
            ~GLFWInitObject();
        };

        GLFWInitObject m_forceGLFWInit;

        /** Holds the applications instance. */
        static ApplicationBase* m_instance;

        /** Holds the configuration file name. */
        std::string m_configFileName;
        /** Holds the configuration for this application. */
        cfg::Configuration m_config;
        /** Holds the windows. */
        std::vector<VKWindow> m_windows;

        // application status
        /** <c>true</c> if application is paused. */
        bool m_pause;
        /**  <c>true</c> if the application has stopped (i.e. the last scene has finished). */
        bool m_stopped;
        /** The (global) time of the application. */
        double m_currentTime;
        /** Time elapsed in the frame. */
        double m_elapsedTime;
        /** Hold whether GUI mode is switched on. */
        bool m_guiMode = true;

        /** Holds the scene object manager. */
        // SceneObjectManager m_sceneObjectManager;

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
        virtual void RenderGUI(const VKWindow* window) = 0;

    private:
        void InitVulkan(const std::string_view& applicationName, std::uint32_t applicationVersion,
                        const std::vector<std::string>& requiredInstanceExtensions);
        static unsigned int ScorePhysicalDevice(const vk::PhysicalDevice& device);
        static bool CheckDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<std::string>& requiredExtensions);
        std::unique_ptr<gfx::LogicalDevice>
        CreateLogicalDevice(const cfg::WindowCfg& windowCfg, const std::vector<std::string>& requiredDeviceExtensions,
                            void* featuresNextChain, const vk::SurfaceKHR& surface,
                            const function_view<bool(const vk::PhysicalDevice&)>& additionalDeviceChecks) const;
        [[nodiscard]] PFN_vkVoidFunction LoadVKInstanceFunction(const std::string& functionName,
                                                                const std::string& extensionName,
                                                                bool mandatory = false) const;

        static void CheckVKInstanceExtensions(const std::vector<const char*>& enabledExtensions);
        void CheckVKInstanceLayers();

        /** Holds the Vulkan validation layers. */
        std::vector<const char*> m_vkValidationLayers;
        /** Holds the Vulkan instance. */
        vk::UniqueInstance m_vkInstance;
        /** Holds the dispatch loader for the instance. */
        // vk::DispatchLoaderDynamic vkDispatchLoaderInst_;
        /** Holds the debug report callback. */
        vk::UniqueHandle<vk::DebugReportCallbackEXT, vk::DispatchLoaderDynamic> m_vkDebugReportCB;
        /** Holds the physical devices. */
        std::map<unsigned int, vk::PhysicalDevice> m_vkPhysicalDevices;
        /** Holds the physical device. */
        //vk::PhysicalDevice vkPhysicalDevice_;
        /** Holds the logical device. */
        //vk::Device vkDevice_;
        /** Holds the graphics queue. */
        //vk::Queue vkGraphicsQueue_;


    };
}
