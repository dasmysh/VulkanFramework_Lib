cmake_minimum_required(VERSION 3.4)

set(FWLIB_LIBNAME VKFWLib)
set(FWLIB_LIBNAME ${FWLIB_LIBNAME} PARENT_SCOPE)
project(${FWLIB_LIBNAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/CMake/modules")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompileSpirvShader.cmake")

find_package(Boost 1.58.0 REQUIRED)
find_package(Vulkan 1.0.30.0 REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target" FORCE)
add_subdirectory(extern/glfw)

set(ADD_FATAL_EXAMPLE OFF CACHE BOOL "Fatal (fatal-crashes/contract) examples " FORCE)
set(ADD_BUILD_WIN_SHARED ON CACHE BOOL "Build shared library on Windows" FORCE)
set(USE_DYNAMIC_LOGGING_LEVELS ON CACHE BOOL "Turn ON/OFF log levels. An disabled level will not push logs of that level to the sink. By default dynamic logging is disabled" FORCE)
add_subdirectory(extern/g3log)

SET(CMAKE_DEBUG_POSTFIX "")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /WX /EHsc")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function -Wno-unused-parameter -Wextra -Wpedantic -std=c++14")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

set(FWLIB_RESOURCE_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}/resources)
set(FWLIB_RESOURCE_BASE_PATH ${FWLIB_RESOURCE_BASE_PATH} PARENT_SCOPE)

file(GLOB_RECURSE LIB_RES_FILES ${FWLIB_RESOURCE_BASE_PATH}/*.*)
file(GLOB_RECURSE LIB_SHADERS 
    ${FWLIB_RESOURCE_BASE_PATH}/*.vert 
    ${FWLIB_RESOURCE_BASE_PATH}/*.frag 
    ${FWLIB_RESOURCE_BASE_PATH}/*.comp 
    ${FWLIB_RESOURCE_BASE_PATH}/*.tesc 
    ${FWLIB_RESOURCE_BASE_PATH}/*.tese
    ${FWLIB_RESOURCE_BASE_PATH}/*.geom)
set(LIB_COMPILED_SHADERS "")
foreach(SHADER ${LIB_SHADERS})
    compile_spirv_shader(${LIB_SHADERS})
    list(APPEND LIB_COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
endforeach()
if(NOT "${LIB_COMPILED_SHADERS}" STREQUAL "")
    LIST(REMOVE_ITEM LIB_RES_FILES ${LIB_COMPILED_SHADERS})
    source_group("Resources\\CompiledShaders" FILES ${LIB_COMPILED_SHADERS})
endif()

file(GLOB LIB_TOP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}/*.cpp)
file(GLOB_RECURSE LIB_TMP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}/*.inl)
LIST(REMOVE_ITEM LIB_TMP_FILES ${LIB_RES_FILES})
LIST(REMOVE_ITEM LIB_TMP_FILES ${LIB_TOP_FILES})

foreach(f ${LIB_TMP_FILES})
    file(RELATIVE_PATH SRCGR ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME} ${f})
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})
    string(REPLACE / \\ SRCGR ${SRCGR})
    source_group("${SRCGR}" FILES ${f})
endforeach()
source_group(" " FILES ${LIB_TOP_FILES})

foreach(f ${LIB_RES_FILES})
    file(RELATIVE_PATH SRCGR ${FWLIB_RESOURCE_BASE_PATH} ${f})
	set(SRCGR "Resources/${SRCGR}")
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})
    string(REPLACE / \\ SRCGR ${SRCGR})
    source_group("${SRCGR}" FILES ${f})
endforeach()

file(GLOB_RECURSE LIB_RES_OBJ_FILES ${FWLIB_RESOURCE_BASE_PATH}/*.obj)
foreach(f ${LIB_RES_OBJ_FILES})
	set_source_files_properties(${f} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()


file(GLOB LIB_CONFIGS CMakeLists.txt Readme.md .gitattributes .gitignore .gitmodules)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME} ${VULKAN_INCLUDE_DIR} extern/glfw/include/ extern/glm/ extern/g3log/src ${Boost_INCLUDE_DIRS})
add_library(${FWLIB_LIBNAME} SHARED ${LIB_COMPILED_SHADERS} ${LIB_TMP_FILES} ${LIB_TOP_FILES} ${LIB_RES_FILES} ${LIB_CONFIGS} ${EXTERN_SOURCES})
target_include_directories(${FWLIB_LIBNAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${FWLIB_LIBNAME}  ${VULKAN_INCLUDE_DIR} extern/glfw/include/ extern/glm/ extern/g3log/src ${Boost_INCLUDE_DIRS})
target_compile_definitions(${FWLIB_LIBNAME} PUBLIC VULKAN_HPP_TYPESAFE_CONVERSION)
target_compile_definitions(${FWLIB_LIBNAME} PRIVATE VKUFW_EXPORT)

target_link_libraries(${FWLIB_LIBNAME} ${VULKAN_LIBRARY} glfw ${GLFW_LIBRARIES} g3logger_shared)
set_property(TARGET ${FWLIB_LIBNAME} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:DebugOpenGLCalls>:_OGL_DEBUG_MSGS> _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS)
