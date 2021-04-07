#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_vert_pos;
layout (location = 0) out vec3 out_vert_pos;

void main() {
    gl_Position = vec4(in_vert_pos, 1);
    out_vert_pos = in_vert_pos;
}
