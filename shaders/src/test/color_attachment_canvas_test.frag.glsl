#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPosition;

layout (binding = 0) uniform sampler2D att1Tex;
layout (binding = 1) uniform sampler2D att2Tex;

void main()
{
    vec3 att1Color = texture(att1Tex, inUV).rgb;
    vec3 att2Color = texture(att2Tex, inUV).rgb;
    
    outColor = vec4(inPosition.x > 0 ? att2Color : att1Color, 1.0);
}
