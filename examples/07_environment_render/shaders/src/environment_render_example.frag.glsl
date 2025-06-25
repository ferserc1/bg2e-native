#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inPosition;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform samplerCube giTex;

void main() {
    vec3 color = texture(colorTex, inTexCoord).rgb;
    vec3 lighting = texture(giTex, inPosition).rgb;
    outColor = vec4(color, 1.0f) * vec4(lighting, 1.0);
}
