#ifndef MATERIAL_HOST_INTERFACE
#define MATERIAL_HOST_INTERFACE

#include "../shader_interface.h"

BEGIN_INTERFACE(materials)

BEGIN_CONSTANTS(MaterialIdentifierCore)
    NoMaterialType = 0,
    PhongMaterialType = 1,
    PhongBumpMaterialType = 2,
    ApplicationMaterialsStart = 3
END_CONSTANTS()

struct NoMaterial
{
    int dummy;
};

struct PhongMaterial
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float alpha;
    float specularExponent;
    uint diffuseTextureIndex;
};

struct PhongBumpMaterial
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float alpha;
    float specularExponent;
    float bumpMultiplier;
    uint diffuseTextureIndex;
    uint bumpTextureIndex;
};

END_INTERFACE()

#endif // MATERIAL_HOST_INTERFACE
