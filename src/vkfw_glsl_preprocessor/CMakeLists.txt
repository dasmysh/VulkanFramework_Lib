file(RELATIVE_PATH PROJECT_REL_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

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


file(GLOB TOP_FILES
    ${PROJECT_SOURCE_DIR}/Readme.md
    ${PROJECT_SOURCE_DIR}/.clang-tidy
    ${PROJECT_SOURCE_DIR}/.clang-format
    ${CMAKE_BINARY_DIR}/glm.natvis
    ${CMAKE_BINARY_DIR}/imgui.natvis)
source_group(" " FILES ${TOP_FILES})

add_executable(vkfw_glsl_preprocessor ${SRC_FILES} ${INCLUDE_FILES} ${EXTERN_SOURCES} ${TOP_FILES} ${RES_FILES} ${COMPILED_SHADERS})
target_link_libraries(vkfw_glsl_preprocessor PRIVATE vkfw_options vkfw_warnings CONAN_PKG::docopt.cpp CONAN_PKG::fmt CONAN_PKG::spdlog)
target_include_directories(vkfw_glsl_preprocessor PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_REL_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR})
