// dear imgui: Renderer for Vulkan
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Missing features:
//  [ ] Renderer: User texture binding. Changes of ImTextureID aren't supported by this binding! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-08-25: Vulkan: Fixed mishandled VkSurfaceCapabilitiesKHR::maxImageCount=0 case.
//  2018-06-22: Inverted the parameters to ImGui_ImplVulkan_RenderDrawData() to be consistent with other bindings.
//  2018-06-08: Misc: Extracted imgui_impl_vulkan.cpp/.h away from the old combined GLFW+Vulkan example.
//  2018-06-08: Vulkan: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-03-03: Vulkan: Various refactor, created a couple of ImGui_ImplVulkanH_XXX helper that the example can use and that viewport support will use.
//  2018-03-01: Vulkan: Renamed ImGui_ImplVulkan_Init_Info to ImGui_ImplVulkan_InitInfo and fields to match more closely Vulkan terminology.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback, ImGui_ImplVulkan_Render() calls ImGui_ImplVulkan_RenderDrawData() itself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2017-05-15: Vulkan: Fix scissor offset being negative. Fix new Vulkan validation warnings. Set required depth member for buffer image copy.
//  2016-11-13: Vulkan: Fix validation layer warnings and errors and redeclare gl_PerVertex.
//  2016-10-18: Vulkan: Add location decorators & change to use structs as in/out in glsl, update embedded spv (produced with glslangValidator -x). Null the released resources.
//  2016-08-27: Vulkan: Fix Vulkan example for use when a depth buffer is active.

#include <cstddef>

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include <cstdio>
#include "main.h"


// Frame data
struct FrameDataForRender
{
    VkDeviceMemory  VertexBufferMemory;
    VkDeviceMemory  IndexBufferMemory;
    VkDeviceSize    VertexBufferSize;
    VkDeviceSize    IndexBufferSize;
    VkBuffer        VertexBuffer;
    VkBuffer        IndexBuffer;
};


struct ImGui_ImplVulkan_InternalInfo
{
    VkRenderPass                 g_RenderPass = VK_NULL_HANDLE;

    VkDeviceSize                 g_BufferMemoryAlignment = 256;
    VkPipelineCreateFlags        g_PipelineCreateFlags = 0;

    VkDescriptorSetLayout        g_DescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout             g_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet              g_DescriptorSet = VK_NULL_HANDLE;
    VkPipeline                   g_Pipeline = VK_NULL_HANDLE;

    int                    g_FrameIndex = 0;
    std::vector<FrameDataForRender>     g_FramesDataBuffers;

    // Font data
    VkSampler              g_FontSampler = VK_NULL_HANDLE;
    VkDeviceMemory         g_FontMemory = VK_NULL_HANDLE;
    VkImage                g_FontImage = VK_NULL_HANDLE;
    VkImageView            g_FontView = VK_NULL_HANDLE;
    VkDeviceMemory         g_UploadBufferMemory = VK_NULL_HANDLE;
    VkBuffer               g_UploadBuffer = VK_NULL_HANDLE;
};

