#version 300 es
precision highp float;

in vec3 fragColor;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

layout(location = 0) out vec4 outColor;

uniform sampler2D ourTexture;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    //Ambient light
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    //Diffuse light
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    //Specular light
    float specularStrength = 0.5;
    float shininessValue = 32.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininessValue);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * fragColor;
    outColor = texture(ourTexture, TexCoord) * vec4(result, 1.0);
}