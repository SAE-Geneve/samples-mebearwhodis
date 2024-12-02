#version 300 es
precision highp float;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

layout(location = 0) out vec4 outColor;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light{
    vec3 position;

    vec3 ambient; //Usually low intensity so the objects don't take too much of the light's colour
    vec3 diffuse; //Exact colour of the light, usually bright white
    vec3 specular; //Usually kept at vec3(1.0)
};

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main() {
    //Ambient light
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));

    //Diffuse light
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));

    //Specular light
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoord));

    vec3 result = (ambient + diffuse + specular);
    outColor = vec4(result, 1.0);
    //outColor = texture(ourTexture, TexCoord) * vec4(result, 1.0);
}