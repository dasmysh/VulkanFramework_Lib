file(RELATIVE_PATH PROJECT_REL_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE RES_FILES CONFIGURE_DEPENDS ${VKFWCORE_RESOURCE_BASE_PATH}/*.*)
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
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
file(GLOB_RECURSE GEN_SHADER_FILES CONFIGURE_DEPENDS
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.vert
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.tesc
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.tese
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.geom
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.frag
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.comp
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.mesh
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.task
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.rgen
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.rint
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.rahit
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.rchit
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.rmiss
    ${VKFWCORE_RESOURCE_BASE_PATH}/*.gen.rcall)
if(NOT "${GEN_SHADER_FILES}" STREQUAL "")
    list(REMOVE_ITEM SHADER_FILES ${GEN_SHADER_FILES})
    list(REMOVE_ITEM RES_FILES ${GEN_SHADER_FILES})
    source_group(TREE ${VKFWCORE_RESOURCE_BASE_PATH} PREFIX "resources\\generated_shaders" FILES ${GEN_SHADER_FILES})
endif()
set(COMPILED_SHADERS "")
foreach(SHADER ${SHADER_FILES})
    compile_spirv_shader("${SHADER}" "${VKFWCORE_RESOURCE_BASE_PATH}/shader")
    list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
endforeach()
if(NOT "${COMPILED_SHADERS}" STREQUAL "")
    list(REMOVE_ITEM RES_FILES ${COMPILED_SHADERS})
    source_group(TREE ${VKFWCORE_RESOURCE_BASE_PATH} PREFIX "resources\\compiled_shaders" FILES ${COMPILED_SHADERS})
endif()
source_group(TREE ${VKFWCORE_RESOURCE_BASE_PATH} PREFIX "resources" FILES ${RES_FILES})


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


file(GLOB EXTERN_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_BINARY_DIR}/extern/imgui/cpp/imgui_stdlib.cpp)
source_group("extern" FILES ${EXTERN_SOURCES})


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
target_link_libraries(vk_framework_core PUBLIC vkfw_options vkfw_warnings Vulkan::Vulkan CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::cereal CONAN_PKG::glm CONAN_PKG::imgui)
target_link_libraries(vk_framework_core PRIVATE CONAN_PKG::glfw CONAN_PKG::stb CONAN_PKG::assimp)
add_dependencies(vk_framework_core compile_shaders_core)
target_include_directories(vk_framework_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}
    ${PROJECT_SOURCE_DIR}/resources/shader
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/extern/imgui/cpp)
target_compile_definitions(vk_framework_core PUBLIC STBI_MSC_SECURE_CRT CEREAL_THREAD_SAFE=1)
