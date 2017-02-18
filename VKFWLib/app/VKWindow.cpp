/**
 * @file   VKWindow.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.17
 *
 * @brief  Windows implementation for the VKWindow.
 */

#include "VKWindow.h"
#include "ApplicationBase.h"
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#undef min
#undef max

#include <vulkan/vulkan.hpp>
#include <gfx/vk/LogicalDevice.h>
#include "gfx/vk/Framebuffer.h"

namespace vku {

    /**
     * Creates a new windows VKWindow.
     * @param conf the window configuration used
     */
    VKWindow::VKWindow(cfg::WindowCfg& conf) :
        window_{ nullptr },
        config_(&conf),
        currMousePosition_(0.0f),
        prevMousePosition_(0.0f),
        relativeMousePosition_(0.0f),
        mouseInWindow_(true),
        minimized_(false),
        maximized_(conf.fullscreen_),
        frameCount_(0)
    {
        this->InitWindow();
        this->InitVulkan();
    }

    VKWindow::VKWindow(VKWindow&& rhs) noexcept :
        window_{ std::move(rhs.window_) },
        config_{ std::move(rhs.config_) },
        vkSurface_{ std::move(rhs.vkSurface_) },
        vkSurfaceExtend_{ std::move(rhs.vkSurfaceExtend_) },
        logicalDevice_{ std::move(rhs.logicalDevice_) },
        graphicsQueue_{ std::move(rhs.graphicsQueue_) },
        vkSwapchain_{ std::move(rhs.vkSwapchain_) },
        //vkSwapchainImages_{ std::move(rhs.vkSwapchainImages_) },
        //vkSwapchainImageViews_{ std::move(rhs.vkSwapchainImageViews_) },
        vkSwapchainRenderPass_{ std::move(rhs.vkSwapchainRenderPass_) },
        swapchainFramebuffers_{ std::move(rhs.swapchainFramebuffers_) },
        //vkSwapchainFrameBuffers_{ std::move(rhs.vkSwapchainFrameBuffers_) },
        vkCommandBuffers_{ std::move(rhs.vkCommandBuffers_) },
        vkImageAvailableSemaphore_{ std::move(rhs.vkImageAvailableSemaphore_) },
        vkRenderingFinishedSemaphore_{ std::move(rhs.vkRenderingFinishedSemaphore_) },
        currentlyRenderedImage_{ std::move(rhs.currentlyRenderedImage_) },
        currMousePosition_{ std::move(rhs.currMousePosition_) },
        prevMousePosition_{ std::move(rhs.prevMousePosition_) },
        relativeMousePosition_{ std::move(rhs.relativeMousePosition_) },
        mouseInWindow_{ std::move(rhs.mouseInWindow_) },
        minimized_{ std::move(rhs.minimized_) },
        maximized_{ std::move(rhs.maximized_) },
        focused_{ std::move(rhs.focused_) },
        frameCount_{ std::move(rhs.frameCount_) }
    {
        rhs.window_ = nullptr;
        rhs.vkSurface_ = vk::SurfaceKHR();
        rhs.vkSwapchain_ = vk::SwapchainKHR();
        rhs.vkSwapchainRenderPass_ = vk::RenderPass();
        rhs.vkImageAvailableSemaphore_ = vk::Semaphore();
        rhs.vkRenderingFinishedSemaphore_ = vk::Semaphore();
    }

