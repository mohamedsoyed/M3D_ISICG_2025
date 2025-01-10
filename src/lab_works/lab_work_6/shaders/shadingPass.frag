#version 450

layout( location = 0 ) out vec4 fragOutputColor;

layout( binding = 1 ) uniform sampler2D uPositionMap;
layout( binding = 2 ) uniform sampler2D uNormalMap;
layout( binding = 3 ) uniform sampler2D uAmbientMap;
layout( binding = 4 ) uniform sampler2D uDiffuseMap;
layout( binding = 5 ) uniform sampler2D uSpecularMap;

uniform vec3 uLightPosition;

void main()
{
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);

    vec3 fragPosition = texelFetch(uPositionMap, fragCoord, 0).xyz;
    vec3 fragNormal = texelFetch(uNormalMap, fragCoord, 0).xyz;
    vec3 fragAmbient = texelFetch(uAmbientMap, fragCoord, 0).xyz;
    vec3 fragDiffuse = texelFetch(uDiffuseMap, fragCoord, 0).xyz;
    vec4 fragSpecular = texelFetch(uSpecularMap, fragCoord, 0);

    vec3 lightDirection = normalize(uLightPosition - fragPosition);
    vec3 viewDirection = normalize(-fragPosition);
    fragNormal = normalize(fragNormal);

    if (dot(lightDirection, fragNormal) < 0)
        fragNormal = -fragNormal;

    float diffuseFactor = max(dot(fragNormal, lightDirection), 0.0);
    float specularFactor = pow(max(dot(fragNormal, normalize(lightDirection + viewDirection)), 0.0), fragSpecular.w);

    fragDiffuse = fragDiffuse * diffuseFactor;
    fragSpecular = fragSpecular * specularFactor;

    fragOutputColor = vec4(fragAmbient + fragDiffuse + fragSpecular.xyz, 1.0);
}