/** Copied from ImGui example. */
static void check_vk_result(VkResult err)
{
    if (err == 0) { return; }
    spdlog::error("VkResult {}", err);
    if (err < 0) { throw std::runtime_error("Vulkan Error."); }
}

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
// NOLINTNEXTLINE
static uint32_t __glsl_shader_vert_spv[] =
{
    0x07230203, 0x00010000, 0x00080001, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, // NOLINT
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, // NOLINT
    0x000a000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000f, 0x00000015, // NOLINT
    0x0000001b, 0x0000001c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, // NOLINT
    0x00000000, 0x00030005, 0x00000009, 0x00000000, 0x00050006, 0x00000009, 0x00000000, 0x6f6c6f43, // NOLINT
    0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005, 0x0000000b, 0x0074754f, // NOLINT
    0x00040005, 0x0000000f, 0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015, 0x00565561, 0x00060005, // NOLINT
    0x00000019, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000019, 0x00000000, // NOLINT
    0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000, 0x00040005, 0x0000001c, // NOLINT
    0x736f5061, 0x00000000, 0x00060005, 0x0000001e, 0x73755075, 0x6e6f4368, 0x6e617473, 0x00000074, // NOLINT
    0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006, 0x0000001e, 0x00000001, // NOLINT
    0x61725475, 0x616c736e, 0x00006574, 0x00030005, 0x00000020, 0x00006370, 0x00040047, 0x0000000b, // NOLINT
    0x0000001e, 0x00000000, 0x00040047, 0x0000000f, 0x0000001e, 0x00000002, 0x00040047, 0x00000015, // NOLINT
    0x0000001e, 0x00000001, 0x00050048, 0x00000019, 0x00000000, 0x0000000b, 0x00000000, 0x00030047, // NOLINT
    0x00000019, 0x00000002, 0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e, // NOLINT
    0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008, // NOLINT
    0x00030047, 0x0000001e, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, // NOLINT
    0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040017, // NOLINT
    0x00000008, 0x00000006, 0x00000002, 0x0004001e, 0x00000009, 0x00000007, 0x00000008, 0x00040020, // NOLINT
    0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015, // NOLINT
    0x0000000c, 0x00000020, 0x00000001, 0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020, // NOLINT
    0x0000000e, 0x00000001, 0x00000007, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000001, 0x00040020, // NOLINT
    0x00000011, 0x00000003, 0x00000007, 0x0004002b, 0x0000000c, 0x00000013, 0x00000001, 0x00040020, // NOLINT
    0x00000014, 0x00000001, 0x00000008, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020, // NOLINT
    0x00000017, 0x00000003, 0x00000008, 0x0003001e, 0x00000019, 0x00000007, 0x00040020, 0x0000001a, // NOLINT
    0x00000003, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014, // NOLINT
    0x0000001c, 0x00000001, 0x0004001e, 0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f, // NOLINT
    0x00000009, 0x0000001e, 0x0004003b, 0x0000001f, 0x00000020, 0x00000009, 0x00040020, 0x00000021, // NOLINT
    0x00000009, 0x00000008, 0x0004002b, 0x00000006, 0x00000028, 0x00000000, 0x0004002b, 0x00000006, // NOLINT
    0x00000029, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, // NOLINT
    0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011, 0x00000012, // NOLINT
    0x0000000b, 0x0000000d, 0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008, 0x00000016, // NOLINT
    0x00000015, 0x00050041, 0x00000017, 0x00000018, 0x0000000b, 0x00000013, 0x0003003e, 0x00000018, // NOLINT
    0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041, 0x00000021, 0x00000022, // NOLINT
    0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085, 0x00000008, // NOLINT
    0x00000024, 0x0000001d, 0x00000023, 0x00050041, 0x00000021, 0x00000025, 0x00000020, 0x00000013, // NOLINT
    0x0004003d, 0x00000008, 0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027, 0x00000024, // NOLINT
    0x00000026, 0x00050051, 0x00000006, 0x0000002a, 0x00000027, 0x00000000, 0x00050051, 0x00000006, // NOLINT
    0x0000002b, 0x00000027, 0x00000001, 0x00070050, 0x00000007, 0x0000002c, 0x0000002a, 0x0000002b, // NOLINT
    0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d, 0x0003003e, // NOLINT
    0x0000002d, 0x0000002c, 0x000100fd, 0x00010038                                                  // NOLINT
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
// NOLINTNEXTLINE
static uint32_t __glsl_shader_frag_spv[] =
{
    0x07230203, 0x00010000, 0x00080001, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, // NOLINT
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, // NOLINT
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010, // NOLINT
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, // NOLINT
    0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x0000726f, 0x00030005, 0x0000000b, 0x00000000, // NOLINT
    0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001, // NOLINT
    0x00005655, 0x00030005, 0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574, // NOLINT
    0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x0000001e, // NOLINT
    0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000000, 0x00040047, 0x00000016, 0x00000021, // NOLINT
    0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, // NOLINT
    0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, // NOLINT
    0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006, // NOLINT
    0x00000002, 0x0004001e, 0x0000000b, 0x00000007, 0x0000000a, 0x00040020, 0x0000000c, 0x00000001, // NOLINT
    0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000001, 0x00040015, 0x0000000e, 0x00000020, // NOLINT
    0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001, // NOLINT
    0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, // NOLINT
    0x00000001, 0x00000000, 0x0003001b, 0x00000014, 0x00000013, 0x00040020, 0x00000015, 0x00000000, // NOLINT
    0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000000, 0x0004002b, 0x0000000e, 0x00000018, // NOLINT
    0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036, 0x00000002, 0x00000004, // NOLINT
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000010, 0x00000011, 0x0000000d, // NOLINT
    0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017, // NOLINT
    0x00000016, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a, // NOLINT
    0x0000001b, 0x0000001a, 0x00050057, 0x00000007, 0x0000001c, 0x00000017, 0x0000001b, 0x00050085, // NOLINT
    0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd, // NOLINT
    0x00010038                                                                                      // NOLINT
};

static uint32_t ImGui_ImplVulkan_MemoryType(ImGui_ImplVulkan_InitInfo* vkinfo, VkMemoryPropertyFlags properties, uint32_t type_bits)
{
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(vkinfo->PhysicalDevice, &prop);
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++) {
        if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i)) { return i; } // NOLINT
    }
    // NOLINTNEXTLINE
    return 0xFFFFFFFF; // Unable to find memoryType
}

