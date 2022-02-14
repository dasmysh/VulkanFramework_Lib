#ifndef RAY_TRACING_HOST_INTERFACE
#define RAY_TRACING_HOST_INTERFACE

#include "../shader_interface.h"

BEGIN_INTERFACE(vkfw_core::gfx::rt)

CONSTANT uint INVALID_TEXTURE_INDEX = 0xffffffff;

struct InstanceDesc
{
    uint vertexSize;
    uint bufferIndex;
    uint materialType;
    uint materialIndexx;
    uint indexOffset;
    mat4 transform;
    mat4 transformInverseTranspose;
};

END_INTERFACE()

#endif // RAY_TRACING_HOST_INTERFACE
