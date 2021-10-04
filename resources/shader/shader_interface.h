#ifndef SHADER_HOST_INTERFACE
#define SHADER_HOST_INTERFACE

#ifdef __cplusplus
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// GLSL Type
namespace shader_types {
    using vec2 = glm::vec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;
    using uint = std::uint32_t;
}

#define CONSTANT constexpr

#define BEGIN_INTERFACE(name) \
    namespace name {          \
        using namespace shader_types;
#define END_INTERFACE() }

#define BEGIN_CONSTANTS(name)       \
    enum class name : std::uint32_t \
    {
#define END_CONSTANTS() };

#define BEGIN_UNIFORM_BLOCK(set_id, binding_id, block_name) struct block_name {
#define END_UNIFORM_BLOCK(var_name) };

#define BEGIN_INPUT_BLOCK(struct_name) struct struct_name {
#define INPUT_ELEMENT(location_id)
#define END_INPUT_BLOCK() };

#else

#define CONSTANT const

#define BEGIN_INTERFACE(name)
#define END_INTERFACE()

#define BEGIN_CONSTANTS(name) CONSTANT uint
#define END_CONSTANTS() ;

#define BEGIN_UNIFORM_BLOCK(set, binding, block_name) layout(set, binding) uniform block_name {
#define END_UNIFORM_BLOCK(var_name) } var_name;

#define BEGIN_INPUT_BLOCK(struct_name)
#define INPUT_ELEMENT(location_id) layout(location = location_id) in
#define END_INPUT_BLOCK()

#endif

#endif // SHADER_HOST_INTERFACE
