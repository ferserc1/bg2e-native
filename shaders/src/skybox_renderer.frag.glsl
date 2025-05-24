#version 450

layout (location = 0) in vec3 inNormal;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform samplerCube skybox;

void main()
{
    float gamma = 1.5;
    outColor = texture(skybox, inNormal);
}
