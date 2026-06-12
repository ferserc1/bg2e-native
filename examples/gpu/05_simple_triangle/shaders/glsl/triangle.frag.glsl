#version 450

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler   uSampler;

layout(push_constant) uniform Push {
    vec4 color;
} pc;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 tex = texture(sampler2D(uTex, uSampler), fragUV);
    outColor = vec4(tex.rgb * fragColor * pc.color.rgb, tex.a * pc.color.a);
}
