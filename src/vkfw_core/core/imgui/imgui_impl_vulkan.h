// dear imgui: Renderer for Vulkan
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Missing features:
//  [ ] Renderer: User texture binding. Changes of ImTextureID aren't supported by this binding! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

#include <vulkan/vulkan.h>
#include <cstddef>

#include <imgui.h>

struct ImGui_ImplVulkan_InternalInfo;

// Please zero-clear before use.
struct ImGui_ImplVulkan_InitInfo
{
    VkInstance                      Instance = nullptr;
    VkPhysicalDevice PhysicalDevice = nullptr;
    VkDevice Device = nullptr;
    uint32_t QueueFamily = 0;
    VkQueue Queue = nullptr;
    VkPipelineCache PipelineCache = 0;
    VkDescriptorPool DescriptorPool = 0;
    const VkAllocationCallbacks* Allocator = nullptr;
    ImGui_ImplVulkan_InternalInfo*  m_internal = nullptr;
};

// Called by user code
IMGUI_IMPL_API bool     ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass, std::size_t numBackbuffers);
IMGUI_IMPL_API void     ImGui_ImplVulkan_Shutdown(ImGui_ImplVulkan_InitInfo* info);
IMGUI_IMPL_API void     ImGui_ImplVulkan_NewFrame(ImGui_ImplVulkan_InitInfo* info);
IMGUI_IMPL_API void     ImGui_ImplVulkan_RenderDrawData(ImGui_ImplVulkan_InitInfo* info, ImDrawData* draw_data, VkCommandBuffer command_buffer);
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateFontsTexture(ImGui_ImplVulkan_InitInfo* info, VkCommandBuffer command_buffer);
IMGUI_IMPL_API void     ImGui_ImplVulkan_InvalidateFontUploadObjects(ImGui_ImplVulkan_InitInfo* info);

// Called by ImGui_ImplVulkan_Init() might be useful elsewhere.
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateDeviceObjects(ImGui_ImplVulkan_InitInfo* info);
IMGUI_IMPL_API void     ImGui_ImplVulkan_InvalidateDeviceObjects(ImGui_ImplVulkan_InitInfo* info);


//-------------------------------------------------------------------------
// Internal / Miscellaneous Vulkan Helpers
//-------------------------------------------------------------------------
// You probably do NOT need to use or care about those functions.
// Those functions only exist because:
//   1) they facilitate the readability and maintenance of the multiple main.cpp examples files.
//   2) the upcoming multi-viewport feature will need them internally.
// Generally we avoid exposing any kind of superfluous high-level helpers in the bindings,
// but it is too much code to duplicate everywhere so we exceptionally expose them.
// Your application/engine will likely already have code to setup all that stuff (swap chain, render pass, frame buffers, etc.).
// You may read this code to learn about Vulkan, but it is recommended you use you own custom tailored code to do equivalent work.
// (those functions do not interact with any of the state used by the regular ImGui_ImplVulkan_XXX functions)
//-------------------------------------------------------------------------

struct ImGui_ImplVulkanH_FrameData;
struct ImGui_ImplVulkanH_WindowData;

// Helper structure to hold the data needed by one rendering context into one OS window
struct ImGui_ImplVulkanH_WindowData
{
    int                 Width;
    int                 Height;
    VkSwapchainKHR      Swapchain;
    VkSurfaceKHR        Surface;
    VkSurfaceFormatKHR  SurfaceFormat;
    VkPresentModeKHR    PresentMode;
    VkRenderPass        RenderPass;

    IMGUI_IMPL_API ImGui_ImplVulkanH_WindowData();
};

