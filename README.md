# VulkanFramework_Lib
Port of my OpenGL Framework to Vulkan.

## Build (Windows)
- Use [vcpkg](https://vcpkg.io/en/)
- Clone repository with submodules.
- From the projects build folder run:

  ```.extern\vkfw_core\vcpkg\vcpkg install catch2 docopt fmt spdlog cereal glm glfw3 imgui stb assimp --triplet x64-windows```

- Start CMake with:

  ```cmake -B ./build -S . -DCMAKE_TOOLCHAIN_FILE=.\extern\vkfw_core\vcpkg\scripts\buildsystems\vcpkg.cmake```

