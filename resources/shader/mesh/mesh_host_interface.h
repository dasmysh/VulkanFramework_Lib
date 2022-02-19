#ifndef MESH_HOST_INTERFACE
#define MESH_HOST_INTERFACE

#include "../shader_interface.h"

BEGIN_INTERFACE(mesh)

BEGIN_UNIFORM_BLOCK(set = 0, binding = 0, WorldUniformBufferObject)
mat4 model;
mat4 normalMatrix;
END_UNIFORM_NAMED_BLOCK(world_ubo)

END_INTERFACE()

#endif // MESH_HOST_INTERFACE
