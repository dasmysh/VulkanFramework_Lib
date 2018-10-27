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

#include <vulkan/vulkan.hpp>
#include <gfx/vk/LogicalDevice.h>
#include "gfx/vk/Framebuffer.h"
#include "imgui.h"
#include "core/imgui/imgui_impl_glfw.h"
#include "core/imgui/imgui_impl_vulkan.h"

namespace vku {

    /**
     * Creates a new windows VKWindow.
     * @param conf the window configuration used
     */
    VKWindow::VKWindow(cfg::WindowCfg& conf, bool useGUI) :
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
        windowData_ = std::make_unique<ImGui_ImplVulkanH_WindowData>();
        this->InitWindow();
        this->InitVulkan();
        if (useGUI) InitGUI();
    }

    VKWindow::VKWindow(VKWindow&& rhs) noexcept :
        window_{ std::move(rhs.window_) },
        config_{ std::move(rhs.config_) },
        vkSurface_{ std::move(rhs.vkSurface_) },
        vkSurfaceExtend_{ std::move(rhs.vkSurfaceExtend_) },
        logicalDevice_{ std::move(rhs.logicalDevice_) },
        graphicsQueue_{ std::move(rhs.graphicsQueue_) },
        vkSwapchain_{ std::move(rhs.vkSwapchain_) },
        vkSwapchainRenderPass_{ std::move(rhs.vkSwapchainRenderPass_) },
        swapchainFramebuffers_{ std::move(rhs.swapchainFramebuffers_) },
        vkCommandPools_{ std::move(rhs.vkCommandPools_) },
        vkCommandBuffers_{ std::move(rhs.vkCommandBuffers_) },
        vkImGuiCommandPools_{ std::move(rhs.vkImGuiCommandPools_) },
        vkImGuiCommandBuffers_{ std::move(rhs.vkImGuiCommandBuffers_) },
        vkImageAvailableSemaphore_{ std::move(rhs.vkImageAvailableSemaphore_) },
        vkRenderingFinishedSemaphore_{ std::move(rhs.vkRenderingFinishedSemaphore_) },
        currentlyRenderedImage_{ std::move(rhs.currentlyRenderedImage_) },
        vkImguiDescPool_{ std::move(rhs.vkImguiDescPool_) },
        windowData_{ std::move(rhs.windowData_) },
        glfwWindowData_{ std::move(rhs.glfwWindowData_) },
        imguiVulkanData_{ std::move(rhs.imguiVulkanData_) },
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
        rhs.glfwWindowData_ = nullptr;
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
            vkSwapchainRenderPass_ = std::move(rhs.vkSwapchainRenderPass_);
            swapchainFramebuffers_ = std::move(rhs.swapchainFramebuffers_);
            vkCommandPools_ = std::move(rhs.vkCommandPools_);
            vkCommandBuffers_ = std::move(rhs.vkCommandBuffers_);
            vkImGuiCommandPools_ = std::move(rhs.vkImGuiCommandPools_);
            vkImGuiCommandBuffers_ = std::move(rhs.vkImGuiCommandBuffers_);
            vkImageAvailableSemaphore_ = std::move(rhs.vkImageAvailableSemaphore_);
            vkRenderingFinishedSemaphore_ = std::move(rhs.vkRenderingFinishedSemaphore_);
            currentlyRenderedImage_ = std::move(rhs.currentlyRenderedImage_);
            vkImguiDescPool_ = std::move(rhs.vkImguiDescPool_);
            windowData_ = std::move(rhs.windowData_);
            glfwWindowData_ = std::move(rhs.glfwWindowData_);
            imguiVulkanData_ = std::move(rhs.imguiVulkanData_);
            currMousePosition_ = std::move(rhs.currMousePosition_);
            prevMousePosition_ = std::move(rhs.prevMousePosition_);
            relativeMousePosition_ = std::move(rhs.relativeMousePosition_);
            mouseInWindow_ = std::move(rhs.mouseInWindow_);
            minimized_ = std::move(rhs.minimized_);
            maximized_ = std::move(rhs.maximized_);
            focused_ = std::move(rhs.focused_);
            frameCount_ = std::move(rhs.frameCount_);

            rhs.glfwWindowData_ = nullptr;
            rhs.window_ = nullptr;
        }
        return *this;
    }

    VKWindow::~VKWindow()
    {
        this->ReleaseVulkan();
        this->ReleaseWindow();
        config_->fullscreen_ = maximized_;
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
        vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic> deleter(ApplicationBase::instance().GetVKInstance(), nullptr, vk::DispatchLoaderStatic());
        vkSurface_ = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surfaceKHR), deleter);
        if (result != VK_SUCCESS) {
            LOG(FATAL) << "Could not create window surface (" << vk::to_string(vk::Result(result)) << ").";
            throw std::runtime_error("Could not create window surface.");
        }
        logicalDevice_ = ApplicationBase::instance().CreateLogicalDevice(*config_, *vkSurface_);
        for (auto i = 0U; i < config_->queues_.size(); ++i) if (config_->queues_[i].graphics_) {
            graphicsQueue_ = i;
            break;
        }

        RecreateSwapChain();

        vk::SemaphoreCreateInfo semaphoreInfo{ };
        vkImageAvailableSemaphore_ = logicalDevice_->GetDevice().createSemaphoreUnique(semaphoreInfo);
        vkDataAvailableSemaphore_ = logicalDevice_->GetDevice().createSemaphoreUnique(semaphoreInfo);
        vkRenderingFinishedSemaphore_ = logicalDevice_->GetDevice().createSemaphoreUnique(semaphoreInfo);

        LOG(INFO) << "Initializing Vulkan surface... done.";
    }

    void VKWindow::InitGUI()
    {
        ImGui_ImplVulkanH_WindowData* wd = windowData_.get();

        {
            wd->Surface = vkSurface_.get();

            // Check for WSI support
            auto res = logicalDevice_->GetPhysicalDevice().getSurfaceSupportKHR(graphicsQueue_, wd->Surface);
            if (res != VK_TRUE)
            {
                LOG(FATAL) << "Error no WSI support on physical device.";
                throw std::runtime_error("No WSI support on physical device.");
            }
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;


        // Setup GLFW binding
        ImGui_ImplGlfw_InitForVulkan(&glfwWindowData_, window_);

        vk::DescriptorPoolSize imguiDescPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 };
        vk::DescriptorPoolCreateInfo imguiDescSetPoolInfo{ vk::DescriptorPoolCreateFlags(), 1, 1, &imguiDescPoolSize };
        vkImguiDescPool_ = logicalDevice_->GetDevice().createDescriptorPoolUnique(imguiDescSetPoolInfo);

        // Setup Vulkan binding
        imguiVulkanData_ = std::make_unique<ImGui_ImplVulkan_InitInfo>();
        imguiVulkanData_->Instance = ApplicationBase::instance().GetVKInstance();
        imguiVulkanData_->PhysicalDevice = logicalDevice_->GetPhysicalDevice();
        imguiVulkanData_->Device = logicalDevice_->GetDevice();
        imguiVulkanData_->QueueFamily = graphicsQueue_;
        imguiVulkanData_->Queue = logicalDevice_->GetQueue(graphicsQueue_, 0);
        imguiVulkanData_->PipelineCache = VK_NULL_HANDLE;
        imguiVulkanData_->DescriptorPool = vkImguiDescPool_.get();
        imguiVulkanData_->Allocator = nullptr;
        ImGui_ImplVulkan_Init(imguiVulkanData_.get(), wd->RenderPass, vkImGuiCommandBuffers_.size());

        // Setup style
        ImGui::StyleColorsDark();

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'misc/fonts/README.txt' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        // Upload Fonts
        {
            VkCommandBuffer command_buffer = *vkImGuiCommandBuffers_[0];

            logicalDevice_->GetDevice().resetCommandPool(*vkImGuiCommandPools_[0], vk::CommandPoolResetFlags());
            vk::CommandBufferBeginInfo begin_info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
            vkImGuiCommandBuffers_[0]->begin(begin_info);

            ImGui_ImplVulkan_CreateFontsTexture(imguiVulkanData_.get(), command_buffer);

            vk::SubmitInfo end_info{ 0, nullptr, nullptr, 1, &(*vkImGuiCommandBuffers_[0]) };
            vkImGuiCommandBuffers_[0]->end();
            logicalDevice_->GetQueue(graphicsQueue_, 0).submit(end_info, vk::Fence());

            logicalDevice_->GetDevice().waitIdle();

            ImGui_ImplVulkan_InvalidateFontUploadObjects(imguiVulkanData_.get());
        }
    }

    void VKWindow::RecreateSwapChain()
    {
        logicalDevice_->GetDevice().waitIdle();

        vkCommandBuffers_.clear();
        vkImGuiCommandBuffers_.clear();
        vkCommandPools_.clear();
        vkImGuiCommandPools_.clear();
        vkCmdBufferUFences_.clear();

        DestroySwapchainImages();

        auto surfaceCapabilities = logicalDevice_->GetPhysicalDevice().getSurfaceCapabilitiesKHR(*vkSurface_);
        auto deviceSurfaceFormats = logicalDevice_->GetPhysicalDevice().getSurfaceFormatsKHR(*vkSurface_);
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
        auto surfaceCaps = logicalDevice_->GetPhysicalDevice().getSurfaceCapabilitiesKHR(*vkSurface_);
        glm::u32vec2 configSurfaceSize(static_cast<std::uint32_t>(config_->windowWidth_), static_cast<std::uint32_t>(config_->windowHeight_));
        glm::u32vec2 minSurfaceSize(surfaceCaps.minImageExtent.width, surfaceCaps.minImageExtent.height);
        glm::u32vec2 maxSurfaceSize(surfaceCaps.maxImageExtent.width, surfaceCaps.maxImageExtent.height);
        auto surfaceExtend = glm::clamp(configSurfaceSize, minSurfaceSize, maxSurfaceSize);
        vkSurfaceExtend_ = vk::Extent2D{ surfaceExtend.x, surfaceExtend.y };
        auto imageCount = surfaceCapabilities.minImageCount + cfg::GetVulkanAdditionalImageCountFromConfig(*config_);

        {
            vk::SwapchainCreateInfoKHR swapChainCreateInfo{ vk::SwapchainCreateFlagsKHR(), *vkSurface_, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
                vkSurfaceExtend_, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, surfaceCapabilities.currentTransform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, *vkSwapchain_ };
            vkSwapchain_ = logicalDevice_->GetDevice().createSwapchainKHRUnique(swapChainCreateInfo);
        }
        windowData_->Width = vkSurfaceExtend_.width;
        windowData_->Height = vkSurfaceExtend_.height;
        windowData_->Swapchain = *vkSwapchain_;
        windowData_->PresentMode = static_cast<VkPresentModeKHR>(presentMode);
        windowData_->SurfaceFormat = surfaceFormat;

        auto swapchainImages = logicalDevice_->GetDevice().getSwapchainImagesKHR(*vkSwapchain_);

        auto dsFormat = FindSupportedDepthFormat();
        {
            // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
            vk::AttachmentDescription colorAttachment{ vk::AttachmentDescriptionFlags(), surfaceFormat.format, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };
            vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
            // TODO: check the stencil load/store operations. [3/20/2017 Sebastian Maisch]
            vk::AttachmentDescription depthAttachment{ vk::AttachmentDescriptionFlags(), dsFormat.second, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal };
            vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

            vk::SubpassDescription subPass{ vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr,
                1, &colorAttachmentRef, nullptr, &depthAttachmentRef, 0, nullptr };

            vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags(),
                vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
            std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
            vk::RenderPassCreateInfo renderPassInfo{ vk::RenderPassCreateFlags(),
                static_cast<std::uint32_t>(attachments.size()), attachments.data(), 1, &subPass, 1, &dependency };

            vkSwapchainRenderPass_ = logicalDevice_->GetDevice().createRenderPassUnique(renderPassInfo);
        }

        {
            // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
            vk::AttachmentDescription colorAttachment{ vk::AttachmentDescriptionFlags(), surfaceFormat.format, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR };
            vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
            // TODO: check the stencil load/store operations. [3/20/2017 Sebastian Maisch]
            vk::AttachmentDescription depthAttachment{ vk::AttachmentDescriptionFlags(), dsFormat.second, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal };
            vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

            vk::SubpassDescription subPass{ vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr,
                1, &colorAttachmentRef, nullptr, &depthAttachmentRef, 0, nullptr };

            vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags(),
                vk::AccessFlagBits::eColorAttachmentWrite };
            std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
            vk::RenderPassCreateInfo renderPassInfo{ vk::RenderPassCreateFlags(),
                static_cast<std::uint32_t>(attachments.size()), attachments.data(), 1, &subPass, 1, &dependency };

            vkImGuiRenderPass_ = logicalDevice_->GetDevice().createRenderPassUnique(renderPassInfo);
            windowData_->RenderPass = *vkImGuiRenderPass_;
        }

        // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
        gfx::FramebufferDescriptor fbDesc;
        fbDesc.tex_.emplace_back(config_->backbufferBits_ / 8, surfaceFormat.format, vk::SampleCountFlagBits::e1);
        fbDesc.tex_.push_back(gfx::TextureDescriptor::DepthBufferTextureDesc(dsFormat.first, dsFormat.second, vk::SampleCountFlagBits::e1));
        swapchainFramebuffers_.reserve(swapchainImages.size());

        vkCommandPools_.resize(swapchainImages.size());
        vkImGuiCommandPools_.resize(swapchainImages.size());
        vkCommandBuffers_.resize(swapchainImages.size());
        vkImGuiCommandBuffers_.resize(swapchainImages.size());

        vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlags(), logicalDevice_->GetQueueInfo(graphicsQueue_).familyIndex_ };

        for (std::size_t i = 0; i < swapchainImages.size(); ++i) {
            std::vector<vk::Image> attachments{ swapchainImages[i] };
            swapchainFramebuffers_.emplace_back(logicalDevice_.get(), glm::uvec2(vkSurfaceExtend_.width, vkSurfaceExtend_.height),
                attachments, *vkSwapchainRenderPass_, fbDesc);

            vkCommandPools_[i] = logicalDevice_->GetDevice().createCommandPoolUnique(poolInfo);
            vkImGuiCommandPools_[i] = logicalDevice_->GetDevice().createCommandPoolUnique(poolInfo);

            vk::CommandBufferAllocateInfo allocInfo{ *vkCommandPools_[i], vk::CommandBufferLevel::ePrimary, 1 };
            vk::CommandBufferAllocateInfo imgui_allocInfo{ *vkImGuiCommandPools_[i], vk::CommandBufferLevel::ePrimary, 1 };

            try {
                vkCommandBuffers_[i] = std::move(logicalDevice_->GetDevice().allocateCommandBuffersUnique(allocInfo)[0]);
                vkImGuiCommandBuffers_[i] = std::move(logicalDevice_->GetDevice().allocateCommandBuffersUnique(imgui_allocInfo)[0]);
            }
            catch (vk::SystemError& e) {
                LOG(FATAL) << "Could not allocate command buffers (" << e.what() << ").";
                throw std::runtime_error("Could not allocate command buffers.");
            }
        }

        vk::FenceCreateInfo fenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled };
        vkCmdBufferUFences_.resize(vkCommandBuffers_.size());
        vkCmdBufferFences_.resize(vkCommandBuffers_.size());
        for (auto& fence : vkCmdBufferUFences_) fence = logicalDevice_->GetDevice().createFenceUnique(fenceCreateInfo);
        std::transform(vkCmdBufferUFences_.begin(), vkCmdBufferUFences_.end(), vkCmdBufferFences_.begin(), [](auto& cmdBuffer) { return *cmdBuffer; });
    }

    void VKWindow::DestroySwapchainImages()
    {
        swapchainFramebuffers_.clear();
        vkSwapchainRenderPass_.reset();
    }

    void VKWindow::ReleaseVulkan()
    {
        logicalDevice_->GetDevice().waitIdle();

        ImGui_ImplVulkan_Shutdown(imguiVulkanData_.get());
        ImGui_ImplGlfw_Shutdown(glfwWindowData_);
        ImGui::DestroyContext();

        vkCmdBufferUFences_.clear();
        vkImageAvailableSemaphore_.reset();
        vkDataAvailableSemaphore_.reset();
        vkRenderingFinishedSemaphore_.reset();
        vkCommandBuffers_.clear();
        vkImGuiCommandBuffers_.clear();
        vkCommandPools_.clear();

        vkImGuiRenderPass_.reset();
        vkImguiDescPool_.reset();
        vkImGuiCommandPools_.clear();

        DestroySwapchainImages();
        vkSwapchain_.reset();
        logicalDevice_.reset();
        vkSurface_.reset();
    }

    std::pair<unsigned int, vk::Format> VKWindow::FindSupportedDepthFormat() const
    {
        std::vector<std::pair<unsigned int, vk::Format>> candidates;
        if (config_->depthBufferBits_ == 16 && config_->stencilBufferBits_ == 0) candidates.emplace_back(std::make_pair(2, vk::Format::eD16Unorm));
        if (config_->depthBufferBits_ <= 24 && config_->stencilBufferBits_ == 0) candidates.emplace_back(std::make_pair(4, vk::Format::eX8D24UnormPack32));
        if (config_->depthBufferBits_ <= 32 && config_->stencilBufferBits_ == 0) candidates.emplace_back(std::make_pair(4, vk::Format::eD32Sfloat));
        if (config_->depthBufferBits_ ==  0 && config_->stencilBufferBits_ <= 8) candidates.emplace_back(std::make_pair(1, vk::Format::eS8Uint));
        if (config_->depthBufferBits_ <= 16 && config_->stencilBufferBits_ <= 8) candidates.emplace_back(std::make_pair(3, vk::Format::eD16UnormS8Uint));
        if (config_->depthBufferBits_ <= 24 && config_->stencilBufferBits_ <= 8) candidates.emplace_back(std::make_pair(4, vk::Format::eD24UnormS8Uint));
        if (config_->depthBufferBits_ <= 32 && config_->stencilBufferBits_ <= 8) candidates.emplace_back(std::make_pair(5, vk::Format::eD32SfloatS8Uint));

        return logicalDevice_->FindSupportedFormat(candidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
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
        auto result = logicalDevice_->GetDevice().acquireNextImageKHR(*vkSwapchain_, std::numeric_limits<std::uint64_t>::max(), *vkImageAvailableSemaphore_, vk::Fence());
        currentlyRenderedImage_ = result.value;

        if (result.result == vk::Result::eErrorOutOfDateKHR) {
            RecreateSwapChain();
            return;
        }
        if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
            LOG(FATAL) << "Could not acquire swap chain image (" << vk::to_string(result.result) << ").";
            throw std::runtime_error("Could not acquire swap chain image.");
        }

        // Start the Dear ImGui frame
        if (ApplicationBase::instance().IsGUIMode()) {
            ImGui_ImplVulkan_NewFrame(imguiVulkanData_.get());
            ImGui_ImplGlfw_NewFrame(glfwWindowData_);
            ImGui::NewFrame();
        }
    }

    void VKWindow::DrawCurrentCommandBuffer() const
    {
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

        // Rendering
        if (ApplicationBase::instance().IsGUIMode()) {
            ImGui::Render();

            {
                logicalDevice_->GetDevice().resetCommandPool(*vkImGuiCommandPools_[currentlyRenderedImage_], vk::CommandPoolResetFlags());
                vk::CommandBufferBeginInfo cmdBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
                vkImGuiCommandBuffers_[currentlyRenderedImage_]->begin(cmdBufferBeginInfo);
            }

            {
                vk::RenderPassBeginInfo imGuiRenderPassBeginInfo{ *vkImGuiRenderPass_, swapchainFramebuffers_[currentlyRenderedImage_].GetFramebuffer(),
                    vk::Rect2D(vk::Offset2D(0, 0), vkSurfaceExtend_), 0, nullptr };
                vkImGuiCommandBuffers_[currentlyRenderedImage_]->beginRenderPass(imGuiRenderPassBeginInfo, vk::SubpassContents::eInline);
            }

            // Record ImGui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(imguiVulkanData_.get(), ImGui::GetDrawData(), *vkImGuiCommandBuffers_[currentlyRenderedImage_]);

            vkImGuiCommandBuffers_[currentlyRenderedImage_]->endRenderPass();
            vkImGuiCommandBuffers_[currentlyRenderedImage_]->end();
        }

        vk::PipelineStageFlags waitStages[]{ vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTopOfPipe };
        vk::Semaphore waitSemaphores[]{ *vkImageAvailableSemaphore_, *vkDataAvailableSemaphore_ };
        std::array<vk::CommandBuffer, 2> submitCmdBuffers{ *vkCommandBuffers_[currentlyRenderedImage_], *vkImGuiCommandBuffers_[currentlyRenderedImage_] };
        vk::SubmitInfo submitInfo{ 2, waitSemaphores, waitStages, 2, submitCmdBuffers.data(), 1, &(*vkRenderingFinishedSemaphore_) };

        logicalDevice_->GetQueue(graphicsQueue_, 0).submit(submitInfo, vkCmdBufferFences_[currentlyRenderedImage_]);
    }

    void VKWindow::SubmitFrame()
    {
        vk::SwapchainKHR swapchains[] = { *vkSwapchain_ };
        vk::PresentInfoKHR presentInfo{ 1, &(*vkRenderingFinishedSemaphore_), 1, swapchains, &currentlyRenderedImage_ }; //<- wait on these semaphores
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

    void VKWindow::UpdatePrimaryCommandBuffers(const std::function<void(const vk::CommandBuffer& commandBuffer, std::size_t cmdBufferIndex)>& fillFunc) const
    {
        for (std::size_t i = 0U; i < vkCommandBuffers_.size(); ++i) {
            {
                auto syncResult = vk::Result::eTimeout;
                while (syncResult == vk::Result::eTimeout)
                    syncResult = logicalDevice_->GetDevice().waitForFences(vkCmdBufferFences_[i], VK_TRUE, defaultFenceTimeout);

                if (syncResult != vk::Result::eSuccess) {
                    LOG(FATAL) << "Error synchronizing command buffers. (" << vk::to_string(syncResult) << ").";
                    throw std::runtime_error("Error synchronizing command buffers.");
                }
            }

            logicalDevice_->GetDevice().resetCommandPool(*vkCommandPools_[i], vk::CommandPoolResetFlags());

            vk::CommandBufferBeginInfo cmdBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            vkCommandBuffers_[i]->begin(cmdBufferBeginInfo);

            std::array<vk::ClearValue, 2> clearColor;
            clearColor[0].setColor(vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } });
            clearColor[1].setDepthStencil(vk::ClearDepthStencilValue{ 1.0f, 0 });
            vk::RenderPassBeginInfo renderPassBeginInfo{ *vkSwapchainRenderPass_, swapchainFramebuffers_[i].GetFramebuffer(),
                vk::Rect2D(vk::Offset2D(0, 0), vkSurfaceExtend_), static_cast<std::uint32_t>(clearColor.size()), clearColor.data() };
            vkCommandBuffers_[i]->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

            fillFunc(*vkCommandBuffers_[i], i);

            vkCommandBuffers_[i]->endRenderPass();
            vkCommandBuffers_[i]->end();
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

        LOG(G3LOG_DEBUG) << "Begin HandleResize()";

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
    void VKWindow::CharCallback(unsigned int) const
    {
        // Not needed at this point... [4/7/2016 Sebastian Maisch]
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void VKWindow::CharModsCallback(unsigned int, int) const
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
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        ImGui_ImplGlfw_MouseButtonCallback(win->glfwWindowData_, button, action, mods);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            win->MouseButtonCallback(button, action, mods);
        }
    }

    void VKWindow::glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->CursorPosCallback(xpos, ypos);
        }
    }

    void VKWindow::glfwCursorEnterCallback(GLFWwindow* window, int entered)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->CursorEnterCallback(entered);
    }

    void VKWindow::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        ImGui_ImplGlfw_ScrollCallback(win->glfwWindowData_, xoffset, yoffset);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            win->ScrollCallback(xoffset, yoffset);
        }
    }

    void VKWindow::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        ImGui_ImplGlfw_KeyCallback(win->glfwWindowData_, key, scancode, action, mods);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            win->KeyCallback(key, scancode, action, mods);
        }
    }

    void VKWindow::glfwCharCallback(GLFWwindow* window, unsigned int codepoint)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        ImGui_ImplGlfw_CharCallback(win->glfwWindowData_, codepoint);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            win->CharCallback(codepoint);
        }
    }

    void VKWindow::glfwCharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
    {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
            win->CharModsCallback(codepoint, mods);
        }
    }

    void VKWindow::glfwDropCallback(GLFWwindow* window, int count, const char** paths)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window));
        win->DropCallback(count, paths);
    }

}