static void CreateOrResizeBuffer(ImGui_ImplVulkan_InitInfo* vkinfo, VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& p_buffer_size, size_t new_size, VkBufferUsageFlagBits usage)
{
    VkResult err;
    if (buffer != VK_NULL_HANDLE) { vkDestroyBuffer(vkinfo->Device, buffer, vkinfo->Allocator); }
    if (buffer_memory != 0) { vkFreeMemory(vkinfo->Device, buffer_memory, vkinfo->Allocator); }

    VkDeviceSize vertex_buffer_size_aligned = ((new_size - 1) / vkinfo->internal_->g_BufferMemoryAlignment + 1) * vkinfo->internal_->g_BufferMemoryAlignment;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = vertex_buffer_size_aligned;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    err = vkCreateBuffer(vkinfo->Device, &buffer_info, vkinfo->Allocator, &buffer);
    check_vk_result(err);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(vkinfo->Device, buffer, &req);
    vkinfo->internal_->g_BufferMemoryAlignment = (vkinfo->internal_->g_BufferMemoryAlignment > req.alignment) ? vkinfo->internal_->g_BufferMemoryAlignment : req.alignment;
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(vkinfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
    err = vkAllocateMemory(vkinfo->Device, &alloc_info, vkinfo->Allocator, &buffer_memory);
    check_vk_result(err);

    err = vkBindBufferMemory(vkinfo->Device, buffer, buffer_memory, 0);
    check_vk_result(err);
    p_buffer_size = new_size;
}

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplVulkan_RenderDrawData(ImGui_ImplVulkan_InitInfo* vkinfo, ImDrawData* draw_data, VkCommandBuffer command_buffer)
{
    VkResult err;
    if (draw_data->TotalVtxCount == 0) { return; }

    FrameDataForRender* fd = &vkinfo->internal_->g_FramesDataBuffers[vkinfo->internal_->g_FrameIndex];
    vkinfo->internal_->g_FrameIndex = ((static_cast<int>(static_cast<std::size_t>(vkinfo->internal_->g_FrameIndex) + 1)) % vkinfo->internal_->g_FramesDataBuffers.size());

    // Create the Vertex and Index buffers:
    size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if (fd->VertexBuffer == 0 || fd->VertexBufferSize < vertex_size) {
        CreateOrResizeBuffer(vkinfo, fd->VertexBuffer, fd->VertexBufferMemory, fd->VertexBufferSize, vertex_size,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }
    if (fd->IndexBuffer == 0 || fd->IndexBufferSize < index_size) {
        CreateOrResizeBuffer(vkinfo, fd->IndexBuffer, fd->IndexBufferMemory, fd->IndexBufferSize, index_size,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    // Upload Vertex and index Data:
    {
        ImDrawVert* vtx_dst = nullptr;
        ImDrawIdx* idx_dst = nullptr;
        err = vkMapMemory(vkinfo->Device, fd->VertexBufferMemory, 0, vertex_size, 0, (void**)(&vtx_dst)); // NOLINT
        check_vk_result(err);
        err = vkMapMemory(vkinfo->Device, fd->IndexBufferMemory, 0, index_size, 0, (void**)(&idx_dst)); // NOLINT
        check_vk_result(err);
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n]; // NOLINT
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size; // NOLINT
            idx_dst += cmd_list->IdxBuffer.Size; // NOLINT
        }
        std::array<VkMappedMemoryRange, 2> range = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = fd->VertexBufferMemory;
        range[0].size = VK_WHOLE_SIZE;
        range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[1].memory = fd->IndexBufferMemory;
        range[1].size = VK_WHOLE_SIZE;
        err = vkFlushMappedMemoryRanges(vkinfo->Device, 2, range.data());
        check_vk_result(err);
        vkUnmapMemory(vkinfo->Device, fd->VertexBufferMemory);
        vkUnmapMemory(vkinfo->Device, fd->IndexBufferMemory);
    }

    // Bind pipeline and descriptor sets:
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkinfo->internal_->g_Pipeline);
        std::array<VkDescriptorSet, 1> desc_set = {vkinfo->internal_->g_DescriptorSet};
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkinfo->internal_->g_PipelineLayout, 0, 1, desc_set.data(), 0, nullptr);
    }

    // Bind Vertex And Index Buffer:
    {
        std::array<VkBuffer, 1> vertex_buffers = {fd->VertexBuffer};
        std::array<VkDeviceSize, 1> vertex_offset = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers.data(), vertex_offset.data());
        vkCmdBindIndexBuffer(command_buffer, fd->IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
    }

    // Setup viewport:
    {
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = draw_data->DisplaySize.x;
        viewport.height = draw_data->DisplaySize.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
    {
        std::array<float, 2> scale = {};
        scale[0] = 2.0f / draw_data->DisplaySize.x;
        scale[1] = 2.0f / draw_data->DisplaySize.y;
        std::array<float, 2> translate = {};
        translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
        translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
        vkCmdPushConstants(command_buffer, vkinfo->internal_->g_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale.data());
        vkCmdPushConstants(command_buffer, vkinfo->internal_->g_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate.data());
    }

    // Render the command lists:
    int vtx_offset = 0;
    int idx_offset = 0;
    ImVec2 display_pos = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n]; // NOLINT
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Apply scissor/clipping rectangle
                // FIXME: We could clamp width/height based on clamped min/max values.
                VkRect2D scissor;
                scissor.offset.x = (int32_t)(pcmd->ClipRect.x - display_pos.x) > 0 ? (int32_t)(pcmd->ClipRect.x - display_pos.x) : 0;
                scissor.offset.y = (int32_t)(pcmd->ClipRect.y - display_pos.y) > 0 ? (int32_t)(pcmd->ClipRect.y - display_pos.y) : 0;
                scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // FIXME: Why +1 here?
                vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                // Draw
                vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
            }
            idx_offset += static_cast<int>(pcmd->ElemCount);
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

