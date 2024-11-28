#version 300 es
precision highp float;

in vec3 fragColor;
in vec2 TexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ourTexture;

void main() {
    outColor = texture(ourTexture, TexCoord) * vec4(fragColor, 1.0);
}