#ifndef RAY_TRACING_HOST_INTERFACE
#define RAY_TRACING_HOST_INTERFACE

#include "../shader_interface.h"

BEGIN_INTERFACE(rt)

struct InstanceDesc
{
    uint vertexSize;
    uint bufferIndex;
    uint materialIndex;
    uint indexOffset;
    mat4 transform;
    mat4 transformInverseTranspose;
};

END_INTERFACE()

#endif // RAY_TRACING_HOST_INTERFACE
