#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_vert_pos;
layout (location = 1) in vec2 in_vert_uv;
layout (location = 0) out vec2 out_vert_uv;

layout (set = 0, binding = 0, std140) uniform Matrix {
    mat4 data;
} matrix;

void main() {
    gl_Position = matrix.data * vec4(in_vert_pos, 1);
    out_vert_uv = in_vert_uv;
}
