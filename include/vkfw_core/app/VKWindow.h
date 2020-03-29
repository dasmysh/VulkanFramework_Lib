/**
 * @file   VKWindow.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.17
 *
 * @brief  Windows declaration for a vulkan window.
 */

#pragma once

#include "main.h"

#include <glm/vec2.hpp>

struct GLFWwindow;

namespace vkfw_core::gfx {
    class Framebuffer;
    class LogicalDevice;
}

struct ImGui_ImplVulkanH_WindowData;
struct ImGui_GLFWWindow;
struct ImGui_ImplVulkan_InitInfo;

namespace vkfw_core {

    class ApplicationBase;

    class VKWindow final
    {
    public:
        explicit VKWindow(cfg::WindowCfg& conf, bool useGUI);
        VKWindow(const VKWindow&) = delete;
        VKWindow(VKWindow&&) noexcept;
        VKWindow& operator=(const VKWindow&) = delete;
        VKWindow& operator=(VKWindow&&) noexcept;
        ~VKWindow() noexcept;

        [[nodiscard]] bool IsFocused() const { return focused_; }
        [[nodiscard]] bool IsClosing() const;
        void ShowWindow() const;
        void CloseWindow() const;
        [[nodiscard]] bool MessageBoxQuestion(const std::string& title, const std::string& content) const;

        [[nodiscard]] cfg::WindowCfg& GetConfig() const { return *config_; };
        [[nodiscard]] gfx::LogicalDevice& GetDevice() const { return *logicalDevice_; }
        [[nodiscard]] const std::vector<gfx::Framebuffer>& GetFramebuffers() const { return swapchainFramebuffers_; }
        [[nodiscard]] vk::RenderPass GetRenderPass() const { return *vkSwapchainRenderPass_; }

        [[nodiscard]] bool IsMouseButtonPressed(int button) const;
        [[nodiscard]] bool IsKeyPressed(int key) const;
        [[nodiscard]] glm::vec2 GetMousePosition() const { return currMousePosition_; }
        [[nodiscard]] glm::vec2 GetMousePositionNormalized() const { return currMousePositionNormalized_; }

        /** Returns the windows width. */
        [[nodiscard]] unsigned int GetWidth() const { return vkSurfaceExtend_.width; }
        /** Returns the windows height. */
        [[nodiscard]] unsigned int GetHeight() const { return vkSurfaceExtend_.height; }
        /** Returns the windows client size. */
        [[nodiscard]] glm::vec2 GetClientSize() const
        {
            return glm::vec2(static_cast<float>(vkSurfaceExtend_.width), static_cast<float>(vkSurfaceExtend_.height));
        }


        void PrepareFrame();
        // TODO: submit other command buffers to queue. [10/26/2016 Sebastian Maisch]
        void DrawCurrentCommandBuffer() const;
        void SubmitFrame();

        // for primary cmd buffer: dirty bit, update if needed. (start cmd buffer, begin render pass, execute other buffers, end pass, end buffer)
        void UpdatePrimaryCommandBuffers(const std::function<void(const vk::CommandBuffer& commandBuffer, std::size_t cmdBufferIndex)>& fillFunc) const;

        [[nodiscard]] std::uint32_t GetCurrentlyRenderedImageIndex() const { return currentlyRenderedImage_; }
        [[nodiscard]] vk::Semaphore GetDataAvailableSemaphore() const { return *vkDataAvailableSemaphore_; }

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

        void SetMousePosition(double xpos, double ypos);

        /** Holds the GLFW window. */
        GLFWwindow* window_;
        /** Holds the configuration for this window. */
        cfg::WindowCfg* config_;

        /** Holds the Vulkan surface. */
        vk::UniqueSurfaceKHR vkSurface_;
        /** Holds the size of the surface. */
        vk::Extent2D vkSurfaceExtend_;
        /** Holds the logical device. */
        std::unique_ptr<gfx::LogicalDevice> logicalDevice_;
        /** Holds the queue number used for graphics output. */
        unsigned int graphicsQueue_ = 0;
        /** Holds the swap chain. */
        vk::UniqueSwapchainKHR vkSwapchain_;
        /** Holds the swap chain render pass. */
        vk::UniqueRenderPass vkSwapchainRenderPass_;
        /** Render pass for ImGui. */
        vk::UniqueRenderPass vkImGuiRenderPass_;
        /** Holds the swap chain frame buffers. */
        std::vector<gfx::Framebuffer> swapchainFramebuffers_;
        /** Command pools for the swap chain cmd buffers (later: not only primary). */
        std::vector<vk::UniqueCommandPool> vkCommandPools_;
        /** Holds the swap chain command buffers. */
        std::vector<vk::UniqueCommandBuffer> vkCommandBuffers_;
        /** Command pools for the ImGui cmd buffers. */
        std::vector<vk::UniqueCommandPool> vkImGuiCommandPools_;
        /** Holds the command buffers for ImGui. */
        std::vector<vk::UniqueCommandBuffer> vkImGuiCommandBuffers_;
        /** Hold a fence for each command buffer to signal it is processed. */
        std::vector<vk::UniqueFence> vkCmdBufferUFences_;
        std::vector<vk::Fence> vkCmdBufferFences_;
        /** Holds the semaphore to notify when a new swap image is available. */
        vk::UniqueSemaphore vkImageAvailableSemaphore_;
        /** Holds the semaphore to notify when the data for that frame is uploaded to the GPU. */
        vk::UniqueSemaphore vkDataAvailableSemaphore_;
        /** Holds the semaphore to notify when rendering is finished. */
        vk::UniqueSemaphore vkRenderingFinishedSemaphore_;
        /** Holds the currently rendered image. */
        std::uint32_t currentlyRenderedImage_ = 0;

        /** The descriptor pool for ImGUI. */
        vk::UniqueDescriptorPool vkImguiDescPool_;
        /** ImGui window data. */
        std::unique_ptr<ImGui_ImplVulkanH_WindowData> windowData_;
        /** ImGui GLFW window data. */
        ImGui_GLFWWindow* glfwWindowData_ = nullptr;
        /** ImGui vulkan data. */
        std::unique_ptr<ImGui_ImplVulkan_InitInfo> imguiVulkanData_;


        /** Holds the current mouse position. */
        glm::vec2 currMousePosition_;
        /** Holds the current normalized (in [-1,1]) mouse position. */
        glm::vec2 currMousePositionNormalized_ = glm::vec2{0.0f};
        /** Holds the last mouse position. */
        glm::vec2 prevMousePosition_;
        /** Holds the relative mouse position. */
        glm::vec2 relativeMousePosition_;
        /** Holds whether the mouse is inside the client window. */
        bool mouseInWindow_;

        // window status
        /** <c>true</c> if the framebuffer was resized but resize was not handled. */
        bool frameBufferResize_ = false;
        /** <c>true</c> if minimized. */
        bool minimized_;
        /** <c>true</c> if maximized. */
        bool maximized_;
        /** Holds whether the window is in focus. */
        bool focused_ = false;
        /** The number (id) of the current frame. */
        std::uint64_t frameCount_;

        void InitWindow();
        void InitVulkan();
        void InitGUI();
        void RecreateSwapChain();
        void DestroySwapchainImages();
        void ReleaseWindow();
        void ReleaseVulkan();

        [[nodiscard]] std::pair<unsigned int, vk::Format> FindSupportedDepthFormat() const;

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
