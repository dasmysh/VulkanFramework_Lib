#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

const vec2 pos_data[3] = vec2[]
(
    vec2(-1.0,  1.0),
    vec2( 3.0,  1.0),
    vec2(-1.0, -3.0)
);

const vec2 tex_data[3] = vec2[]
(
    vec2(0.0, 1.0),
    vec2(2.0, 1.0),
    vec2(0.0, -1.0)
);

void main()
{
    fragTexCoord = tex_data[ gl_VertexIndex ];
    gl_Position = vec4( pos_data[ gl_VertexIndex ], 0.0, 1.0 );
}
