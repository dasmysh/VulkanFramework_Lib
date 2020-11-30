# VulkanFramework_Lib
Port of my OpenGL Framework to Vulkan.

## Build (Windows)
- Use [conan](https://conan.io/)
- From the projects build folder run:

  ```conan install --build=missing --install-folder=./vkfw_core ../extern/vkfw_core/```

  ```conan install --build=missing --install-folder=./vkfw_core -s build_type=Debug ../extern/vkfw_core/```

  This does not generate debug symbols for Visual Studio and some warnings will be generated. To avoid use the `--build` parameter without `=missing`.

