#version 450

layout( location = 0 ) in vec3 inVertexPosition;
layout( location = 1 ) in vec3 inVertexNormal;
layout( location = 2 ) in vec2 inVertexTexCoords;
layout( location = 3 ) in vec3 inVertexTangent;
layout( location = 4 ) in vec3 inVertexBitangent;

uniform mat4 uMVPMatrix; 
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

uniform vec3 uLightPosition;

out vec3 fragLightPosition;
out vec3 fragPosition;
out vec3 fragNormal; 
out vec2 fragTexCoords;

void main()
{
    fragTexCoords = inVertexTexCoords;

    fragNormal = mat3(uNormalMatrix) * inVertexNormal;
    vec3 tangent = normalize(vec3(uModelMatrix * vec4(inVertexTangent, 0.0)));
    vec3 bitangent = normalize(vec3(uModelMatrix * vec4(inVertexBitangent, 0.0)));
    vec3 normal = normalize(vec3(uModelMatrix * vec4(inVertexNormal, 0.0)));

    tangent = normalize(tangent - dot(tangent, normal) * normal); 
    bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    mat3 invTBN = transpose(TBN);

    fragLightPosition = vec3(uViewMatrix * vec4(uLightPosition, 1.0)) * invTBN;
    fragPosition = vec3(uViewMatrix * uModelMatrix * vec4(inVertexPosition, 1.0)) * invTBN;
    gl_Position = uMVPMatrix * vec4(inVertexPosition, 1.0);
}
