/**
 * @file   ApplicationBase.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Declares the application base class.
 */

#pragma once

#include "main.h"
#include <vulkan/vulkan.hpp>

namespace vku {

    namespace cfg {
        class Configuration;
    }

    class VKWindow;

    class ApplicationBase
    {
    public:
        ApplicationBase(const std::string& applicationName, uint32_t applicationVersion, const std::string& configFileName);
        ApplicationBase(const ApplicationBase&) = delete;
        ApplicationBase(ApplicationBase&&) = delete;
        ApplicationBase& operator=(const ApplicationBase&) = delete;
        ApplicationBase& operator=(ApplicationBase&&) = delete;
        virtual ~ApplicationBase();

        static ApplicationBase& instance() { return *instance_; };

        /** Starts the application. */
        void StartRun();
        /** Checks if the application is still running. */
        bool IsRunning() const;
        /** Make one application <em>step</em> (rendering etc.). */
        void Step();
        /** Called if the application is to end running. */
        void EndRun();

        bool IsPaused() const { return pause_; }
        bool IsGUIMode() const { return guiMode_; }
        VKWindow* GetFocusedWindow();

        void SetPause(bool pause);

        virtual bool HandleKeyboard(int key, int scancode, int action, int mods, VKWindow* sender);
        bool HandleMouse(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender);
        virtual bool HandleMouseApp(int button, int action, int mods, float mouseWheelDelta, VKWindow* sender) = 0;
        void OnResize(unsigned int width, unsigned int height);
        virtual void Resize(const glm::uvec2& screenSize);

        const cfg::Configuration& GetConfig() const { return config_; };
        const vk::Instance& GetVKInstance() const { return vkInstance_; }
        const vk::PhysicalDevice& GetPhyicalDeviceForSurace(const vk::SurfaceKHR& surface) const;

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

    protected:
        /**
         * Moves a single frame.
         * @param time the time elapsed since the application started
         * @param elapsed the time elapsed since the last frame
         */
        virtual void FrameMove(float time, float elapsed) = 0;
        /** Render the scene. */
        virtual void RenderScene() = 0;
        /** Render the scenes GUI. */
        virtual void RenderGUI() = 0;

    private:
        void InitVulkan(const std::string& applicationName, uint32_t applicationVersion);
        static unsigned int ScorePhysicalDevice(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
        PFN_vkVoidFunction LoadVKInstanceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory = false) const;
        PFN_vkVoidFunction LoadVKDeviceFunction(const std::string& functionName, const std::string& extensionName, bool mandatory = false) const;

        static constexpr uint32_t NUM_GRAPHICS_QUEUES = 1;

        /** Holds the Vulkan instance. */
        vk::Instance vkInstance_;
        /** Holds the debug report callback. */
        vk::DebugReportCallbackEXT vkDebugReportCB_;
        /** Holds the physical devices. */
        std::vector<vk::PhysicalDevice> vkPhysicalDevices_;
        /** Holds the physical device. */
        //vk::PhysicalDevice vkPhysicalDevice_;
        /** Holds the logical device. */
        //vk::Device vkDevice_;
        /** Holds the graphics queue. */
        //vk::Queue vkGraphicsQueue_;


        /** Holds whether debug markers are enabled. */
        bool enableDebugMarkers_ = false;
    };
}
