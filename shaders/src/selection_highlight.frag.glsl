#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec4 inColor;

void main()
{
    outColor = vec4(vec3(1.0) - inColor.rgb, inColor.a);
}