bool ImGui_ImplVulkan_CreateFontsTexture(ImGui_ImplVulkan_InitInfo* vkinfo, VkCommandBuffer command_buffer)
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width;
    int height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = static_cast<std::size_t>(width)*static_cast<std::size_t>(height)*4ULL*sizeof(char);

    VkResult err;

    // Create the Image:
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = static_cast<unsigned int>(VK_IMAGE_USAGE_SAMPLED_BIT) | static_cast<unsigned int>(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        err = vkCreateImage(vkinfo->Device, &info, vkinfo->Allocator, &vkinfo->internal_->g_FontImage);
        check_vk_result(err);
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(vkinfo->Device, vkinfo->internal_->g_FontImage, &req);
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(vkinfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(vkinfo->Device, &alloc_info, vkinfo->Allocator, &vkinfo->internal_->g_FontMemory);
        check_vk_result(err);
        err = vkBindImageMemory(vkinfo->Device, vkinfo->internal_->g_FontImage, vkinfo->internal_->g_FontMemory, 0);
        check_vk_result(err);
    }

    // Create the Image View:
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = vkinfo->internal_->g_FontImage;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        err = vkCreateImageView(vkinfo->Device, &info, vkinfo->Allocator, &vkinfo->internal_->g_FontView);
        check_vk_result(err);
    }

    // Update the Descriptor Set:
    {
        std::array<VkDescriptorImageInfo, 1> desc_image = {};
        desc_image[0].sampler = vkinfo->internal_->g_FontSampler;
        desc_image[0].imageView = vkinfo->internal_->g_FontView;
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        std::array<VkWriteDescriptorSet, 1> write_desc = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = vkinfo->internal_->g_DescriptorSet;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image.data();
        vkUpdateDescriptorSets(vkinfo->Device, 1, write_desc.data(), 0, nullptr);
    }

    // Create the Upload Buffer:
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = upload_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateBuffer(vkinfo->Device, &buffer_info, vkinfo->Allocator, &vkinfo->internal_->g_UploadBuffer);
        check_vk_result(err);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(vkinfo->Device, vkinfo->internal_->g_UploadBuffer, &req);
        vkinfo->internal_->g_BufferMemoryAlignment = (vkinfo->internal_->g_BufferMemoryAlignment > req.alignment) ? vkinfo->internal_->g_BufferMemoryAlignment : req.alignment;
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(vkinfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(vkinfo->Device, &alloc_info, vkinfo->Allocator, &vkinfo->internal_->g_UploadBufferMemory);
        check_vk_result(err);
        err = vkBindBufferMemory(vkinfo->Device, vkinfo->internal_->g_UploadBuffer, vkinfo->internal_->g_UploadBufferMemory, 0);
        check_vk_result(err);
    }

    // Upload to Buffer:
    {
        char* map = nullptr;
        err = vkMapMemory(vkinfo->Device, vkinfo->internal_->g_UploadBufferMemory, 0, upload_size, 0, (void**)(&map)); // NOLINT
        check_vk_result(err);
        memcpy(map, pixels, upload_size);
        std::array<VkMappedMemoryRange, 1> range = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = vkinfo->internal_->g_UploadBufferMemory;
        range[0].size = upload_size;
        err = vkFlushMappedMemoryRanges(vkinfo->Device, 1, range.data());
        check_vk_result(err);
        vkUnmapMemory(vkinfo->Device, vkinfo->internal_->g_UploadBufferMemory);
    }

    // Copy to Image:
    {
        std::array<VkImageMemoryBarrier, 1> copy_barrier = {};
        copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].image = vkinfo->internal_->g_FontImage;
        copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_barrier[0].subresourceRange.levelCount = 1;
        copy_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, copy_barrier.data());

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(command_buffer, vkinfo->internal_->g_UploadBuffer, vkinfo->internal_->g_FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        std::array<VkImageMemoryBarrier, 1> use_barrier = {};
        use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].image = vkinfo->internal_->g_FontImage;
        use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        use_barrier[0].subresourceRange.levelCount = 1;
        use_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, use_barrier.data());
    }

    // Store our identifier
    io.Fonts->TexID = (ImTextureID)(intptr_t)vkinfo->internal_->g_FontImage; // NOLINT

    return true;
}

