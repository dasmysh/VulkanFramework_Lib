cmake_minimum_required(VERSION 3.15)

project(VKFWLib CXX)

include(cmake/StandardProjectSettings.cmake)

add_library(vkfw_options INTERFACE)
set_project_options(vkfw_options)
target_compile_features(vkfw_options INTERFACE cxx_std_20)

add_library(vkfw_warnings INTERFACE)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(vkfw_warnings)

# enable doxygen
include(cmake/Doxygen.cmake)
enable_doxygen()

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_TESTING "Enable Test Builds" ON)

option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if (ENABLE_PCH)
  # This sets a global PCH parameter, each project will build its own PCH, which
  # is a good idea if any #define's change
  # 
  # 
  target_precompile_headers(vkfw_options INTERFACE <vector> <string> <map> <utility>)
endif()

set(IMGUI_VERSION "1.75")
set(GLM_VERSION "0.9.9.8")

set(CONAN_EXTRA_REQUIRES "")
set(CONAN_EXTRA_OPTIONS "")

set(CONAN_EXTRA_REQUIRES ${CONAN_EXTRA_REQUIRES}
                         catch2/2.11.3
                         docopt.cpp/0.6.2
                         fmt/6.1.2
                         spdlog/1.5.0
                         cereal/1.3.0
                         glm/${GLM_VERSION}
                         glfw/3.3.2@bincrafters/stable
                         imgui/${IMGUI_VERSION}
                         stb/20200203
                         assimp/5.0.1)

include(cmake/Conan.cmake)
run_conan()

find_package(Vulkan 1.1.126.0 REQUIRED)
target_compile_definitions(vkfw_options INTERFACE VULKAN_HPP_TYPESAFE_CONVERSION)
include(cmake/CompileSpirvShader.cmake)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/glm.natvis")
  message(
    STATUS
      "Downloading glm.natvis from https://github.com/g-truc/glm")
  file(DOWNLOAD "https://github.com/g-truc/glm/raw/${GLM_VERSION}/util/glm.natvis"
       "${CMAKE_BINARY_DIR}/glm.natvis")
endif()

if(NOT EXISTS "${CMAKE_BINARY_DIR}/imgui.natvis")
  message(
    STATUS
      "Downloading imgui.natvis from https://github.com/ocornut/imgui")
  file(DOWNLOAD "https://github.com/ocornut/imgui/raw/v${IMGUI_VERSION}/misc/natvis/imgui.natvis"
       "${CMAKE_BINARY_DIR}/imgui.natvis")
endif()

if(NOT EXISTS "${CMAKE_BINARY_DIR}/extern/imgui/cpp/imgui_stdlib.cpp")
  message(
    STATUS
      "Downloading imgui_stdlib.cpp from https://github.com/ocornut/imgui")
  file(DOWNLOAD "https://github.com/ocornut/imgui/raw/v${IMGUI_VERSION}/misc/cpp/imgui_stdlib.cpp"
       "${CMAKE_BINARY_DIR}/extern/imgui/cpp/imgui_stdlib.cpp")
endif()

if(NOT EXISTS "${CMAKE_BINARY_DIR}/extern/imgui/cpp/imgui_stdlib.h")
  message(
    STATUS
      "Downloading imgui.natvis from https://github.com/ocornut/imgui")
  file(DOWNLOAD "https://github.com/ocornut/imgui/raw/v${IMGUI_VERSION}/misc/cpp/imgui_stdlib.h"
       "${CMAKE_BINARY_DIR}/extern/imgui/cpp/imgui_stdlib.h")
endif()



set(VKFW_DEBUG_PIPELINE ON CACHE BOOL "Checks if application is started from RenderDoc and uses only a single queue then.")
if (${VKFW_DEBUG_PIPELINE})
    target_compile_definitions(vkfw_options INTERFACE VKFW_DEBUG_PIPELINE)
endif()

set(VKFWCORE_RESOURCE_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(VKFWCORE_RESOURCE_BASE_PATH ${VKFWCORE_RESOURCE_BASE_PATH} PARENT_SCOPE)


if(ENABLE_TESTING)
  enable_testing()
  message(
    "Building Tests. Be sure to check out test/constexpr_tests for constexpr testing"
  )
  add_subdirectory(test)
endif()

add_subdirectory(src)
