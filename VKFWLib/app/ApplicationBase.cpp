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

namespace vku {


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
    ApplicationBase::ApplicationBase(const std::string& mainWindowTitle, cfg::Configuration& config, const glm::vec3& camPos) :
        pause_(true),
        stopped_(false),
        currentTime_(0.0),
        elapsedTime_(0.0)
    {
        mainWin.RegisterApplication(*this);
        mainWin.ShowWindow();
        glm::vec2 screenSize(mainWin.GetClientSize());
    }

    ApplicationBase::~ApplicationBase() = default;

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
        return !stopped_ && !mainWin.IsClosing();
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

        if (!this->pause_) {
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
        std::vector<const char*> enabledExtensions;
        std::vector<const char*> validationLayers;
        unsigned int glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (auto i = 0; i < glfwExtensionCount; ++i) enabledExtensions.push_back(glfwExtensions[i]);

#ifndef NDEBUG
        enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

        vk::ApplicationInfo appInfo{ applicationName.c_str(), applicationVersion, engineName, engineVersion, VK_API_VERSION_1_0 };
        vk::InstanceCreateInfo createInfo{vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
            static_cast<uint32_t>(enabledExtensions.size()), enabledExtensions.data() };

        // TODO: RAII object... [10/18/2016 Sebastian Maisch]
        vkInstance = vk::createInstance(createInfo);
    }
}
