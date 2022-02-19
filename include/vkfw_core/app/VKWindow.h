/**
 * @file   VKWindow.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.17
 *
 * @brief  Windows declaration for a vulkan window.
 */

#pragma once

#include "main.h"
#include "gfx/vk/wrappers/RenderPass.h"
#include "gfx/vk/wrappers/Swapchain.h"
#include "gfx/vk/wrappers/VulkanSyncResources.h"
#include "gfx/vk/wrappers/CommandBuffer.h"
#include "gfx/vk/wrappers/DescriptorPool.h"

#include <glm/vec2.hpp>

#include <core/function_view.h>

struct GLFWwindow;

namespace vkfw_core::gfx {
    class Framebuffer;
    class LogicalDevice;
    class DescriptorSet;
    class VertexInputResources;
}

struct ImGui_ImplVulkanH_Window;
struct ImGui_ImplVulkan_InitInfo;

namespace vkfw_core {

    class ApplicationBase;

    class VKWindow final
    {
    public:
        explicit VKWindow(cfg::WindowCfg& conf, bool useGUI, const std::vector<std::string>& requiredDeviceExtensions,
                          void* deviceFeaturesNextChain);
        VKWindow(const VKWindow&) = delete;
        VKWindow(VKWindow&&) noexcept;
        VKWindow& operator=(const VKWindow&) = delete;
        VKWindow& operator=(VKWindow&&) noexcept;
        ~VKWindow() noexcept;

        [[nodiscard]] bool IsFocused() const { return m_focused; }
        [[nodiscard]] bool IsClosing() const;
        void ShowWindow() const;
        void CloseWindow() const;
        [[nodiscard]] bool MessageBoxQuestion(const std::string& title, const std::string& content) const;

        [[nodiscard]] cfg::WindowCfg& GetConfig() const { return *m_config; };
        [[nodiscard]] gfx::LogicalDevice& GetDevice() const { return *m_logicalDevice; }
        [[nodiscard]] const std::vector<gfx::Framebuffer>& GetFramebuffers() const { return m_swapchainFramebuffers; }
        [[nodiscard]] std::vector<gfx::Framebuffer>& GetFramebuffers() { return m_swapchainFramebuffers; }
        [[nodiscard]] const gfx::RenderPass& GetRenderPass() const { return m_mainRenderingRenderPass; }

        [[nodiscard]] bool IsMouseButtonPressed(int button) const;
        [[nodiscard]] bool IsKeyPressed(int key) const;
        [[nodiscard]] glm::vec2 GetMousePosition() const { return m_currMousePosition; }
        [[nodiscard]] glm::vec2 GetMousePositionNormalized() const { return m_currMousePositionNormalized; }

        /** Returns the windows width. */
        [[nodiscard]] unsigned int GetWidth() const { return m_vkSurfaceExtent.width; }
        /** Returns the windows height. */
        [[nodiscard]] unsigned int GetHeight() const { return m_vkSurfaceExtent.height; }
        /** Returns the windows client size. */
        [[nodiscard]] glm::vec2 GetClientSize() const
        {
            return glm::vec2(static_cast<float>(m_vkSurfaceExtent.width), static_cast<float>(m_vkSurfaceExtent.height));
        }


        void ForceResizeEvent() { m_frameBufferResize = true; }
        void PrepareFrame();
        // TODO: submit other command buffers to queue. [10/26/2016 Sebastian Maisch]
        void DrawCurrentCommandBuffer();
        void SubmitFrame();

        // for primary cmd buffer: dirty bit, update if needed. (start cmd buffer, end cmd buffer; render pass needs to be started and ended with BeginSwapchainRenderPass and EndSwapchainRenderpass.)
        void UpdatePrimaryCommandBuffers(const function_view<void(gfx::CommandBuffer& commandBuffer,
                                                                  std::size_t cmdBufferIndex)>& fillFunc);
        void BeginSwapchainRenderPass(std::size_t cmdBufferIndex, std::span<gfx::DescriptorSet*> descriptorSets,
                                      std::span<gfx::VertexInputResources*> vertexInputs);
        void EndSwapchainRenderPass(std::size_t cmdBufferIndex) const;

