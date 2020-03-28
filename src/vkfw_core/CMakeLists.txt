file(RELATIVE_PATH PROJECT_REL_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE RES_FILES ${VKFWCORE_RESOURCE_BASE_PATH}/*.*)
file(GLOB_RECURSE SHADER_FILES
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.vert
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.tesc
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.tese
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.geom
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.frag
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.comp
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.mesh
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.task
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.rgen
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.rint
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.rahit
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.rchit
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.rmiss
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.rcall)
set(COMPILED_SHADERS "")
foreach(SHADER ${SHADER_FILES})
    compile_spirv_shader(${SHADER})
    list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
endforeach()
if(NOT "${COMPILED_SHADERS}" STREQUAL "")
    LIST(REMOVE_ITEM RES_FILES ${COMPILED_SHADERS})
    source_group(TREE ${VKFWCORE_RESOURCE_BASE_PATH} PREFIX "resources\\compiled_shaders" FILES ${COMPILED_SHADERS})
endif()
source_group(TREE ${VKFWCORE_RESOURCE_BASE_PATH} PREFIX "resources" FILES ${RES_FILES})


file(GLOB_RECURSE SRC_FILES
    *.h
    *.hpp
    *.inl
    *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})


file(GLOB_RECURSE INCLUDE_FILES
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.h
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.hpp
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.inl)
source_group(TREE ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH} FILES ${INCLUDE_FILES})


file(GLOB EXTERN_SOURCES
    ${CMAKE_BINARY_DIR}/extern/imgui/cpp/imgui_stdlib.cpp)
source_group("extern" FILES ${EXTERN_SOURCES})


file(GLOB TOP_FILES
    ${PROJECT_SOURCE_DIR}/Readme.md
    ${PROJECT_SOURCE_DIR}/.clang-tidy
    ${PROJECT_SOURCE_DIR}/.clang-format
    ${CMAKE_BINARY_DIR}/glm.natvis
    ${CMAKE_BINARY_DIR}/imgui.natvis)
source_group(" " FILES ${TOP_FILES})


file(GLOB_RECURSE RES_OBJ_FILES ${VKFWCORE_RESOURCE_BASE_PATH}/*.obj)
foreach(f ${RES_OBJ_FILES})
    set_source_files_properties(${f} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

add_library(vk_framework_core ${SRC_FILES} ${INCLUDE_FILES} ${EXTERN_SOURCES} ${TOP_FILES} ${RES_FILES} ${COMPILED_SHADERS})
target_link_libraries(vk_framework_core PUBLIC vkfw_options vkfw_warnings Vulkan::Vulkan CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::cereal CONAN_PKG::glm CONAN_PKG::imgui)
target_link_libraries(vk_framework_core PRIVATE Shaderc::Shaderc CONAN_PKG::glfw CONAN_PKG::stb CONAN_PKG::assimp)
target_include_directories(vk_framework_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/extern/imgui/cpp)
#    extern/glfw/include
#    extern/glm
#    extern/g3log/src
#    ${CMAKE_CURRENT_BINARY_DIR}/extern/g3log/include
#    extern/stb
#    extern/cereal/include
#    extern/assimp/include
#    ${CMAKE_CURRENT_BINARY_DIR}/extern/assimp/include
#    extern/imgui
#    extern/imgui/misc/cpp)
# target_compile_definitions(vk_framework_core PRIVATE VKUFW_EXPORT)
# set_property(TARGET vk_framework_core PROPERTY CXX_STANDARD 17)
target_compile_definitions(vk_framework_core PUBLIC STBI_MSC_SECURE_CRT)


# target_link_libraries(${FWLIB_LIBNAME} ${VULKAN_LIBRARY} glfw ${GLFW_LIBRARIES} g3logger assimp)
# set_property(TARGET ${FWLIB_LIBNAME} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:DebugOpenGLCalls>:_OGL_DEBUG_MSGS>)
