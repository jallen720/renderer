#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_vert_pos;
layout (location = 0) out vec4 color;

void main() {
    color = vec4(in_vert_pos, 1);
}
