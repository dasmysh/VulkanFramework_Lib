/**
 * @file   VKWindow.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.17
 *
 * @brief  Windows implementation for the VKWindow.
 */

#include "app/VKWindow.h"
#include "app/ApplicationBase.h"
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.hpp>
#include <gfx/vk/LogicalDevice.h>
#include "gfx/vk/Framebuffer.h"
#include "imgui.h"
#include "core/imgui/imgui_impl_glfw.h"
#include "core/imgui/imgui_impl_vulkan.h"

namespace vkfw_core {

    /**
     * Creates a new windows VKWindow.
     * @param conf the window configuration used
     */
    VKWindow::VKWindow(cfg::WindowCfg& conf, bool useGUI, const std::vector<std::string>& requiredDeviceExtensions,
                       void* deviceFeaturesNextChain)
        : m_window{nullptr}
        , m_config{&conf}
        , m_surface{nullptr, fmt::format("Win-{} Surface", conf.m_windowTitle), vk::UniqueSurfaceKHR{}}
        , m_swapchain{nullptr, fmt::format("Win-{} Swapchain", conf.m_windowTitle), vk::UniqueSwapchainKHR{}}
        , m_swapchainRenderPass{nullptr, fmt::format("Win-{} SCRenderPass", conf.m_windowTitle), vk::UniqueRenderPass{}}
        , m_imGuiRenderPass{nullptr, fmt::format("Win-{} ImGuiRenderPass", conf.m_windowTitle), vk::UniqueRenderPass{}}
        , m_imguiDescPool{nullptr, fmt::format("Win-{} ImGuiDescriptorPool", conf.m_windowTitle), vk::UniqueDescriptorPool{}}
        , m_currMousePosition(0.0f),
        m_prevMousePosition(0.0f),
        m_relativeMousePosition(0.0f),
        m_mouseInWindow(true),
        m_minimized(false),
        m_maximized(conf.m_fullscreen),
        m_frameCount(0)
    {
        m_windowData = std::make_unique<ImGui_ImplVulkanH_Window>();
        this->InitWindow();

        vk::PhysicalDeviceVulkan12Features enableVulkan12Features;
        enableVulkan12Features.setBufferDeviceAddress(true);
        enableVulkan12Features.setDescriptorIndexing(true);
        enableVulkan12Features.setScalarBlockLayout(true);
        enableVulkan12Features.setRuntimeDescriptorArray(true);
        enableVulkan12Features.setShaderStorageBufferArrayNonUniformIndexing(true);
        enableVulkan12Features.setShaderSampledImageArrayNonUniformIndexing(true);
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{VK_TRUE};
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{VK_TRUE};
        std::vector<std::string> reqDeviceExtensions = requiredDeviceExtensions;
        if (m_config->m_useRayTracing) {
            if (std::find(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end(),
                          VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                == requiredDeviceExtensions.end()) {
                reqDeviceExtensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            }

            if (std::find(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end(),
                          VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                == requiredDeviceExtensions.end()) {
                reqDeviceExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            }

            if (std::find(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end(),
                          VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
                == requiredDeviceExtensions.end()) {
                reqDeviceExtensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            }

            if (std::find(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end(),
                          VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
                == requiredDeviceExtensions.end()) {
                reqDeviceExtensions.emplace_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
            }

            // there is no way to check the next chain, so we just add and hope it is fine :).
            enabledAccelerationStructureFeatures.pNext = deviceFeaturesNextChain;
            enabledRayTracingPipelineFeatures.pNext = &enabledAccelerationStructureFeatures;
            enableVulkan12Features.pNext = &enabledRayTracingPipelineFeatures;
        } else {
            enableVulkan12Features.pNext = deviceFeaturesNextChain;
        }

        this->InitVulkan(reqDeviceExtensions, &enableVulkan12Features);
        if (useGUI) { InitGUI(); }
    }

    VKWindow::VKWindow(VKWindow&& rhs) noexcept :
        m_window{ rhs.m_window },
        m_config{ rhs.m_config },
        m_surface{ std::move(rhs.m_surface) },
        m_vkSurfaceExtend{ rhs.m_vkSurfaceExtend },
        m_logicalDevice{ std::move(rhs.m_logicalDevice) },
        m_graphicsQueue{ rhs.m_graphicsQueue },
        m_swapchain{ std::move(rhs.m_swapchain) },
        m_swapchainRenderPass{ std::move(rhs.m_swapchainRenderPass) }
        , m_imGuiRenderPass{ std::move(rhs.m_imGuiRenderPass)}
        ,m_swapchainFramebuffers{ std::move(rhs.m_swapchainFramebuffers) },
        m_commandPools{ std::move(rhs.m_commandPools) },
        m_commandBuffers{ std::move(rhs.m_commandBuffers) },
        m_imGuiCommandPools{ std::move(rhs.m_imGuiCommandPools) },
        m_imGuiCommandBuffers{ std::move(rhs.m_imGuiCommandBuffers) },
        m_imageAvailableSemaphore{ std::move(rhs.m_imageAvailableSemaphore) },
        m_renderingFinishedSemaphore{ std::move(rhs.m_renderingFinishedSemaphore) },
        m_currentlyRenderedImage{ rhs.m_currentlyRenderedImage },
        m_imguiDescPool{ std::move(rhs.m_imguiDescPool) },
        m_windowData{ std::move(rhs.m_windowData) },
        m_imguiVulkanData{ std::move(rhs.m_imguiVulkanData) },
        m_currMousePosition{ rhs.m_currMousePosition },
        m_prevMousePosition{ rhs.m_prevMousePosition },
        m_relativeMousePosition{ rhs.m_relativeMousePosition },
        m_mouseInWindow{ rhs.m_mouseInWindow },
        m_minimized{ rhs.m_minimized },
        m_maximized{ rhs.m_maximized },
        m_focused{ rhs.m_focused },
        m_frameCount{ rhs.m_frameCount }
    {
        rhs.m_window = nullptr;
    }

    VKWindow& VKWindow::operator=(VKWindow&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~VKWindow();
            m_window = rhs.m_window;
            m_config = rhs.m_config;
            m_surface = std::move(rhs.m_surface);
            m_vkSurfaceExtend = rhs.m_vkSurfaceExtend;
            m_logicalDevice = std::move(rhs.m_logicalDevice);
            m_graphicsQueue = rhs.m_graphicsQueue;
            m_swapchain = std::move(rhs.m_swapchain);
            m_swapchainRenderPass = std::move(rhs.m_swapchainRenderPass);
            m_imGuiRenderPass = std::move(rhs.m_imGuiRenderPass);
            m_swapchainFramebuffers = std::move(rhs.m_swapchainFramebuffers);
            m_commandPools = std::move(rhs.m_commandPools);
            m_commandBuffers = std::move(rhs.m_commandBuffers);
            m_imGuiCommandPools = std::move(rhs.m_imGuiCommandPools);
            m_imGuiCommandBuffers = std::move(rhs.m_imGuiCommandBuffers);
            m_imageAvailableSemaphore = std::move(rhs.m_imageAvailableSemaphore);
            m_renderingFinishedSemaphore = std::move(rhs.m_renderingFinishedSemaphore);
            m_currentlyRenderedImage = rhs.m_currentlyRenderedImage;
            m_imguiDescPool = std::move(rhs.m_imguiDescPool);
            m_windowData = std::move(rhs.m_windowData);
            m_imguiVulkanData = std::move(rhs.m_imguiVulkanData);
            m_currMousePosition = rhs.m_currMousePosition;
            m_prevMousePosition = rhs.m_prevMousePosition;
            m_relativeMousePosition = rhs.m_relativeMousePosition;
            m_mouseInWindow = rhs.m_mouseInWindow;
            m_minimized = rhs.m_minimized;
            m_maximized = rhs.m_maximized;
            m_focused = rhs.m_focused;
            m_frameCount = rhs.m_frameCount;

            rhs.m_window = nullptr;
        }
        return *this;
    }

    VKWindow::~VKWindow() noexcept
    {
        try {
            this->ReleaseVulkan();
            this->ReleaseWindow();
        } catch (...) {
            spdlog::critical("Error while releasing vulkan and window. Unknown exception.");
        }
        m_config->m_fullscreen = m_maximized;
        m_config->m_windowWidth = m_vkSurfaceExtend.width;
        m_config->m_windowHeight = m_vkSurfaceExtend.height;
    }

    bool VKWindow::IsClosing() const { return glfwWindowShouldClose(m_window) == GLFW_TRUE; }

    /**
     * Initializes the window.
     */
    void VKWindow::InitWindow()
    {
        spdlog::info("Creating window '{}'.", m_config->m_windowTitle);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwSetErrorCallback(VKWindow::glfwErrorCallback);

        if (m_config->m_fullscreen) {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            m_window =
                glfwCreateWindow(static_cast<int>(m_config->m_windowWidth), static_cast<int>(m_config->m_windowHeight),
                                 m_config->m_windowTitle.c_str(), glfwGetPrimaryMonitor(), nullptr);
            if (m_window == nullptr) {
                spdlog::critical("Could not create window!");
                glfwTerminate();
                throw std::runtime_error("Could not create window!");
            }
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
            m_window =
                glfwCreateWindow(static_cast<int>(m_config->m_windowWidth), static_cast<int>(m_config->m_windowHeight),
                                 m_config->m_windowTitle.c_str(), nullptr, nullptr);
            if (m_window == nullptr) {
                spdlog::critical("Could not create window!");
                glfwTerminate();
                throw std::runtime_error("Could not create window!");
            }
            glfwSetWindowPos(m_window, static_cast<int>(m_config->m_windowLeft),
                             static_cast<int>(m_config->m_windowTop));
        }
        glfwSetWindowUserPointer(m_window, this);
        glfwSetInputMode(m_window, GLFW_STICKY_MOUSE_BUTTONS, 1);
        glfwSetCursorPos(m_window, 0.0, 0.0);

        glfwSetWindowPosCallback(m_window, VKWindow::glfwWindowPosCallback);
        glfwSetWindowSizeCallback(m_window, VKWindow::glfwWindowSizeCallback);
        glfwSetWindowFocusCallback(m_window, VKWindow::glfwWindowFocusCallback);
        glfwSetWindowCloseCallback(m_window, VKWindow::glfwWindowCloseCallback);
        glfwSetFramebufferSizeCallback(m_window, VKWindow::glfwFramebufferSizeCallback);
        glfwSetWindowIconifyCallback(m_window, VKWindow::glfwWindowIconifyCallback);


        glfwSetMouseButtonCallback(m_window, VKWindow::glfwMouseButtonCallback);
        glfwSetCursorPosCallback(m_window, VKWindow::glfwCursorPosCallback);
        glfwSetCursorEnterCallback(m_window, VKWindow::glfwCursorEnterCallback);
        glfwSetScrollCallback(m_window, VKWindow::glfwScrollCallback);
        glfwSetKeyCallback(m_window, VKWindow::glfwKeyCallback);
        glfwSetCharCallback(m_window, VKWindow::glfwCharCallback);
        glfwSetCharModsCallback(m_window, VKWindow::glfwCharModsCallback);
        glfwSetDropCallback(m_window, VKWindow::glfwDropCallback);

        // Check for Valid Context
        if (m_window == nullptr) {
            spdlog::critical("Could not create window!");
            glfwTerminate();
            throw std::runtime_error("Could not create window!");
        }

        spdlog::info("Window successfully initialized.");
    }

    /**
     * Initializes OpenGL.
     */
    void VKWindow::InitVulkan(const std::vector<std::string>& requiredDeviceExtensions, void* featuresNextChain)
    {
        spdlog::info("Initializing Vulkan surface...");

        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;
        auto result = glfwCreateWindowSurface(ApplicationBase::instance().GetVKInstance(), m_window, nullptr, &surfaceKHR);
        vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic> deleter(ApplicationBase::instance().GetVKInstance());
        m_surface = gfx::Surface{nullptr, fmt::format("Win-{} Surface", m_config->m_windowTitle), vk::UniqueSurfaceKHR(vk::SurfaceKHR(surfaceKHR), deleter)};
        if (result != VK_SUCCESS) {
            spdlog::critical("Could not create window surface ({}).", vk::to_string(vk::Result(result)));
            throw std::runtime_error("Could not create window surface.");
        }
        m_logicalDevice = ApplicationBase::instance().CreateLogicalDevice(*m_config, requiredDeviceExtensions,
                                                                          featuresNextChain, m_surface);
        m_surface = gfx::Surface{m_logicalDevice->GetHandle(), std::move(m_surface)};
        for (auto i = 0U; i < m_config->m_queues.size(); ++i) {
            if (m_config->m_queues[i].m_graphics) {
                m_graphicsQueue = i;
                break;
            }
        }

        RecreateSwapChain();

        vk::SemaphoreCreateInfo semaphoreInfo{ };
        m_imageAvailableSemaphore = gfx::Semaphore{m_logicalDevice->GetHandle(), "Win-{} ImgAvailSemaphore",
                                                     m_logicalDevice->GetHandle().createSemaphoreUnique(semaphoreInfo)};
        m_dataAvailableSemaphore = gfx::Semaphore{m_logicalDevice->GetHandle(), "Win-{} DataAvailSemaphore",
                                                  m_logicalDevice->GetHandle().createSemaphoreUnique(semaphoreInfo)};
        m_renderingFinishedSemaphore =
            gfx::Semaphore{m_logicalDevice->GetHandle(), "Win-{} RenderFinishedSemaphore",
                           m_logicalDevice->GetHandle().createSemaphoreUnique(semaphoreInfo)};

        spdlog::info("Initializing Vulkan surface... done.");
    }

    void VKWindow::InitGUI()
    {
        ImGui_ImplVulkanH_Window* wd = m_windowData.get();

        {
            wd->Surface = m_surface.GetHandle();

            // Check for WSI support
            auto res = m_logicalDevice->GetPhysicalDevice().getSurfaceSupportKHR(m_graphicsQueue, wd->Surface);
            if (res != VK_TRUE)
            {
                spdlog::critical("Error no WSI support on physical device.");
                throw std::runtime_error("No WSI support on physical device.");
            }
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;


        // Setup GLFW binding
        ImGui_ImplGlfw_InitForVulkan(m_window, true);

        vk::DescriptorPoolSize imguiDescPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 };
        vk::DescriptorPoolCreateInfo imguiDescSetPoolInfo{ vk::DescriptorPoolCreateFlags(), 1, 1, &imguiDescPoolSize };
        m_imguiDescPool = gfx::DescriptorPool{
            m_logicalDevice->GetHandle(), fmt::format("Win-{} ImGuiDescPool", m_config->m_windowTitle),
            m_logicalDevice->GetHandle().createDescriptorPoolUnique(imguiDescSetPoolInfo)};

        // Setup Vulkan binding
        m_imguiVulkanData = std::make_unique<ImGui_ImplVulkan_InitInfo>();
        m_imguiVulkanData->Instance = ApplicationBase::instance().GetVKInstance();
        m_imguiVulkanData->PhysicalDevice = m_logicalDevice->GetPhysicalDevice();
        m_imguiVulkanData->Device = m_logicalDevice->GetHandle();
        m_imguiVulkanData->QueueFamily = m_graphicsQueue;
        m_imguiVulkanData->Queue = m_logicalDevice->GetQueue(m_graphicsQueue, 0).GetHandle();
        m_imguiVulkanData->PipelineCache = VK_NULL_HANDLE;
        m_imguiVulkanData->DescriptorPool = m_imguiDescPool.GetHandle();
        m_imguiVulkanData->Allocator = nullptr;
        m_imguiVulkanData->CheckVkResultFn = nullptr;
        m_imguiVulkanData->ImageCount = static_cast<std::uint32_t>(m_imGuiCommandBuffers.size());
        m_imguiVulkanData->MinImageCount = static_cast<std::uint32_t>(m_imGuiCommandBuffers.size());
        m_imguiVulkanData->MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        m_imguiVulkanData->Subpass = 0;
        ImGui_ImplVulkan_Init(m_imguiVulkanData.get(), wd->RenderPass);

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
            // VkCommandBuffer command_buffer = *m_vkImGuiCommandBuffers[0];

            m_logicalDevice->GetHandle().resetCommandPool(m_imGuiCommandPools[0].GetHandle(), vk::CommandPoolResetFlags());
            vk::CommandBufferBeginInfo begin_info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
            m_imGuiCommandBuffers[0].Begin(begin_info);

            ImGui_ImplVulkan_CreateFontsTexture(m_imGuiCommandBuffers[0].GetHandle());

            vk::SubmitInfo end_info{0, nullptr, nullptr, 1, m_imGuiCommandBuffers[0].GetHandlePtr()};
            m_imGuiCommandBuffers[0].End();

            const auto& graphicsQueue = m_logicalDevice->GetQueue(m_graphicsQueue, 0);
            {
                QUEUE_REGION(graphicsQueue, "Upload ImGui Fonts");
                graphicsQueue.Submit(end_info, gfx::Fence{});
            }

            m_logicalDevice->GetHandle().waitIdle();

            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
        m_imguiInitialized = true;
    }

    void VKWindow::RecreateSwapChain()
    {
        int width = 0;
        int height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        m_logicalDevice->GetHandle().waitIdle();

        m_commandBuffers.clear();
        m_imGuiCommandBuffers.clear();
        m_commandPools.clear();
        m_imGuiCommandPools.clear();
        m_cmdBufferFences.clear();

        DestroySwapchainImages();

        // NOLINTNEXTLINE
        auto surfaceCapabilities =
            m_logicalDevice->GetPhysicalDevice().getSurfaceCapabilitiesKHR(m_surface.GetHandle());
        auto deviceSurfaceFormats = m_logicalDevice->GetPhysicalDevice().getSurfaceFormatsKHR(m_surface.GetHandle());
        auto surfaceFormats = cfg::GetVulkanSurfaceFormatsFromConfig(*m_config);
        std::sort(deviceSurfaceFormats.begin(), deviceSurfaceFormats.end(),
                  [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0.format < f1.format; });
        std::sort(surfaceFormats.begin(), surfaceFormats.end(),
                  [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0.format < f1.format; });

        vk::SurfaceFormatKHR surfaceFormat;
        if (deviceSurfaceFormats.size() == 1 && deviceSurfaceFormats[0].format == vk::Format::eUndefined) {
            surfaceFormat = surfaceFormats[0];
        } else {
            std::vector<vk::SurfaceFormatKHR> formatIntersection;
            std::set_intersection(
                deviceSurfaceFormats.begin(), deviceSurfaceFormats.end(), surfaceFormats.begin(), surfaceFormats.end(),
                std::back_inserter(formatIntersection),
                [](const vk::SurfaceFormatKHR& f0, const vk::SurfaceFormatKHR& f1) { return f0 == f1; });
            if (!formatIntersection.empty()) {
                surfaceFormat = formatIntersection
                    [0]; // no color space check here as color space does not depend on the color space flag but the actual format.
            } else {
                spdlog::critical(
                    "No suitable surface format found after correct enumeration (this should never happen).");
                throw std::runtime_error(
                    "No suitable surface format found after correct enumeration (this should never happen).");
            }
        }

        auto presentMode = cfg::GetVulkanPresentModeFromConfig(*m_config);
        glm::u32vec2 configSurfaceSize(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
        glm::u32vec2 minSurfaceSize(surfaceCapabilities.minImageExtent.width,
                                    surfaceCapabilities.minImageExtent.height);
        glm::u32vec2 maxSurfaceSize(surfaceCapabilities.maxImageExtent.width,
                                    surfaceCapabilities.maxImageExtent.height);
        auto surfaceExtend = glm::clamp(configSurfaceSize, minSurfaceSize, maxSurfaceSize);
        m_vkSurfaceExtend = vk::Extent2D{surfaceExtend.x, surfaceExtend.y};
        auto imageCount = surfaceCapabilities.minImageCount + cfg::GetVulkanAdditionalImageCountFromConfig(*m_config);

        {
            vk::SwapchainCreateInfoKHR swapChainCreateInfo{vk::SwapchainCreateFlagsKHR(),
                                                           m_surface.GetHandle(),
                                                           imageCount,
                                                           surfaceFormat.format,
                                                           surfaceFormat.colorSpace,
                                                           m_vkSurfaceExtend,
                                                           1,
                                                           vk::ImageUsageFlagBits::eColorAttachment,
                                                           vk::SharingMode::eExclusive,
                                                           0,
                                                           nullptr,
                                                           surfaceCapabilities.currentTransform,
                                                           vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                           presentMode,
                                                           static_cast<vk::Bool32>(true),
                                                           m_swapchain.GetHandle()};

            // not sure if this is needed for anything else but ray tracing ...
            if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc) {
                swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
            }

            if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
                swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
            }

            m_swapchain =
                gfx::Swapchain{m_logicalDevice->GetHandle(), fmt::format("Win-{} Swapchain", m_config->m_windowTitle),
                               m_logicalDevice->GetHandle().createSwapchainKHRUnique(swapChainCreateInfo)};
            m_windowData->Width = m_vkSurfaceExtend.width;
            m_windowData->Height = m_vkSurfaceExtend.height;
            m_windowData->Swapchain = m_swapchain.GetHandle();
            m_windowData->PresentMode = static_cast<VkPresentModeKHR>(presentMode);
            m_windowData->SurfaceFormat = surfaceFormat;

            auto swapchainImages = m_logicalDevice->GetHandle().getSwapchainImagesKHR(m_swapchain.GetHandle());

            auto dsFormat = FindSupportedDepthFormat();
            {
                // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
                vk::AttachmentDescription colorAttachment{vk::AttachmentDescriptionFlags(),
                                                          surfaceFormat.format,
                                                          vk::SampleCountFlagBits::e1,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eStore,
                                                          vk::AttachmentLoadOp::eDontCare,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::ImageLayout::eUndefined,
                                                          vk::ImageLayout::eColorAttachmentOptimal};
                vk::AttachmentReference colorAttachmentRef{0, vk::ImageLayout::eColorAttachmentOptimal};
                // TODO: check the stencil load/store operations. [3/20/2017 Sebastian Maisch]
                vk::AttachmentDescription depthAttachment{vk::AttachmentDescriptionFlags(),
                                                          dsFormat.second,
                                                          vk::SampleCountFlagBits::e1,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                          vk::ImageLayout::eDepthStencilAttachmentOptimal};
                vk::AttachmentReference depthAttachmentRef{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

                vk::SubpassDescription subPass{vk::SubpassDescriptionFlags(),
                                               vk::PipelineBindPoint::eGraphics,
                                               0,
                                               nullptr,
                                               1,
                                               &colorAttachmentRef,
                                               nullptr,
                                               &depthAttachmentRef,
                                               0,
                                               nullptr};

                vk::SubpassDependency dependency{VK_SUBPASS_EXTERNAL,
                                                 0,
                                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                 vk::AccessFlags(),
                                                 vk::AccessFlagBits::eColorAttachmentRead
                                                     | vk::AccessFlagBits::eColorAttachmentWrite};
                std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
                vk::RenderPassCreateInfo renderPassInfo{vk::RenderPassCreateFlags(),
                                                        static_cast<std::uint32_t>(attachments.size()),
                                                        attachments.data(),
                                                        1,
                                                        &subPass,
                                                        1,
                                                        &dependency};

                m_swapchainRenderPass = gfx::RenderPass{
                    m_logicalDevice->GetHandle(), fmt::format("Win-{} SwapchainRenderPass", m_config->m_windowTitle),
                    m_logicalDevice->GetHandle().createRenderPassUnique(renderPassInfo)};
            }

            {
                // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
                vk::AttachmentDescription colorAttachment{
                    vk::AttachmentDescriptionFlags(), surfaceFormat.format,
                    vk::SampleCountFlagBits::e1,      vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,    vk::AttachmentLoadOp::eDontCare,
                    vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::ePresentSrcKHR};
                vk::AttachmentReference colorAttachmentRef{0, vk::ImageLayout::eColorAttachmentOptimal};
                // TODO: check the stencil load/store operations. [3/20/2017 Sebastian Maisch]
                vk::AttachmentDescription depthAttachment{vk::AttachmentDescriptionFlags(),
                                                          dsFormat.second,
                                                          vk::SampleCountFlagBits::e1,
                                                          vk::AttachmentLoadOp::eLoad,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::AttachmentLoadOp::eDontCare,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                          vk::ImageLayout::eDepthStencilAttachmentOptimal};
                vk::AttachmentReference depthAttachmentRef{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

                vk::SubpassDescription subPass{vk::SubpassDescriptionFlags(),
                                               vk::PipelineBindPoint::eGraphics,
                                               0,
                                               nullptr,
                                               1,
                                               &colorAttachmentRef,
                                               nullptr,
                                               &depthAttachmentRef,
                                               0,
                                               nullptr};

                vk::SubpassDependency dependency{VK_SUBPASS_EXTERNAL,
                                                 0,
                                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                 vk::AccessFlags(),
                                                 vk::AccessFlagBits::eColorAttachmentWrite};
                std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
                vk::RenderPassCreateInfo renderPassInfo{vk::RenderPassCreateFlags(),
                                                        static_cast<std::uint32_t>(attachments.size()),
                                                        attachments.data(),
                                                        1,
                                                        &subPass,
                                                        1,
                                                        &dependency};

                m_imGuiRenderPass = gfx::RenderPass{
                    m_logicalDevice->GetHandle(), fmt::format("Win-{} ImGuiRenderPass", m_config->m_windowTitle),
                    m_logicalDevice->GetHandle().createRenderPassUnique(renderPassInfo)};
                m_windowData->RenderPass = m_imGuiRenderPass.GetHandle();
            }

            // TODO: set correct multisampling flags. [11/2/2016 Sebastian Maisch]
            gfx::FramebufferDescriptor fbDesc;
            fbDesc.m_tex.emplace_back(m_config->m_backbufferBits / 8, surfaceFormat.format,
                                      vk::SampleCountFlagBits::e1);
            if (m_config->m_useRayTracing) { fbDesc.m_tex.back().m_imageUsage |= vk::ImageUsageFlagBits::eTransferDst; }
            fbDesc.m_tex.push_back(gfx::TextureDescriptor::DepthBufferTextureDesc(dsFormat.first, dsFormat.second,
                                                                                  vk::SampleCountFlagBits::e1));
            m_swapchainFramebuffers.reserve(swapchainImages.size());

            m_commandPools.resize(swapchainImages.size());
            m_imGuiCommandPools.resize(swapchainImages.size());
            m_commandBuffers.resize(swapchainImages.size());
            m_imGuiCommandBuffers.resize(swapchainImages.size());
            m_cmdBufferFences.resize(m_commandBuffers.size());

            vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlags(),
                                               m_logicalDevice->GetQueueInfo(m_graphicsQueue).m_familyIndex};
            vk::FenceCreateInfo fenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};

            for (std::size_t i = 0; i < swapchainImages.size(); ++i) {
                std::vector<vk::Image> attachments{swapchainImages[i]};
                m_swapchainFramebuffers.emplace_back(m_logicalDevice.get(),
                                                     fmt::format("Win-{} SwapchainImage{}", m_config->m_windowTitle, i),
                                                     glm::uvec2(m_vkSurfaceExtend.width, m_vkSurfaceExtend.height),
                                                     attachments, m_swapchainRenderPass, fbDesc);

                m_commandPools[i] = gfx::CommandPool{
                    m_logicalDevice->GetHandle(), fmt::format("Win-{} CommandPool{}", m_config->m_windowTitle, i),
                    m_graphicsQueue, m_logicalDevice->GetHandle().createCommandPoolUnique(poolInfo)};
                m_imGuiCommandPools[i] = gfx::CommandPool{
                    m_logicalDevice->GetHandle(), fmt::format("Win-{} ImGuiCommandPool{}", m_config->m_windowTitle, i),
                    m_graphicsQueue, m_logicalDevice->GetHandle().createCommandPoolUnique(poolInfo)};

                vk::CommandBufferAllocateInfo allocInfo{m_commandPools[i].GetHandle(), vk::CommandBufferLevel::ePrimary,
                                                        1};
                vk::CommandBufferAllocateInfo imgui_allocInfo{m_imGuiCommandPools[i].GetHandle(),
                                                              vk::CommandBufferLevel::ePrimary, 1};

                try {
                    m_commandBuffers[i] = gfx::CommandBuffer{
                        m_logicalDevice->GetHandle(), fmt::format("Win-{} CommandBuffer{}", m_config->m_windowTitle, i),
                        m_graphicsQueue,
                        std::move(m_logicalDevice->GetHandle().allocateCommandBuffersUnique(allocInfo)[0])};
                    m_imGuiCommandBuffers[i] = gfx::CommandBuffer{
                        m_logicalDevice->GetHandle(),
                        fmt::format("Win-{} ImGuiCommandBuffer{}", m_config->m_windowTitle, i), m_graphicsQueue,
                        std::move(m_logicalDevice->GetHandle().allocateCommandBuffersUnique(imgui_allocInfo)[0])};
                } catch (vk::SystemError& e) {
                    spdlog::critical("Could not allocate command buffers ({}).", e.what());
                    throw std::runtime_error("Could not allocate command buffers.");
                }

                m_cmdBufferFences[i] =
                    gfx::Fence{m_logicalDevice->GetHandle(),
                               fmt::format("Win-{} CommandBufferFence{}", m_config->m_windowTitle, i),
                               m_logicalDevice->GetHandle().createFenceUnique(fenceCreateInfo)};
            }
        }
    }

    void VKWindow::DestroySwapchainImages()
    {
        m_swapchainFramebuffers.clear();
        m_swapchainRenderPass = gfx::RenderPass{};
    }

    void VKWindow::ReleaseVulkan()
    {
        m_logicalDevice->GetHandle().waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_cmdBufferFences.clear();
        m_imageAvailableSemaphore = gfx::Semaphore{};
        m_dataAvailableSemaphore = gfx::Semaphore{};
        m_renderingFinishedSemaphore = gfx::Semaphore{};
        m_commandBuffers.clear();
        m_imGuiCommandBuffers.clear();
        m_commandPools.clear();

        m_imGuiRenderPass = gfx::RenderPass{};
        m_imguiDescPool = gfx::DescriptorPool{};
        m_imGuiCommandPools.clear();

        DestroySwapchainImages();
        m_swapchain = gfx::Swapchain{};
        m_logicalDevice.reset();
        m_surface = gfx::Surface{};
    }

    std::pair<unsigned int, vk::Format> VKWindow::FindSupportedDepthFormat() const
    {
        constexpr std::size_t BITS_RBG8 = 24;
        std::vector<std::pair<unsigned int, vk::Format>> candidates;
        if (m_config->m_depthBufferBits == 16 && m_config->m_stencilBufferBits == 0) {
            candidates.emplace_back(std::make_pair(2, vk::Format::eD16Unorm));
        }
        if (m_config->m_depthBufferBits <= BITS_RBG8 && m_config->m_stencilBufferBits == 0) {
            candidates.emplace_back(std::make_pair(4, vk::Format::eX8D24UnormPack32));
        }
        if (m_config->m_depthBufferBits <= 32 && m_config->m_stencilBufferBits == 0) {
            candidates.emplace_back(std::make_pair(4, vk::Format::eD32Sfloat));
        }
        if (m_config->m_depthBufferBits == 0 && m_config->m_stencilBufferBits <= 8) {
            candidates.emplace_back(std::make_pair(1, vk::Format::eS8Uint));
        }
        if (m_config->m_depthBufferBits <= 16 && m_config->m_stencilBufferBits <= 8) {
            candidates.emplace_back(std::make_pair(3, vk::Format::eD16UnormS8Uint));
        }
        if (m_config->m_depthBufferBits <= BITS_RBG8 && m_config->m_stencilBufferBits <= 8) {
            candidates.emplace_back(std::make_pair(4, vk::Format::eD24UnormS8Uint));
        }
        if (m_config->m_depthBufferBits <= 32 && m_config->m_stencilBufferBits <= 8) {
            candidates.emplace_back(std::make_pair(5, vk::Format::eD32SfloatS8Uint));
        }

        return m_logicalDevice->FindSupportedFormat(candidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    /**
     * Shows the window.
     */
    void VKWindow::ShowWindow() const
    {
        glfwShowWindow(m_window);
    }

    /**
     *  Closes the window.
     */
    void VKWindow::CloseWindow() const { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }

    void VKWindow::ReleaseWindow()
    {
        if (m_window != nullptr) { glfwDestroyWindow(m_window); }
        m_window = nullptr;
    }

    void VKWindow::PrepareFrame()
    {
        auto result = m_logicalDevice->GetHandle().acquireNextImageKHR(
            m_swapchain.GetHandle(), std::numeric_limits<std::uint64_t>::max(), m_imageAvailableSemaphore.GetHandle(), vk::Fence());
        m_currentlyRenderedImage = result.value;

        // NOLINTNEXTLINE
        if (result.result == vk::Result::eErrorOutOfDateKHR) {
            // NOLINTNEXTLINE
            RecreateSwapChain();
            return;
        }
        if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
            spdlog::critical("Could not acquire swap chain image ({}).", vk::to_string(result.result));
            throw std::runtime_error("Could not acquire swap chain image.");
        }

        // Start the Dear ImGui frame
        if (ApplicationBase::instance().IsGUIMode()) {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }
    }

    void VKWindow::DrawCurrentCommandBuffer() const
    {
        {
            auto syncResult =
                m_logicalDevice->GetHandle().getFenceStatus(m_cmdBufferFences[m_currentlyRenderedImage].GetHandle());
            while (syncResult == vk::Result::eTimeout || syncResult == vk::Result::eNotReady) {
                syncResult = m_logicalDevice->GetHandle().waitForFences(
                    m_cmdBufferFences[m_currentlyRenderedImage].GetHandle(), VK_TRUE, defaultFenceTimeout);
            }

            if (syncResult != vk::Result::eSuccess) {
                spdlog::critical("Error synchronizing command buffer. ({}).", vk::to_string(syncResult));
                throw std::runtime_error("Error synchronizing command buffer.");
            }

            m_logicalDevice->GetHandle().resetFences(m_cmdBufferFences[m_currentlyRenderedImage].GetHandle());
        }

        // Rendering
        if (ApplicationBase::instance().IsGUIMode()) {
            ImGui::Render();

            {
                m_logicalDevice->GetHandle().resetCommandPool(m_imGuiCommandPools[m_currentlyRenderedImage].GetHandle(),
                                                              vk::CommandPoolResetFlags());
                vk::CommandBufferBeginInfo cmdBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
                m_imGuiCommandBuffers[m_currentlyRenderedImage].Begin(cmdBufferBeginInfo);
            }

            {
                vk::RenderPassBeginInfo imGuiRenderPassBeginInfo{
                    m_imGuiRenderPass.GetHandle(), m_swapchainFramebuffers[m_currentlyRenderedImage].GetHandle(),
                    vk::Rect2D(vk::Offset2D(0, 0), m_vkSurfaceExtend), 0, nullptr };
                m_imGuiCommandBuffers[m_currentlyRenderedImage].GetHandle().beginRenderPass(imGuiRenderPassBeginInfo, vk::SubpassContents::eInline);
            }

            // Record ImGui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_imGuiCommandBuffers[m_currentlyRenderedImage].GetHandle());

            m_imGuiCommandBuffers[m_currentlyRenderedImage].GetHandle().endRenderPass();
            m_imGuiCommandBuffers[m_currentlyRenderedImage].End();
        }

        std::array<vk::PipelineStageFlags, 2> waitStages{ vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTopOfPipe };
        std::array<vk::Semaphore, 2> waitSemaphores{m_imageAvailableSemaphore.GetHandle(), m_dataAvailableSemaphore.GetHandle()};
        std::array<vk::CommandBuffer, 2> submitCmdBuffers{ m_commandBuffers[m_currentlyRenderedImage].GetHandle(), m_imGuiCommandBuffers[m_currentlyRenderedImage].GetHandle() };
        vk::SubmitInfo submitInfo{ 2, waitSemaphores.data(), waitStages.data(), 2, submitCmdBuffers.data(), 1, m_renderingFinishedSemaphore.GetHandlePtr() };

        const auto& graphicsQueue = m_logicalDevice->GetQueue(m_graphicsQueue, 0);
        {
            QUEUE_REGION(graphicsQueue, "Draw");
            graphicsQueue.Submit(submitInfo, m_cmdBufferFences[m_currentlyRenderedImage]);
        }
    }

    void VKWindow::SubmitFrame()
    {
        const auto& graphicsQueue = m_logicalDevice->GetQueue(m_graphicsQueue, 0);
        QUEUE_REGION(graphicsQueue, "Present");
        std::array<vk::SwapchainKHR, 1> swapchains = {m_swapchain.GetHandle()};
        vk::PresentInfoKHR presentInfo{1, m_renderingFinishedSemaphore.GetHandlePtr(), 1, m_swapchain.GetHandlePtr(),
                                       &m_currentlyRenderedImage}; //<- wait on these semaphores
        auto result = m_logicalDevice->GetQueue(m_graphicsQueue, 0).Present(presentInfo);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_frameBufferResize) {
            m_frameBufferResize = false;
            RecreateSwapChain();

            try {
                // TODO: notify all resources depending on this...
                ApplicationBase::instance().OnResize(static_cast<int>(m_config->m_windowWidth),
                                                     static_cast<int>(m_config->m_windowHeight), this);
            } catch (std::runtime_error& e) {
                spdlog::critical("Could not reacquire resources after resize: {}", e.what());
                throw std::runtime_error("Could not reacquire resources after resize.");
            }
        } else if (result != vk::Result::eSuccess) {
            spdlog::critical("Could not present swap chain image ({}).", vk::to_string(result));
            throw std::runtime_error("Could not present swap chain image.");
        }

        ++m_frameCount;
    }

    void VKWindow::UpdatePrimaryCommandBuffers(
        const function_view<void(const gfx::CommandBuffer& commandBuffer, std::size_t cmdBufferIndex)>& fillFunc) const
    {
        for (std::size_t i = 0U; i < m_commandBuffers.size(); ++i) {
            {
                auto syncResult = vk::Result::eTimeout;
                while (syncResult == vk::Result::eTimeout) {
                    syncResult = m_logicalDevice->GetHandle().waitForFences(m_cmdBufferFences[i].GetHandle(), VK_TRUE,
                                                                            defaultFenceTimeout);
                }

                if (syncResult != vk::Result::eSuccess) {
                    spdlog::critical("Error synchronizing command buffers. ({}).", vk::to_string(syncResult));
                    throw std::runtime_error("Error synchronizing command buffers.");
                }
            }

            m_logicalDevice->GetHandle().resetCommandPool(m_commandPools[i].GetHandle(), vk::CommandPoolResetFlags());

            vk::CommandBufferBeginInfo cmdBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            m_commandBuffers[i].Begin(cmdBufferBeginInfo);

            fillFunc(m_commandBuffers[i], i);

            m_commandBuffers[i].End();
        }
    }

    void VKWindow::BeginSwapchainRenderPass(std::size_t cmdBufferIndex) const
    {
        std::array<vk::ClearValue, 2> clearColor;
        clearColor[0].setColor(vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}});
        clearColor[1].setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0});
        vk::RenderPassBeginInfo renderPassBeginInfo{m_swapchainRenderPass.GetHandle(),
                                                    m_swapchainFramebuffers[cmdBufferIndex].GetHandle(),
                                                    vk::Rect2D(vk::Offset2D(0, 0), m_vkSurfaceExtend),
                                                    static_cast<std::uint32_t>(clearColor.size()), clearColor.data()};
        m_commandBuffers[cmdBufferIndex].GetHandle().beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    }

    void VKWindow::EndSwapchainRenderPass(std::size_t cmdBufferIndex) const
    {
        m_commandBuffers[cmdBufferIndex].GetHandle().endRenderPass();
    }

    /**
     * Shows a question message box.
     * @param title the message boxes title
     * @param content the message boxes content
     * @return returns <code>true</code> if the user pressed 'yes' <code>false</code> if 'no'
     */
    bool VKWindow::MessageBoxQuestion(const std::string& title, const std::string& content) const
    {
        return MessageBoxA(glfwGetWin32Window(m_window), content.c_str(), title.c_str(), MB_YESNO) == IDYES;
    }

    bool VKWindow::IsMouseButtonPressed(int button) const { return glfwGetMouseButton(m_window, button) == GLFW_PRESS; }

    bool VKWindow::IsKeyPressed(int key) const
    {
        return glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    void VKWindow::WindowPosCallback(int xpos, int ypos) const
    {
        m_config->m_windowLeft = xpos;
        m_config->m_windowTop = ypos;
    }

    void VKWindow::WindowSizeCallback(int width, int height)
    {
        spdlog::info("Got window resize event ({}, {}) ...", width, height);
        if (width == 0 || height == 0) { return; }

        spdlog::debug("Begin HandleResize()");

        m_config->m_windowWidth = width;
        m_config->m_windowHeight = height;
        m_frameBufferResize = true;
    }

    void VKWindow::WindowFocusCallback(int focused)
    {
        m_focused = focused == GLFW_TRUE;
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    // NOLINTNEXTLINE
    void VKWindow::WindowCloseCallback() const
    {
        spdlog::info("Got close event ...");
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    // NOLINTNEXTLINE
    void VKWindow::FramebufferSizeCallback(int width, int height) const
    {
        spdlog::info("Got framebuffer resize event ({}, {}) ...", width, height);
    }

    void VKWindow::WindowIconifyCallback(int iconified)
    {
        if (iconified == GLFW_TRUE) {
            ApplicationBase::instance().SetPause(true);
            m_minimized = true;
            m_maximized = false;
        } else {
            if (m_minimized) { ApplicationBase::instance().SetPause(false); }
            m_minimized = false;
            m_maximized = false;
        }
    }

    void VKWindow::MouseButtonCallback(int button, int action, int mods)
    {
        if (m_mouseInWindow) {
            ApplicationBase::instance().HandleMouse(button, action, mods, 0.0f, this);
        }
    }

    void VKWindow::CursorPosCallback(double, double)
    {
        if (m_mouseInWindow) {
            ApplicationBase::instance().HandleMouse(-1, 0, 0, 0.0f, this);
        }
    }

    void VKWindow::CursorEnterCallback(int entered)
    {
        if (entered != 0) {
            double xpos;
            double ypos;
            glfwGetCursorPos(m_window, &xpos, &ypos);
            m_currMousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
            m_mouseInWindow = true;
        } else {
            m_mouseInWindow = false;
        }
    }

    void VKWindow::ScrollCallback(double, double yoffset)
    {
        if (m_mouseInWindow) {
            constexpr float SCROLL_SCALE = 50.0f;
            ApplicationBase::instance().HandleMouse(-1, 0, 0, SCROLL_SCALE * static_cast<float>(yoffset), this);
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
    // NOLINTNEXTLINE
    void VKWindow::DropCallback(int, const char**) const
    {
        throw std::runtime_error("File dropping not implemented.");
    }

    void VKWindow::glfwErrorCallback(int error, const char* description)
    {
        spdlog::critical("An GLFW error occurred ({}):\n{}", error, description);
    }

    void VKWindow::SetMousePosition(double xpos, double ypos)
    {
        m_prevMousePosition = m_currMousePosition;
        m_currMousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
        m_relativeMousePosition = m_currMousePosition - m_prevMousePosition;

        auto mousePos = glm::dvec2{ xpos, ypos };
        mousePos /=
            glm::dvec2(static_cast<double>(m_vkSurfaceExtend.width), static_cast<double>(m_vkSurfaceExtend.height));
        m_currMousePositionNormalized.x = static_cast<float>(2.0 * mousePos.x - 1.0);
        m_currMousePositionNormalized.y = static_cast<float>(-2.0 * mousePos.y + 1.0);
    }

    void VKWindow::glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->WindowPosCallback(xpos, ypos);
    }

    void VKWindow::glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->WindowSizeCallback(width, height);
    }

    void VKWindow::glfwWindowFocusCallback(GLFWwindow* window, int focused)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->WindowFocusCallback(focused);
    }

    void VKWindow::glfwWindowCloseCallback(GLFWwindow* window)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->WindowCloseCallback();
    }

    void VKWindow::glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->FramebufferSizeCallback(width, height);
    }

    void VKWindow::glfwWindowIconifyCallback(GLFWwindow* window, int iconified)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->WindowIconifyCallback(iconified);
    }

    void VKWindow::glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        if (!win->m_imguiInitialized) return;

        // ImGui_ImplGlfw_MouseButtonCallback(win->m_glfwWindowData, button, action, mods);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            win->MouseButtonCallback(button, action, mods);
        }
    }

    void VKWindow::glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->SetMousePosition(xpos, ypos);

        if (!win->m_imguiInitialized) return;
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            win->CursorPosCallback(xpos, ypos);
        }
    }

    void VKWindow::glfwCursorEnterCallback(GLFWwindow* window, int entered)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->CursorEnterCallback(entered);
    }

    void VKWindow::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        if (!win->m_imguiInitialized) return;

        // ImGui_ImplGlfw_ScrollCallback(win->m_glfwWindowData, xoffset, yoffset);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            win->ScrollCallback(xoffset, yoffset);
        }
    }

    void VKWindow::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        if (!win->m_imguiInitialized) return;

        // ImGui_ImplGlfw_KeyCallback(win->m_glfwWindowData, key, scancode, action, mods);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            win->KeyCallback(key, scancode, action, mods);
        }
    }

    void VKWindow::glfwCharCallback(GLFWwindow* window, unsigned int codepoint)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        if (!win->m_imguiInitialized) return;

        // ImGui_ImplGlfw_CharCallback(win->m_glfwWindowData, codepoint);

        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            win->CharCallback(codepoint);
        }
    }

    void VKWindow::glfwCharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        if (!win->m_imguiInitialized) return;
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            win->CharModsCallback(codepoint, mods);
        }
    }

    void VKWindow::glfwDropCallback(GLFWwindow* window, int count, const char** paths)
    {
        auto win = reinterpret_cast<VKWindow*>(glfwGetWindowUserPointer(window)); // NOLINT
        win->DropCallback(count, paths);
    }

}
