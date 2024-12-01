#version 300 es
precision highp float;

//layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aColor;
//layout (location = 2) in vec2 aTexCoord;
layout(location = 0) in vec3 light_position; // Vertex position
layout(location = 1) in vec2 light_texCoord; // Texture coordinate

uniform mat4 light_model;
uniform mat4 light_view;
uniform mat4 light_projection;

out vec3 light_fragColor;
out vec2 light_TexCoord;

//vec3 positions[4] = vec3[](
//vec3(0.5, 0.5, 0.0),
//vec3(0.5, -0.5, 0.0),
//vec3(-0.5, -0.5, 0.0),
//vec3(-0.5, 0.5, 0.0)
//);

vec3 light_colors[6] = vec3[](
vec3(1.0, 0.0, 0.0),
vec3(0.0, 1.0, 0.0),
vec3(0.0, 0.0, 1.0),
vec3(0.0, 0.0, 1.0),
vec3(0.0, 1.0, 0.0),
vec3(1.0, 0.0, 0.0)
);

//vec2 texCoords[4] = vec2[](
//vec2(1.0, 1.0),
//vec2(1.0, 0.0),
//vec2(0.0, 0.0),
//vec2(0.0, 1.0)
//);

void main() {
    gl_Position = light_projection * light_view * light_model * vec4(light_position, 1.0);
    light_fragColor = light_colors[gl_VertexID % 6];
    light_TexCoord = light_texCoord;
}