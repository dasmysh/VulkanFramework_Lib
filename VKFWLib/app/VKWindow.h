/**
 * @file   VKWindow.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.17
 *
 * @brief  Windows declaration for a vulkan window.
 */

#pragma once

#include "main.h"

struct GLFWwindow;

namespace vku {
    namespace gfx{
        class LogicalDevice;
    }

    class ApplicationBase;

    class VKWindow
    {
    public:
        explicit VKWindow(cfg::WindowCfg& conf);
        VKWindow(const VKWindow&) = delete;
        VKWindow(VKWindow&&) noexcept;
        VKWindow& operator=(const VKWindow&) = delete;
        VKWindow& operator=(VKWindow&&) noexcept;
        virtual ~VKWindow();

        bool IsFocused() const { return focused_; }
        bool IsClosing() const;
        void ShowWindow() const;
        void CloseWindow() const;
        bool MessageBoxQuestion(const std::string& title, const std::string& content) const;

        cfg::WindowCfg& GetConfig() const { return *config_; };

        bool IsMouseButtonPressed(int button) const;
        bool IsKeyPressed(int key) const;
        glm::vec2 GetMousePosition() const { return currMousePosition_; }

        /** Returns the windows width. */
        unsigned int GetWidth() const { return vkSurfaceExtend_.width; }
        /** Returns the windows height. */
        unsigned int GetHeight() const { return vkSurfaceExtend_.height; }
        /** Returns the windows client size. */
        glm::vec2 GetClientSize() const { return glm::vec2(static_cast<float>(vkSurfaceExtend_.width), static_cast<float>(vkSurfaceExtend_.height)); }


        void PrepareFrame();
        // TODO: submit other command buffers to queue. [10/26/2016 Sebastian Maisch]
        void DrawCurrentCommandBuffer() const;
        void SubmitFrame();

        // for primary cmd buffer: dirty bit, update if needed. (start cmd buffer, begin render pass, execute other buffers, end pass, end buffer)
        void UpdatePrimaryCommandBuffers(const std::function<void(const vk::CommandBuffer& commandBuffer)>& fillFunc) const;

    private:
        void WindowPosCallback(int xpos, int ypos) const;
        void WindowSizeCallback(int width, int height);
        void WindowFocusCallback(int focused);
        void WindowCloseCallback() const;
        void FramebufferSizeCallback(int width, int height) const;
        void WindowIconifyCallback(int iconified);

        void MouseButtonCallback(int button, int action, int mods);
        void CursorPosCallback(double xpos, double ypos);
        void CursorEnterCallback(int entered);
        void ScrollCallback(double xoffset, double yoffset);
        void KeyCallback(int key, int scancode, int action, int mods);
        void CharCallback(unsigned int codepoint) const;
        void CharModsCallback(unsigned int codepoint, int mods) const;
        void DropCallback(int count, const char** paths) const;

        /** Holds the GLFW window. */
        GLFWwindow* window_;
        /** Holds the configuration for this window. */
        cfg::WindowCfg* config_;

        /** Holds the Vulkan surface. */
        vk::SurfaceKHR vkSurface_;
        /** Holds the size of the surface. */
        vk::Extent2D vkSurfaceExtend_;
        /** Holds the logical device. */
        std::unique_ptr<gfx::LogicalDevice> logicalDevice_;
        /** Holds the queue number used for graphics output. */
        unsigned int graphicsQueue_ = 0;
        /** Holds the swap chain. */
        vk::SwapchainKHR vkSwapchain_;
        /** Holds the swap chain images. */
        std::vector<vk::Image> vkSwapchainImages_;
        /** Holds the swap chain image views. */
        std::vector<vk::ImageView> vkSwapchainImageViews_;
        /** Holds the swap chain render pass. */
        vk::RenderPass vkSwapchainRenderPass_;
        /** Holds the swap chain frame buffers. */
        std::vector<vk::Framebuffer> vkSwapchainFrameBuffers_;
        /** Holds the swap chain command buffers. */
        std::vector<vk::CommandBuffer> vkCommandBuffers_;
        /** Holds the semaphore to notify when a new swap image is available. */
        vk::Semaphore vkImageAvailableSemaphore_;
        /** Holds the semaphore to notify when rendering is finished. */
        vk::Semaphore vkRenderingFinishedSemaphore_;
        /** Holds the currently rendered image. */
        uint32_t currentlyRenderedImage_ = 0;


        /** Holds the current mouse position. */
        glm::vec2 currMousePosition_;
        /** Holds the last mouse position. */
        glm::vec2 prevMousePosition_;
        /** Holds the relative mouse position. */
        glm::vec2 relativeMousePosition_;
        /** Holds whether the mouse is inside the client window. */
        bool mouseInWindow_;

        // window status
        /** <c>true</c> if minimized. */
        bool minimized_;
        /** <c>true</c> if maximized. */
        bool maximized_;
        /** Holds whether the window is in focus. */
        bool focused_;
        /** The number (id) of the current frame. */
        uint64_t frameCount_;

        void InitWindow();
        void InitVulkan();
        void RecreateSwapChain();
        void DestroySwapchainImages();
        void ReleaseWindow();
        void ReleaseVulkan();

    public:
        static void glfwErrorCallback(int error, const char* description);
        static void glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos);
        static void glfwWindowSizeCallback(GLFWwindow* window, int width, int height);
        static void glfwWindowFocusCallback(GLFWwindow* window, int focused);
        static void glfwWindowCloseCallback(GLFWwindow* window);
        static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void glfwWindowIconifyCallback(GLFWwindow* window, int iconified);

        static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void glfwCursorEnterCallback(GLFWwindow* window, int entered);
        static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void glfwCharCallback(GLFWwindow* window, unsigned int codepoint);
        static void glfwCharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
        static void glfwDropCallback(GLFWwindow* window, int count, const char** paths);
    };
}
