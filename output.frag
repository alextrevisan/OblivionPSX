#version 300 es

// The Pixel Shader is most likely what the user will want to change.
precision highp float;
uniform sampler2D Texture;
in vec2 Frag_UV;
in vec4 Frag_Color;
layout (location = 0) out vec4 Out_Color;
void main() {
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
    Out_Color.a = 1.0;
}
