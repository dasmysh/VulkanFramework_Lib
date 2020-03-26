// dear imgui: Platform Binding for GLFW
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan..)
// (Info: GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [x] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. FIXME: 3 cursors types are missing from GLFW.
//  [X] Platform: Keyboard arrays indexed using GLFW_KEY_* codes, e.g. ImGui::IsKeyPressed(GLFW_KEY_SPACE).

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// About GLSL version:
// The 'glsl_version' initialization parameter defaults to "#version 150" if NULL.
// Only override if your GL version doesn't handle this GLSL version. Keep NULL if unsure!

struct GLFWwindow;
struct ImGui_GLFWWindow;

IMGUI_IMPL_API bool     ImGui_ImplGlfw_InitForOpenGL(ImGui_GLFWWindow** imgui_window, GLFWwindow* window);
IMGUI_IMPL_API bool     ImGui_ImplGlfw_InitForVulkan(ImGui_GLFWWindow** imgui_window, GLFWwindow* window);
IMGUI_IMPL_API void     ImGui_ImplGlfw_Shutdown(ImGui_GLFWWindow* imgui_window);
IMGUI_IMPL_API void     ImGui_ImplGlfw_NewFrame(ImGui_GLFWWindow* imgui_window);

// GLFW callbacks (installed by default if you enable 'install_callbacks' during initialization)
// Provided here if you want to chain callbacks.
// You can also handle inputs yourself and use those as a reference.
IMGUI_IMPL_API void     ImGui_ImplGlfw_MouseButtonCallback(ImGui_GLFWWindow* imgui_window, int button, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplGlfw_ScrollCallback(ImGui_GLFWWindow* imgui_window, double xoffset, double yoffset);
IMGUI_IMPL_API void     ImGui_ImplGlfw_KeyCallback(ImGui_GLFWWindow* imgui_window, int key, int scancode, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplGlfw_CharCallback(ImGui_GLFWWindow* imgui_window, unsigned int c);
