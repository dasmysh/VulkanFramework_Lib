function(RESOURCE_FILE_LIST RESOURCE_BASE_PATH RES_FILES_OUT SHADER_FILES_OUT COMPILED_SHADERS_OUT)
    file(GLOB_RECURSE RES_FILES CONFIGURE_DEPENDS ${RESOURCE_BASE_PATH}/*.*)
    file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
        ${RESOURCE_BASE_PATH}/*.vert
        ${RESOURCE_BASE_PATH}/*.tesc
        ${RESOURCE_BASE_PATH}/*.tese
        ${RESOURCE_BASE_PATH}/*.geom
        ${RESOURCE_BASE_PATH}/*.frag
        ${RESOURCE_BASE_PATH}/*.comp
        ${RESOURCE_BASE_PATH}/*.mesh
        ${RESOURCE_BASE_PATH}/*.task
        ${RESOURCE_BASE_PATH}/*.rgen
        ${RESOURCE_BASE_PATH}/*.rint
        ${RESOURCE_BASE_PATH}/*.rahit
        ${RESOURCE_BASE_PATH}/*.rchit
        ${RESOURCE_BASE_PATH}/*.rmiss
        ${RESOURCE_BASE_PATH}/*.rcall)
    file(GLOB_RECURSE GEN_SHADER_FILES CONFIGURE_DEPENDS
        ${RESOURCE_BASE_PATH}/*.gen.vert
        ${RESOURCE_BASE_PATH}/*.gen.tesc
        ${RESOURCE_BASE_PATH}/*.gen.tese
        ${RESOURCE_BASE_PATH}/*.gen.geom
        ${RESOURCE_BASE_PATH}/*.gen.frag
        ${RESOURCE_BASE_PATH}/*.gen.comp
        ${RESOURCE_BASE_PATH}/*.gen.mesh
        ${RESOURCE_BASE_PATH}/*.gen.task
        ${RESOURCE_BASE_PATH}/*.gen.rgen
        ${RESOURCE_BASE_PATH}/*.gen.rint
        ${RESOURCE_BASE_PATH}/*.gen.rahit
        ${RESOURCE_BASE_PATH}/*.gen.rchit
        ${RESOURCE_BASE_PATH}/*.gen.rmiss
        ${RESOURCE_BASE_PATH}/*.gen.rcall)
    file(GLOB_RECURSE SHADER_DEP_FILES CONFIGURE_DEPENDS
        ${RESOURCE_BASE_PATH}/*.vert.dep
        ${RESOURCE_BASE_PATH}/*.tesc.dep
        ${RESOURCE_BASE_PATH}/*.tese.dep
        ${RESOURCE_BASE_PATH}/*.geom.dep
        ${RESOURCE_BASE_PATH}/*.frag.dep
        ${RESOURCE_BASE_PATH}/*.comp.dep
        ${RESOURCE_BASE_PATH}/*.mesh.dep
        ${RESOURCE_BASE_PATH}/*.task.dep
        ${RESOURCE_BASE_PATH}/*.rgen.dep
        ${RESOURCE_BASE_PATH}/*.rint.dep
        ${RESOURCE_BASE_PATH}/*.rahit.dep
        ${RESOURCE_BASE_PATH}/*.rchit.dep
        ${RESOURCE_BASE_PATH}/*.rmiss.dep
        ${RESOURCE_BASE_PATH}/*.rcall.dep)

    if(NOT "${GEN_SHADER_FILES}" STREQUAL "")
        list(REMOVE_ITEM SHADER_FILES ${GEN_SHADER_FILES})
        list(REMOVE_ITEM RES_FILES ${GEN_SHADER_FILES})
        source_group(TREE ${RESOURCE_BASE_PATH} PREFIX "resources\\generated_shaders" FILES ${GEN_SHADER_FILES})
    endif()
    if(NOT "${SHADER_DEP_FILES}" STREQUAL "")
        list(REMOVE_ITEM SHADER_FILES ${SHADER_DEP_FILES})
        list(REMOVE_ITEM RES_FILES ${SHADER_DEP_FILES})
    endif()

    set(COMPILED_SHADERS "")
    foreach(SHADER ${SHADER_FILES})
        compile_spirv_shader("${SHADER}" "${RESOURCE_BASE_PATH}/shader")
        list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
    endforeach()
    if(NOT "${COMPILED_SHADERS}" STREQUAL "")
        list(REMOVE_ITEM RES_FILES ${COMPILED_SHADERS})
        source_group(TREE ${RESOURCE_BASE_PATH} PREFIX "resources\\compiled_shaders" FILES ${COMPILED_SHADERS})
    endif()
    source_group(TREE ${RESOURCE_BASE_PATH} PREFIX "resources" FILES ${RES_FILES})
    set(${RES_FILES_OUT} ${RES_FILES})
    set(${SHADER_FILES_OUT} ${SHADER_FILES})
    set(${COMPILED_SHADERS_OUT} ${COMPILED_SHADERS})
    return(PROPAGATE ${RES_FILES_OUT} ${SHADER_FILES_OUT} ${COMPILED_SHADERS_OUT})
endfunction()