        [[nodiscard]] std::uint32_t GetCurrentlyRenderedImageIndex() const { return m_currentlyRenderedImage; }
        [[nodiscard]] const gfx::Semaphore& GetDataAvailableSemaphore() const { return m_dataAvailableSemaphore; }
        [[nodiscard]] const gfx::Semaphore& GetRenderingFinishedSemaphore() const { return m_renderingFinishedSemaphore; }

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
        GLFWwindow* m_window;
        /** Holds the configuration for this window. */
        cfg::WindowCfg* m_config;

        /** Holds the Vulkan surface. */
        gfx::Surface m_surface;
        /** Holds the size of the surface. */
        vk::Extent2D m_vkSurfaceExtent;
        /** Holds the logical device. */
        std::unique_ptr<gfx::LogicalDevice> m_logicalDevice;
        /** Holds the queue number used for graphics output. */
        unsigned int m_graphicsQueue = 0;
        /** Holds the swap chain. */
        gfx::Swapchain m_swapchain;
        /** Holds the swap chain render pass. */
        gfx::RenderPass m_mainRenderingRenderPass;
        /** Render pass for ImGui. */
        gfx::RenderPass m_imGuiRenderPass;
        /** Holds the swap chain frame buffers. */
        std::vector<gfx::Framebuffer> m_swapchainFramebuffers;
        /** Command pools for the swap chain cmd buffers (later: not only primary). */
        std::vector<gfx::CommandPool> m_commandPools;
        /** Holds the swap chain command buffers. */
        std::vector<gfx::CommandBuffer> m_commandBuffers;
        /** Command pools for the ImGui cmd buffers. */
        std::vector<gfx::CommandPool> m_imGuiCommandPools;
        /** Holds the command buffers for ImGui. */
        std::vector<gfx::CommandBuffer> m_imGuiCommandBuffers;
        /** Hold a fence for each command buffer to signal it is processed. */
        std::vector<gfx::Fence> m_cmdBufferFences;
        /** Holds the semaphore to notify when a new swap image is available. */
        gfx::Semaphore m_imageAvailableSemaphore;
        /** Holds the semaphore to notify when the data for that frame is uploaded to the GPU. */
        gfx::Semaphore m_dataAvailableSemaphore;
        /** Holds the semaphore to notify when rendering is finished. */
        gfx::Semaphore m_renderingFinishedSemaphore;
        /** Holds the currently rendered image. */
        std::uint32_t m_currentlyRenderedImage = 0;

        /** The descriptor pool for ImGUI. */
        gfx::DescriptorPool m_imguiDescPool;
        /** ImGui window data. */
        std::unique_ptr<ImGui_ImplVulkanH_Window> m_windowData;
        /** ImGui vulkan data. */
        std::unique_ptr<ImGui_ImplVulkan_InitInfo> m_imguiVulkanData;

        /** Tracks if ImGui is initialized. */
        bool m_imguiInitialized = false;
        /** Holds the current mouse position. */
        glm::vec2 m_currMousePosition;
        /** Holds the current normalized (in [-1,1]) mouse position. */
        glm::vec2 m_currMousePositionNormalized = glm::vec2{0.0f};
        /** Holds the last mouse position. */
        glm::vec2 m_prevMousePosition;
        /** Holds the relative mouse position. */
        glm::vec2 m_relativeMousePosition;
        /** Holds whether the mouse is inside the client window. */
        bool m_mouseInWindow;

        // window status
        /** <c>true</c> if the framebuffer was resized but resize was not handled. */
        bool m_frameBufferResize = false;
        /** <c>true</c> if minimized. */
        bool m_minimized;
        /** <c>true</c> if maximized. */
        bool m_maximized;
        /** Holds whether the window is in focus. */
        bool m_focused = false;
        /** The number (id) of the current frame. */
        std::uint64_t m_frameCount;

        void InitWindow();
        void InitVulkan(const std::vector<std::string>& requiredDeviceExtensions, void* featuresNextChain);
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
