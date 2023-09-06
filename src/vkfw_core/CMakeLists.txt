file(RELATIVE_PATH PROJECT_REL_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
set(RES_FILES "")
set(SHADER_FILES "")
set(COMPILED_SHADERS "")
resource_file_list(${VKFWCORE_RESOURCE_BASE_PATH} "${VKFWCORE_RESOURCE_BASE_PATH}/shader" RES_FILES SHADER_FILES COMPILED_SHADERS)


file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    *.h
    *.hpp
    *.inl
    *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})


file(GLOB_RECURSE INCLUDE_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.h
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.hpp
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}/*.inl)
source_group(TREE ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH} FILES ${INCLUDE_FILES})


file(GLOB TOP_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/Readme.md
    ${PROJECT_SOURCE_DIR}/.clang-tidy
    ${PROJECT_SOURCE_DIR}/.clang-format
    ${CMAKE_BINARY_DIR}/glm.natvis
    ${CMAKE_BINARY_DIR}/imgui.natvis
    ${CMAKE_BINARY_DIR}/imgui.gdb
    ${CMAKE_BINARY_DIR}/imgui.natstepfilter)
source_group(" " FILES ${TOP_FILES})


file(GLOB_RECURSE RES_OBJ_FILES ${VKFWCORE_RESOURCE_BASE_PATH}/*.obj)
foreach(f ${RES_OBJ_FILES})
    set_source_files_properties(${f} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

add_custom_target(compile_shaders_core DEPENDS ${COMPILED_SHADERS})

add_library(vk_framework_core ${SRC_FILES} ${INCLUDE_FILES} ${EXTERN_SOURCES} ${TOP_FILES} ${RES_FILES})
target_link_libraries(vk_framework_core PUBLIC vkfw_options vkfw_warnings Vulkan::Vulkan fmt::fmt spdlog::spdlog cereal::cereal glm::glm imgui::imgui)
target_link_libraries(vk_framework_core PRIVATE glfw assimp::assimp)
add_dependencies(vk_framework_core compile_shaders_core)
target_include_directories(vk_framework_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}
    ${Stb_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/resources/shader
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(vk_framework_core PUBLIC STBI_MSC_SECURE_CRT CEREAL_THREAD_SAFE=1)
