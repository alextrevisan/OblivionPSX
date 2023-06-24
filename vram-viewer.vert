#version 300 es

precision highp float;
in vec2 i_position;
in vec2 i_texUV;
uniform mat4 u_projMatrix;
out vec2 fragUV;

void main() {
    fragUV = i_texUV;
    gl_Position = u_projMatrix * vec4(i_position.xy, 0.0f, 1.0f);
}
