#version 300 es
precision highp float;

out vec4 light_FragColor;

uniform vec3 lightColour;

void main() {
    light_FragColor = vec4(lightColour, 1.0);
}