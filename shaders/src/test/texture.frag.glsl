#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;

layout(set = 1, binding = 1) uniform sampler2D colorTex;

void main() {
    vec3 color = texture(colorTex, inTexCoord).rgb * 1.8f;
    //outColor = vec4(color * (inNormal * 0.5 + 0.5), 1.0f);
    //outColor = vec4((inNormal * 0.5 + 0.5), 1.0f);
    outColor = vec4((inTangent * 0.5 + 0.5), 1.0f);
}
