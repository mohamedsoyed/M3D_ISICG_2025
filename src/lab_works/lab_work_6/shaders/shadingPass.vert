#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 viUV;

void main()
{
    viUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
