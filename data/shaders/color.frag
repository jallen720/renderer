#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0, std140) uniform Color {
    vec4 data;
} color;

void main() {
    out_color = vec4(color.data.rgb, 1);
}