    VKWindow& VKWindow::operator=(VKWindow&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~VKWindow();
            window_ = std::move(rhs.window_);
            config_ = std::move(rhs.config_);
            vkSurface_ = std::move(rhs.vkSurface_);
            vkSurfaceExtend_ = std::move(rhs.vkSurfaceExtend_);
            logicalDevice_ = std::move(rhs.logicalDevice_);
            graphicsQueue_ = std::move(rhs.graphicsQueue_);
            vkSwapchain_ = std::move(rhs.vkSwapchain_);
            //vkSwapchainImages_ = std::move(rhs.vkSwapchainImages_);
            //vkSwapchainImageViews_ = std::move(rhs.vkSwapchainImageViews_);
            vkSwapchainRenderPass_ = std::move(rhs.vkSwapchainRenderPass_);
            swapchainFramebuffers_ = std::move(rhs.swapchainFramebuffers_);
            //vkSwapchainFrameBuffers_ = std::move(rhs.vkSwapchainFrameBuffers_);
            vkCommandBuffers_ = std::move(rhs.vkCommandBuffers_);
            vkImageAvailableSemaphore_ = std::move(rhs.vkImageAvailableSemaphore_);
            vkRenderingFinishedSemaphore_ = std::move(rhs.vkRenderingFinishedSemaphore_);
            currentlyRenderedImage_ = std::move(rhs.currentlyRenderedImage_);
            currMousePosition_ = std::move(rhs.currMousePosition_);
            prevMousePosition_ = std::move(rhs.prevMousePosition_);
            relativeMousePosition_ = std::move(rhs.relativeMousePosition_);
            mouseInWindow_ = std::move(rhs.mouseInWindow_);
            minimized_ = std::move(rhs.minimized_);
            maximized_ = std::move(rhs.maximized_);
            focused_ = std::move(rhs.focused_);
            frameCount_ = std::move(rhs.frameCount_);

            rhs.window_ = nullptr;
            rhs.vkSurface_ = vk::SurfaceKHR();
            rhs.vkSwapchain_ = vk::SwapchainKHR();
            rhs.vkSwapchainRenderPass_ = vk::RenderPass();
            rhs.vkImageAvailableSemaphore_ = vk::Semaphore();
            rhs.vkRenderingFinishedSemaphore_ = vk::Semaphore();
        }
        return *this;
    }

    VKWindow::~VKWindow()
    {
        this->ReleaseVulkan();
        this->ReleaseWindow();
        config_->fullscreen_ = maximized_;
        // TODO: use frame buffer object [10/26/2016 Sebastian Maisch]
        config_->windowWidth_ = vkSurfaceExtend_.width;
        config_->windowHeight_ = vkSurfaceExtend_.height;
    }

    bool VKWindow::IsClosing() const
    {
        return glfwWindowShouldClose(window_) == GLFW_TRUE;
    }

    /**
     * Initializes the window.
     */
    void VKWindow::InitWindow()
    {
        LOG(INFO) << "Creating window '" << config_->windowTitle_ << "'.";
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwSetErrorCallback(VKWindow::glfwErrorCallback);

        if (config_->fullscreen_) {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            window_ = glfwCreateWindow(config_->windowWidth_, config_->windowHeight_, config_->windowTitle_.c_str(), glfwGetPrimaryMonitor(), nullptr);
            if (window_ == nullptr) {
                LOG(FATAL) << "Could not create window!";
                glfwTerminate();
                throw std::runtime_error("Could not create window!");
            }
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
            window_ = glfwCreateWindow(config_->windowWidth_, config_->windowHeight_, config_->windowTitle_.c_str(), nullptr, nullptr);
            if (window_ == nullptr) {
                LOG(FATAL) << "Could not create window!";
                glfwTerminate();
                throw std::runtime_error("Could not create window!");
            }
            glfwSetWindowPos(window_, config_->windowLeft_, config_->windowTop_);
            
        }
        glfwSetWindowUserPointer(window_, this);
        glfwSetInputMode(window_, GLFW_STICKY_MOUSE_BUTTONS, 1);
        glfwSetCursorPos(window_, 0.0, 0.0);

        glfwSetWindowPosCallback(window_, VKWindow::glfwWindowPosCallback);
        glfwSetWindowSizeCallback(window_, VKWindow::glfwWindowSizeCallback);
        glfwSetWindowFocusCallback(window_, VKWindow::glfwWindowFocusCallback);
        glfwSetWindowCloseCallback(window_, VKWindow::glfwWindowCloseCallback);
        glfwSetFramebufferSizeCallback(window_, VKWindow::glfwFramebufferSizeCallback);
        glfwSetWindowIconifyCallback(window_, VKWindow::glfwWindowIconifyCallback);


        glfwSetMouseButtonCallback(window_, VKWindow::glfwMouseButtonCallback);
        glfwSetCursorPosCallback(window_, VKWindow::glfwCursorPosCallback);
        glfwSetCursorEnterCallback(window_, VKWindow::glfwCursorEnterCallback);
        glfwSetScrollCallback(window_, VKWindow::glfwScrollCallback);
        glfwSetKeyCallback(window_, VKWindow::glfwKeyCallback);
        glfwSetCharCallback(window_, VKWindow::glfwCharCallback);
        glfwSetCharModsCallback(window_, VKWindow::glfwCharModsCallback);
        glfwSetDropCallback(window_, VKWindow::glfwDropCallback);

        // Check for Valid Context
        if (window_ == nullptr) {
            LOG(FATAL) << "Could not create window!";
            glfwTerminate();
            throw std::runtime_error("Could not create window!");
        }

        LOG(INFO) << "Window successfully initialized.";
    }

    /**
     * Initializes OpenGL.
     */
    void VKWindow::InitVulkan()
    {
        LOG(INFO) << "Initializing Vulkan surface...";

        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;
        auto result = glfwCreateWindowSurface(ApplicationBase::instance().GetVKInstance(), window_, nullptr, &surfaceKHR);
        if (result != VK_SUCCESS) {
            LOG(FATAL) << "Could not create window surface (" << vk::to_string(vk::Result(result)) << ").";
            throw std::runtime_error("Could not create window surface.");
        }
        vkSurface_ = vk::SurfaceKHR(surfaceKHR);
        logicalDevice_ = ApplicationBase::instance().CreateLogicalDevice(*config_, vkSurface_);
        for (auto i = 0U; i < config_->queues_.size(); ++i) if (config_->queues_[i].graphics_) {
            graphicsQueue_ = i;
            break;
        }

        RecreateSwapChain();

        vk::SemaphoreCreateInfo semaphoreInfo{ };
        vkImageAvailableSemaphore_ = logicalDevice_->GetDevice().createSemaphore(semaphoreInfo);
        vkDataAvailableSemaphore_ = logicalDevice_->GetDevice().createSemaphore(semaphoreInfo);
        vkRenderingFinishedSemaphore_ = logicalDevice_->GetDevice().createSemaphore(semaphoreInfo);

        LOG(INFO) << "Initializing Vulkan surface... done.";

        // fbo.Resize(config_->windowWidth_, config_->windowHeight_);

        // TODO ImGui_ImplGlfwGL3_Init(window_, false);
    }

    void VKWindow::RecreateSwapChain()
    {
        logicalDevice_->GetDevice().waitIdle();

        if (vkCommandBuffers_.size() > 0) logicalDevice_->GetDevice().freeCommandBuffers(logicalDevice_->GetCommandPool(graphicsQueue_),
            static_cast<uint32_t>(vkCommandBuffers_.size()), vkCommandBuffers_.data());
        for (auto& fence : vkCmdBufferFences_) {
            if(fence) logicalDevice_->GetDevice().destroyFence(fence);
            fence = vk::Fence();
        }

        DestroySwapchainImages();

        auto surfaceCapabilities = logicalDevice_->GetPhysicalDevice().getSurfaceCapabilitiesKHR(vkSurface_);
        auto deviceSurfaceFormats = logicalDevice_->GetPhysicalDevice().getSurfaceFormatsKHR(vkSurface_);
        auto surfaceFormats = cfg::GetVulkanSurfaceFormatsFromConfig(*config_);
        std::sort(deviceSurfaceFormats.begin(), deviceSurfaceFormats.end(), [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0.format < f1.format; });
        std::sort(surfaceFormats.begin(), surfaceFormats.end(), [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0.format < f1.format; });

        vk::SurfaceFormatKHR surfaceFormat;
        if (deviceSurfaceFormats.size() == 1 && deviceSurfaceFormats[0].format == vk::Format::eUndefined) surfaceFormat = surfaceFormats[0];
        else {
            std::vector<vk::SurfaceFormatKHR> formatIntersection;
            std::set_intersection(deviceSurfaceFormats.begin(), deviceSurfaceFormats.end(), surfaceFormats.begin(), surfaceFormats.end(),
                std::back_inserter(formatIntersection), [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0 == f1; });
            if (!formatIntersection.empty()) surfaceFormat = formatIntersection[0]; // no color space check here as color space does not depend on the color space flag but the actual format.
            else {
                LOG(FATAL) << "No suitable surface format found after correct enumeration (this should never happen).";
                throw std::runtime_error("No suitable surface format found after correct enumeration (this should never happen).");
            }
        }

        auto presentMode = cfg::GetVulkanPresentModeFromConfig(*config_);
        vkSurfaceExtend_ = vk::Extent2D{ static_cast<uint32_t>(config_->windowWidth_), static_cast<uint32_t>(config_->windowHeight_) };
        auto imageCount = surfaceCapabilities.minImageCount + cfg::GetVulkanAdditionalImageCountFromConfig(*config_);

        {
            auto oldSwapChain = vkSwapchain_;
            vk::SwapchainCreateInfoKHR swapChainCreateInfo{ vk::SwapchainCreateFlagsKHR(), vkSurface_, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
                vkSurfaceExtend_, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, surfaceCapabilities.currentTransform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, oldSwapChain };
            auto newSwapChain = logicalDevice_->GetDevice().createSwapchainKHR(swapChainCreateInfo);
            if (oldSwapChain) logicalDevice_->GetDevice().destroySwapchainKHR(oldSwapChain);
            vkSwapchain_ = newSwapChain;
        }

        auto swapchainImages = logicalDevice_->GetDevice().getSwapchainImagesKHR(vkSwapchain_);

        {
            // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
            vk::AttachmentDescription colorAttachment{ vk::AttachmentDescriptionFlags(), surfaceFormat.format, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };

            vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
            vk::SubpassDescription subPass{ vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef, nullptr, nullptr, 0, nullptr };
            vk::RenderPassCreateInfo renderPassInfo{ vk::RenderPassCreateFlags(), 1, &colorAttachment, 1, &subPass, 0, nullptr };

            vkSwapchainRenderPass_ = logicalDevice_->GetDevice().createRenderPass(renderPassInfo);
        }

        // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
        gfx::FramebufferDescriptor fbDesc;
        fbDesc.tex_.emplace_back(config_->backbufferBits_ / 8, surfaceFormat.format, vk::SampleCountFlagBits::e1);
        swapchainFramebuffers_.reserve(swapchainImages.size());
        for (auto i = 0U; i < swapchainImages.size(); ++i) {
            std::vector<vk::Image> attachments{ swapchainImages[i] };
            swapchainFramebuffers_.emplace_back(logicalDevice_.get(), glm::uvec2(vkSurfaceExtend_.width, vkSurfaceExtend_.height), attachments, vkSwapchainRenderPass_, fbDesc);
        }

        vkCommandBuffers_.resize(swapchainImages.size());
        vk::CommandBufferAllocateInfo allocInfo{ logicalDevice_->GetCommandPool(graphicsQueue_), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(vkCommandBuffers_.size()) };

        auto result = logicalDevice_->GetDevice().allocateCommandBuffers(&allocInfo, vkCommandBuffers_.data());
        if (result != vk::Result::eSuccess) {
            LOG(FATAL) << "Could not allocate command buffers(" << vk::to_string(result) << ").";
            throw std::runtime_error("Could not allocate command buffers.");
        }

        vk::FenceCreateInfo fenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled };
        vkCmdBufferFences_.resize(vkCommandBuffers_.size());
        for (auto& fence : vkCmdBufferFences_) fence = logicalDevice_->GetDevice().createFence(fenceCreateInfo);
    }

    void VKWindow::DestroySwapchainImages()
    {
        swapchainFramebuffers_.clear();

        if (vkSwapchainRenderPass_) logicalDevice_->GetDevice().destroyRenderPass(vkSwapchainRenderPass_);
        vkSwapchainRenderPass_ = vk::RenderPass();
    }

    void VKWindow::ReleaseVulkan()
    {
        logicalDevice_->GetDevice().waitIdle();

        //TODO ImGui_ImplGlfwGL3_Shutdown();

        for (auto& fence : vkCmdBufferFences_) {
            if (fence) logicalDevice_->GetDevice().destroyFence(fence);
            fence = vk::Fence();
        }

        if (vkImageAvailableSemaphore_) logicalDevice_->GetDevice().destroySemaphore(vkImageAvailableSemaphore_);
        vkImageAvailableSemaphore_ = vk::Semaphore();
        if (vkDataAvailableSemaphore_) logicalDevice_->GetDevice().destroySemaphore(vkDataAvailableSemaphore_);
        vkDataAvailableSemaphore_ = vk::Semaphore();
        if (vkRenderingFinishedSemaphore_) logicalDevice_->GetDevice().destroySemaphore(vkRenderingFinishedSemaphore_);
        vkRenderingFinishedSemaphore_ = vk::Semaphore();

        DestroySwapchainImages();
        if (vkSwapchain_) logicalDevice_->GetDevice().destroySwapchainKHR(vkSwapchain_);
        vkSwapchain_ = vk::SwapchainKHR();
        logicalDevice_.reset();
        if (vkSurface_) ApplicationBase::instance().GetVKInstance().destroySurfaceKHR(vkSurface_);
        vkSurface_ = vk::SurfaceKHR();
    }

    /**
     * Shows the window.
     */
    void VKWindow::ShowWindow() const
    {
        glfwShowWindow(window_);
    }

    /**
     *  Closes the window.
     */
    void VKWindow::CloseWindow() const
    {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }

    void VKWindow::ReleaseWindow()
    {
        if (window_) glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    void VKWindow::PrepareFrame()
    {
        auto result = logicalDevice_->GetDevice().acquireNextImageKHR(vkSwapchain_, std::numeric_limits<uint64_t>::max(), vkImageAvailableSemaphore_, vk::Fence());
        currentlyRenderedImage_ = result.value;

        if (result.result == vk::Result::eErrorOutOfDateKHR) {
            RecreateSwapChain();
            return;
        }
        if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
            LOG(FATAL) << "Could not acquire swap chain image (" << vk::to_string(result.result) << ").";
            throw std::runtime_error("Could not acquire swap chain image.");
        }
    }

    void VKWindow::DrawCurrentCommandBuffer() const
    {
        vk::PipelineStageFlags waitStages[]{ vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTopOfPipe };
        vk::Semaphore waitSemaphores[]{ vkImageAvailableSemaphore_, vkDataAvailableSemaphore_ };
        vk::SubmitInfo submitInfo{ 2, waitSemaphores, waitStages, 1, &vkCommandBuffers_[currentlyRenderedImage_], 1, &vkRenderingFinishedSemaphore_ };

        {
            auto syncResult = logicalDevice_->GetDevice().getFenceStatus(vkCmdBufferFences_[currentlyRenderedImage_]);
            while (syncResult == vk::Result::eTimeout || syncResult == vk::Result::eNotReady)
                syncResult = logicalDevice_->GetDevice().waitForFences(vkCmdBufferFences_[currentlyRenderedImage_], VK_TRUE, defaultFenceTimeout);

            if (syncResult != vk::Result::eSuccess) {
                LOG(FATAL) << "Error synchronizing command buffer. (" << vk::to_string(syncResult) << ").";
                throw std::runtime_error("Error synchronizing command buffer.");
            }

            logicalDevice_->GetDevice().resetFences(vkCmdBufferFences_[currentlyRenderedImage_]);
        }

        logicalDevice_->GetQueue(graphicsQueue_, 0).submit(submitInfo, vkCmdBufferFences_[currentlyRenderedImage_]);
    }

    void VKWindow::SubmitFrame()
    {
        vk::SwapchainKHR swapchains[] = { vkSwapchain_ };
        vk::PresentInfoKHR presentInfo{ 1, &vkRenderingFinishedSemaphore_, 1, swapchains, &currentlyRenderedImage_ }; //<- wait on these semaphores
        auto result = logicalDevice_->GetQueue(graphicsQueue_, 0).presentKHR(presentInfo);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            RecreateSwapChain();
        }
        else if (result != vk::Result::eSuccess) {
            LOG(FATAL) << "Could not present swap chain image (" << vk::to_string(result) << ").";
            throw std::runtime_error("Could not present swap chain image.");
        }

        ++frameCount_;
    }

    void VKWindow::UpdatePrimaryCommandBuffers(const std::function<void(const vk::CommandBuffer& commandBuffer, uint32_t cmdBufferIndex)>& fillFunc) const
    {
        {
            auto syncResult = vk::Result::eTimeout;
            while (syncResult == vk::Result::eTimeout)
                syncResult = logicalDevice_->GetDevice().waitForFences(vkCmdBufferFences_, VK_TRUE, defaultFenceTimeout);

            if (syncResult != vk::Result::eSuccess) {
                LOG(FATAL) << "Error synchronizing command buffers. (" << vk::to_string(syncResult) << ").";
                throw std::runtime_error("Error synchronizing command buffers.");
            }
        }

        logicalDevice_->GetDevice().resetCommandPool(logicalDevice_->GetCommandPool(graphicsQueue_), vk::CommandPoolResetFlags());
        for (auto i = 0U; i < vkCommandBuffers_.size(); ++i) {
            vk::CommandBufferBeginInfo cmdBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            vkCommandBuffers_[i].begin(cmdBufferBeginInfo);

            std::array<vk::ClearValue, 2> clearColor;
            clearColor[0].setColor(vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } });
            clearColor[1].setDepthStencil(vk::ClearDepthStencilValue{ 0.0f, 0 });
            vk::RenderPassBeginInfo renderPassBeginInfo{ vkSwapchainRenderPass_, swapchainFramebuffers_[i].GetFramebuffer(),
                vk::Rect2D(vk::Offset2D(0, 0), vkSurfaceExtend_), 1, clearColor.data() };
            vkCommandBuffers_[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

            fillFunc(vkCommandBuffers_[i], i);

            vkCommandBuffers_[i].endRenderPass();
            vkCommandBuffers_[i].end();
        }
    }

    /**
     * Shows a question message box.
     * @param title the message boxes title
     * @param content the message boxes content
     * @return returns <code>true</code> if the user pressed 'yes' <code>false</code> if 'no'
     */
    bool VKWindow::MessageBoxQuestion(const std::string& title, const std::string& content) const
    {
        return MessageBoxA(glfwGetWin32Window(window_), content.c_str(), title.c_str(), MB_YESNO) == IDYES;
    }

    bool VKWindow::IsMouseButtonPressed(int button) const
    {
        return glfwGetMouseButton(window_, button) == GLFW_PRESS;
    }

    bool VKWindow::IsKeyPressed(int key) const
    {
        return glfwGetKey(window_, key) == GLFW_PRESS;
    }

    void VKWindow::WindowPosCallback(int xpos, int ypos) const
    {
        config_->windowLeft_ = xpos;
        config_->windowTop_ = ypos;
    }

    void VKWindow::WindowSizeCallback(int width, int height)
    {
        LOG(INFO) << "Got window resize event (" << width << ", " << height << ") ...";
        if (width == 0 || height == 0) return;

        LOG(DEBUG) << "Begin HandleResize()";

        config_->windowWidth_ = width;
        config_->windowHeight_ = height;

        RecreateSwapChain();

        // TODO: resize external frame buffer object?
        // this->Resize(width, height);

        try {
            // TODO: notify all resources depending on this...
            ApplicationBase::instance().OnResize(width, height, this);
        }
        catch (std::runtime_error e) {
            LOG(FATAL) << "Could not reacquire resources after resize: " << e.what();
            throw std::runtime_error("Could not reacquire resources after resize.");
        }
    }

    void VKWindow::WindowFocusCallback(int focused)
    {
        if (focused == GLFW_TRUE) focused_ = true;
        else focused_ = false;
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void VKWindow::WindowCloseCallback() const
    {
        LOG(INFO) << "Got close event ...";
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void VKWindow::FramebufferSizeCallback(int width, int height) const
    {
        LOG(INFO) << "Got framebuffer resize event (" << width << ", " << height << ") ...";
    }

    void VKWindow::WindowIconifyCallback(int iconified)
    {
        if (iconified == GLFW_TRUE) {
            ApplicationBase::instance().SetPause(true);
            minimized_ = true;
            maximized_ = false;
        } else {
            if (minimized_) ApplicationBase::instance().SetPause(false);
            minimized_ = false;
            maximized_ = false;
        }
    }

    void VKWindow::MouseButtonCallback(int button, int action, int mods)
    {
        if (mouseInWindow_) {
            ApplicationBase::instance().HandleMouse(button, action, mods, 0.0f, this);
        }
    }

    void VKWindow::CursorPosCallback(double xpos, double ypos)
    {
        if (mouseInWindow_) {
            prevMousePosition_ = currMousePosition_;
            currMousePosition_ = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
            relativeMousePosition_ = currMousePosition_ - prevMousePosition_;

            ApplicationBase::instance().HandleMouse(-1, 0, 0, 0.0f, this);
        }
    }

    void VKWindow::CursorEnterCallback(int entered)
    {
        if (entered) {
            double xpos, ypos;
            glfwGetCursorPos(window_, &xpos, &ypos);
            currMousePosition_ = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
            mouseInWindow_ = true;
        } else {
            mouseInWindow_ = false;
        }
    }

    void VKWindow::ScrollCallback(double, double yoffset)
    {
        if (mouseInWindow_) {
            ApplicationBase::instance().HandleMouse(-1, 0, 0, 50.0f * static_cast<float>(yoffset), this);
        }
    }

    void VKWindow::KeyCallback(int key, int scancode, int action, int mods)
    {
        ApplicationBase::instance().HandleKeyboard(key, scancode, action, mods, this);
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void VKWindow::CharCallback(unsigned) const
    {
        // Not needed at this point... [4/7/2016 Sebastian Maisch]
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void VKWindow::CharModsCallback(unsigned, int) const
    {
        // Not needed at this point... [4/7/2016 Sebastian Maisch]
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void VKWindow::DropCallback(int, const char**) const
    {
        throw std::runtime_error("File dropping not implemented.");
    }

    void VKWindow::glfwErrorCallback(int error, const char* description)
    {
        LOG(FATAL) << "An GLFW error occurred (" << error << "): " << std::endl << description;
    }

    void VKWindow::glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->WindowPosCallback(xpos, ypos);
    }

    void VKWindow::glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->WindowSizeCallback(width, height);
    }

    void VKWindow::glfwWindowFocusCallback(GLFWwindow* window, int focused)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->WindowFocusCallback(focused);
    }

    void VKWindow::glfwWindowCloseCallback(GLFWwindow* window)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->WindowCloseCallback();
    }

    void VKWindow::glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->FramebufferSizeCallback(width, height);
    }

    void VKWindow::glfwWindowIconifyCallback(GLFWwindow* window, int iconified)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->WindowIconifyCallback(iconified);
    }

    void VKWindow::glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        //TODO ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);

        //auto& io = ImGui::GetIO();
        //if (!io.WantCaptureMouse) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->MouseButtonCallback(button, action, mods);
        //}
    }

    void VKWindow::glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        //TODO auto& io = ImGui::GetIO();
        //if (!io.WantCaptureMouse) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->CursorPosCallback(xpos, ypos);
        //}
    }

    void VKWindow::glfwCursorEnterCallback(GLFWwindow* window, int entered)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->CursorEnterCallback(entered);
    }

    void VKWindow::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        //TODO ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);

        //auto& io = ImGui::GetIO();
        //if (!io.WantCaptureMouse) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->ScrollCallback(xoffset, yoffset);
        //}
    }

    void VKWindow::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        //TODO ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);

        //auto& io = ImGui::GetIO();
        //if (!io.WantCaptureKeyboard) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->KeyCallback(key, scancode, action, mods);
        //}
    }

    void VKWindow::glfwCharCallback(GLFWwindow* window, unsigned codepoint)
    {
        //TODO ImGui_ImplGlfwGL3_CharCallback(window, codepoint);

        //TODO auto& io = ImGui::GetIO();
        //if (!io.WantCaptureKeyboard) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->CharCallback(codepoint);
        //}
    }

    void VKWindow::glfwCharModsCallback(GLFWwindow* window, unsigned codepoint, int mods)
    {
        //TODO auto& io = ImGui::GetIO();
        //if (!io.WantCaptureKeyboard) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->CharModsCallback(codepoint, mods);
        //}
    }

    void VKWindow::glfwDropCallback(GLFWwindow* window, int count, const char** paths)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->DropCallback(count, paths);
    }
}
