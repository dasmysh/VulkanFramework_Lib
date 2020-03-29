# 
#  CompileSpirvShader.cmake
# 
#  Created by Bradley Austin Davis on 2016/06/23
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

function(COMPILE_SPIRV_SHADER SHADER_FILE SHADER_INCLUDE_DIRS)
    # Define the final name of the generated shader file
    find_program(GLSLANG_EXECUTABLE glslangValidator)
    get_filename_component(SHADER_DIRECTORY ${SHADER_FILE} DIRECTORY)
    get_filename_component(SHADER_TARGET ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
    set(COMPILE_OUTPUT "${SHADER_FILE}.spv")
    set(PREPROCESSOR_OUTPUT "${SHADER_DIRECTORY}/${SHADER_TARGET}.gen${SHADER_EXT}")
    list(TRANSFORM SHADER_INCLUDE_DIRS PREPEND "-i;" OUTPUT_VARIABLE  SHADER_INCLUDE_DIRS_PARAMETER)
    string (REPLACE ";" " " SHADER_INCLUDE_DIRS_STRING "${SHADER_INCLUDE_DIRS_PARAMETER}")

    add_custom_command(
        OUTPUT ${PREPROCESSOR_OUTPUT} 
        COMMAND vkfw_glsl_preprocessor ${SHADER_FILE} ${SHADER_INCLUDE_DIRS_PARAMETER} -o ${PREPROCESSOR_OUTPUT}
        DEPENDS ${SHADER_FILE} COMMAND_EXPAND_LISTS)
    add_custom_command(
        OUTPUT ${COMPILE_OUTPUT} 
        COMMAND ${GLSLANG_EXECUTABLE} -V ${PREPROCESSOR_OUTPUT} -o ${COMPILE_OUTPUT} 
        DEPENDS ${PREPROCESSOR_OUTPUT})
    set(COMPILE_SPIRV_SHADER_RETURN ${COMPILE_OUTPUT} PARENT_SCOPE)
endfunction()
