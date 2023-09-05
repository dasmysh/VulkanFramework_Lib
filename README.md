# VulkanFramework_Lib
Port of my OpenGL Framework to Vulkan.

## Build (Windows)
- Use [vcpkg](https://vcpkg.io/en/)
- Clone repository with submodules recursive:

  ```git clone --recurse-submodules -j8 https://github.com/dasmysh/VulkanFramework_Lib ```
  ```cd VulkanFramework_Lib```

- Start CMake from this folder with:

  ```cmake -S . --preset=default```

- CMake GUI can be used after the first configure step is done with the correct toolchain file.
