#version 300 es

// The Vertex Shader isn't necessarily very useful, but is still provided here.
precision highp float;
layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;
uniform mat4 u_projMatrix;
out vec2 Frag_UV;
out vec4 Frag_Color;
void main() {
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = u_projMatrix * vec4(Position.xy, 0, 1);
}
