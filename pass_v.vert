#version 330 core

out vec2 uv;

const vec2 verts[8] = vec2[8](
    // First triangle
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, -1.0),

    // Second triangle
    vec2(+1.0, +1.0),
    vec2(-1.0, +1.0),
    vec2(-1.0, -1.0),
    vec2(+1.0, +1.0)  
);

void main(void)
{
    uv = vec2(0.5, 0.5) + 0.5 * verts[gl_VertexID];
    gl_Position.xy = verts[gl_VertexID];
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
