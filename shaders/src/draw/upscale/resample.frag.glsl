#version 450
layout(set = 0, binding = 0) uniform texture2D sourceImage;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(push_constant) uniform Push
{
    vec4 outputSize;
} pc;
layout(location = 0) out vec4 outColor;
void main()
{
    vec2 size = vec2(textureSize(sampler2D(sourceImage, sourceSampler), 0));
    vec2 p = gl_FragCoord.xy / pc.outputSize.xy * size - 0.5;
    vec2 b = floor(p);
    vec2 w = p - b;
    vec4 a = textureLod(sampler2D(sourceImage, sourceSampler), (b + 0.5) / size, 0.0);
    vec4 c = textureLod(sampler2D(sourceImage, sourceSampler), (b + vec2(1.5, 0.5)) / size, 0.0);
    vec4 d = textureLod(sampler2D(sourceImage, sourceSampler), (b + vec2(0.5, 1.5)) / size, 0.0);
    vec4 e = textureLod(sampler2D(sourceImage, sourceSampler), (b + 1.5) / size, 0.0);
    outColor = mix(mix(a, c, w.x), mix(d, e, w.x), w.y);
}