bool ImGui_ImplVulkan_CreateDeviceObjects(ImGui_ImplVulkan_InitInfo* vkinfo)
{
    VkResult err;
    VkShaderModule vert_module;
    VkShaderModule frag_module;

    // Create The Shader Modules:
    {
        VkShaderModuleCreateInfo vert_info = {};
        vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vert_info.codeSize = sizeof(__glsl_shader_vert_spv);
        vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;
        err = vkCreateShaderModule(vkinfo->Device, &vert_info, vkinfo->Allocator, &vert_module);
        check_vk_result(err);
        VkShaderModuleCreateInfo frag_info = {};
        frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        frag_info.codeSize = sizeof(__glsl_shader_frag_spv);
        frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;
        err = vkCreateShaderModule(vkinfo->Device, &frag_info, vkinfo->Allocator, &frag_module);
        check_vk_result(err);
    }

    if (vkinfo->internal_->g_FontSampler == 0)
    {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
        err = vkCreateSampler(vkinfo->Device, &info, vkinfo->Allocator, &vkinfo->internal_->g_FontSampler);
        check_vk_result(err);
    }

    if (vkinfo->internal_->g_DescriptorSetLayout == 0)
    {
        std::array<VkSampler, 1> sampler = { vkinfo->internal_->g_FontSampler};
        std::array<VkDescriptorSetLayoutBinding, 1> binding = {};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[0].pImmutableSamplers = sampler.data();
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = binding.data();
        err = vkCreateDescriptorSetLayout(vkinfo->Device, &info, vkinfo->Allocator, &vkinfo->internal_->g_DescriptorSetLayout);
        check_vk_result(err);
    }

    // Create Descriptor Set:
    {
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = vkinfo->DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &vkinfo->internal_->g_DescriptorSetLayout;
        err = vkAllocateDescriptorSets(vkinfo->Device, &alloc_info, &vkinfo->internal_->g_DescriptorSet);
        check_vk_result(err);
    }

    if (vkinfo->internal_->g_PipelineLayout == 0)
    {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        std::array<VkPushConstantRange, 1> push_constants = {};
        push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset = sizeof(float) * 0;
        push_constants[0].size = sizeof(float) * 4;
        std::array<VkDescriptorSetLayout, 1> set_layout = { vkinfo->internal_->g_DescriptorSetLayout };
        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = set_layout.data();
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = push_constants.data();
        err = vkCreatePipelineLayout(vkinfo->Device, &layout_info, vkinfo->Allocator, &vkinfo->internal_->g_PipelineLayout);
        check_vk_result(err);
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> stage = {};
    stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage[0].module = vert_module;
    stage[0].pName = "main";
    stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage[1].module = frag_module;
    stage[1].pName = "main";

    std::array<VkVertexInputBindingDescription, 1> binding_desc = {};
    binding_desc[0].stride = sizeof(ImDrawVert);
    binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attribute_desc = {};
    attribute_desc[0].location = 0;
    attribute_desc[0].binding = binding_desc[0].binding;
    attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[0].offset = offsetof(ImDrawVert, pos); // NOLINT
    attribute_desc[1].location = 1;
    attribute_desc[1].binding = binding_desc[0].binding;
    attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[1].offset = offsetof(ImDrawVert, uv); // NOLINT
    attribute_desc[2].location = 2;
    attribute_desc[2].binding = binding_desc[0].binding;
    attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribute_desc[2].offset = offsetof(ImDrawVert, col); // NOLINT

    VkPipelineVertexInputStateCreateInfo vertex_info = {};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = &binding_desc[0];
    vertex_info.vertexAttributeDescriptionCount = 3;
    vertex_info.pVertexAttributeDescriptions = &attribute_desc[0];

    VkPipelineInputAssemblyStateCreateInfo ia_info = {};
    ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms_info = {};
    ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::array<VkPipelineColorBlendAttachmentState, 1> color_attachment = {};
    color_attachment[0].blendEnable = VK_TRUE;
    color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].colorWriteMask = static_cast<unsigned int>(VK_COLOR_COMPONENT_R_BIT)
                                         | static_cast<unsigned int>(VK_COLOR_COMPONENT_G_BIT)
        | static_cast<unsigned int>(VK_COLOR_COMPONENT_B_BIT) | static_cast<unsigned int>(VK_COLOR_COMPONENT_A_BIT);

    VkPipelineDepthStencilStateCreateInfo depth_info = {};
    depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &color_attachment[0];

    std::array<VkDynamicState, 2> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(std::size(dynamic_states));
    dynamic_state.pDynamicStates = &dynamic_states[0];

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.flags = vkinfo->internal_->g_PipelineCreateFlags;
    info.stageCount = 2;
    info.pStages = &stage[0];
    info.pVertexInputState = &vertex_info;
    info.pInputAssemblyState = &ia_info;
    info.pViewportState = &viewport_info;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &ms_info;
    info.pDepthStencilState = &depth_info;
    info.pColorBlendState = &blend_info;
    info.pDynamicState = &dynamic_state;
    info.layout = vkinfo->internal_->g_PipelineLayout;
    info.renderPass = vkinfo->internal_->g_RenderPass;
    err = vkCreateGraphicsPipelines(vkinfo->Device, vkinfo->PipelineCache, 1, &info, vkinfo->Allocator, &vkinfo->internal_->g_Pipeline);
    check_vk_result(err);

    vkDestroyShaderModule(vkinfo->Device, vert_module, vkinfo->Allocator);
    vkDestroyShaderModule(vkinfo->Device, frag_module, vkinfo->Allocator);

    return true;
}

