#version 450

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragAmbient;
layout(location = 3) out vec3 fragDiffuse;
layout(location = 4) out vec3 fragSpecular;

uniform vec3 cameraPosition;
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float materialShininess;

uniform bool hasDiffuseMap;
layout(binding = 1) uniform sampler2D diffuseMap;

uniform bool hasAmbientMap;
layout(binding = 2) uniform sampler2D ambientMap;

uniform bool hasSpecularMap;
layout(binding = 3) uniform sampler2D specularMap;

uniform bool hasShininessMap;
layout(binding = 4) uniform sampler2D shininessMap;

uniform bool hasNormalMap;
layout(binding = 5) uniform sampler2D normalMap;

in vec3 lightPosition;
in vec2 texCoords;
in vec3 normalVector;
in vec3 fragPositionInput;

void main()
{
    vec3 normal = normalize(normalVector);
    fragPosition = fragPositionInput;
    vec3 computedDiffuse;
    vec3 computedSpecular;

    vec3 viewDir = normalize(cameraPosition - fragPositionInput);
    vec3 lightDir = normalize(lightPosition - fragPositionInput);

    if (texture(diffuseMap, texCoords).a < 0.5f)
        discard;

    if (hasNormalMap)
    {
        normal = texture(normalMap, texCoords).xyz;
        normal = normalize(normal * 2.0 - 1.0);
    }

    if (dot(lightDir, normal) < 0)
        normal = -normal;

    fragNormal = normal;

    if (hasDiffuseMap)
    {
        computedDiffuse = vec3(texture(diffuseMap, texCoords)) * max(dot(normal, lightDir), 0.0);
    }
    else
    {
        computedDiffuse = diffuseColor * max(dot(normal, lightDir), 0.0);
    }
    fragDiffuse = computedDiffuse;

    vec3 computedAmbient = ambientColor;
    if (hasAmbientMap)
    {
        computedAmbient = vec3(texture(ambientMap, texCoords));
    }
    fragAmbient = computedAmbient;

    float shininessValue = materialShininess;
    if (hasShininessMap)
    {
        shininessValue = texture(shininessMap, texCoords).x;
    }

    vec3 computedSpecularColor = specularColor;
    if (hasSpecularMap)
    {
        computedSpecular = vec3(texture(specularMap, texCoords)).xxx * pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), shininessValue);
    }
    else
    {
        computedSpecular = specularColor * pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), shininessValue);
    }
    fragSpecular = computedSpecular;
}
