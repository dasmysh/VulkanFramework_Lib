cmake_minimum_required(VERSION 3.21)

execute_process(COMMAND vkfw_glsl_preprocessor ${SHADER_FILE} ${SHADER_INCLUDE_DIRS_PARAMETER} -o ${PREPROCESSOR_OUTPUT})
execute_process(COMMAND ${GLSLANG_EXECUTABLE} -V ${PREPROCESSOR_OUTPUT} --target-env vulkan1.2 -o ${COMPILE_OUTPUT})