void    ImGui_ImplVulkan_InvalidateFontUploadObjects(ImGui_ImplVulkan_InitInfo* info)
{
    if ((info->internal_->g_UploadBuffer) != 0)
    {
        vkDestroyBuffer(info->Device, info->internal_->g_UploadBuffer, info->Allocator);
        info->internal_->g_UploadBuffer = VK_NULL_HANDLE;
    }
    if ((info->internal_->g_UploadBufferMemory) != 0)
    {
        vkFreeMemory(info->Device, info->internal_->g_UploadBufferMemory, info->Allocator);
        info->internal_->g_UploadBufferMemory = VK_NULL_HANDLE;
    }
}

void    ImGui_ImplVulkan_InvalidateDeviceObjects(ImGui_ImplVulkan_InitInfo* info)
{
    ImGui_ImplVulkan_InvalidateFontUploadObjects(info);

    for (auto& fd : info->internal_->g_FramesDataBuffers)
    {
        if ((fd.VertexBuffer) != 0) { vkDestroyBuffer(info->Device, fd.VertexBuffer, info->Allocator); fd.VertexBuffer = VK_NULL_HANDLE; }
        if ((fd.VertexBufferMemory) != 0) { vkFreeMemory(info->Device, fd.VertexBufferMemory, info->Allocator); fd.VertexBufferMemory = VK_NULL_HANDLE; }
        if ((fd.IndexBuffer) != 0) { vkDestroyBuffer(info->Device, fd.IndexBuffer, info->Allocator); fd.IndexBuffer = VK_NULL_HANDLE; }
        if ((fd.IndexBufferMemory) != 0) { vkFreeMemory(info->Device, fd.IndexBufferMemory, info->Allocator); fd.IndexBufferMemory = VK_NULL_HANDLE; }
    }

    if ((info->internal_->g_FontView) != 0)             { vkDestroyImageView(info->Device, info->internal_->g_FontView, info->Allocator); info->internal_->g_FontView = VK_NULL_HANDLE; }
    if ((info->internal_->g_FontImage) != 0)            { vkDestroyImage(info->Device, info->internal_->g_FontImage, info->Allocator); info->internal_->g_FontImage = VK_NULL_HANDLE; }
    if ((info->internal_->g_FontMemory) != 0)           { vkFreeMemory(info->Device, info->internal_->g_FontMemory, info->Allocator); info->internal_->g_FontMemory = VK_NULL_HANDLE; }
    if ((info->internal_->g_FontSampler) != 0)          { vkDestroySampler(info->Device, info->internal_->g_FontSampler, info->Allocator); info->internal_->g_FontSampler = VK_NULL_HANDLE; }
    if ((info->internal_->g_DescriptorSetLayout) != 0)  { vkDestroyDescriptorSetLayout(info->Device, info->internal_->g_DescriptorSetLayout, info->Allocator); info->internal_->g_DescriptorSetLayout = VK_NULL_HANDLE; }
    if ((info->internal_->g_PipelineLayout) != 0)       { vkDestroyPipelineLayout(info->Device, info->internal_->g_PipelineLayout, info->Allocator); info->internal_->g_PipelineLayout = VK_NULL_HANDLE; }
    if ((info->internal_->g_Pipeline) != 0)             { vkDestroyPipeline(info->Device, info->internal_->g_Pipeline, info->Allocator); info->internal_->g_Pipeline = VK_NULL_HANDLE; }
}

