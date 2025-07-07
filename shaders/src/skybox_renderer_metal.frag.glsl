#version 450

layout (location = 0) in vec3 inNormal;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform samplerCube skybox;

void main()
{
    
    vec3 color = texture(skybox, inNormal).rgb;
    // HDR tone mapping
    color = color / (color + vec3(1.0));
    // to SRGB
    color = pow(color, vec3(1.0 / 1.0));
    
    outColor = vec4(color, 1.0);
}