bool    ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass, std::size_t numBackbuffers)
{
    IM_ASSERT(info->Instance != VK_NULL_HANDLE);
    IM_ASSERT(info->PhysicalDevice != VK_NULL_HANDLE);
    IM_ASSERT(info->Device != VK_NULL_HANDLE);
    IM_ASSERT(info->Queue != VK_NULL_HANDLE);
    IM_ASSERT(info->DescriptorPool != VK_NULL_HANDLE);
    IM_ASSERT(render_pass != VK_NULL_HANDLE);

    info->internal_ = new ImGui_ImplVulkan_InternalInfo; // NOLINT
    info->internal_->g_FramesDataBuffers.resize(numBackbuffers);
    info->internal_->g_RenderPass = render_pass;
    ImGui_ImplVulkan_CreateDeviceObjects(info);

    return true;
}

void ImGui_ImplVulkan_Shutdown(ImGui_ImplVulkan_InitInfo* info)
{
    ImGui_ImplVulkan_InvalidateDeviceObjects(info);
    delete info->internal_; // NOLINT
}

void ImGui_ImplVulkan_NewFrame(ImGui_ImplVulkan_InitInfo*)
{
}


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

#include <cstdlib> // malloc

// NOLINTNEXTLINE
ImGui_ImplVulkanH_WindowData::ImGui_ImplVulkanH_WindowData()
{
    Width = Height = 0;
    Swapchain = VK_NULL_HANDLE;
    Surface = VK_NULL_HANDLE;
    memset(&SurfaceFormat, 0, sizeof(SurfaceFormat));
    PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    RenderPass = VK_NULL_HANDLE;
}
