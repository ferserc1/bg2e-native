#version 450

layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler     uSampler;

// cameraPos: viewer position for the reflection vector.
// lod:       explicit cubemap mip level used when sampling. Always 0 for the
//            single-mip maps; cycles through the specular map mip levels.
layout(push_constant) uniform PushConstants {
    vec3  cameraPos;
    float lod;
} push;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 I = normalize(fragWorldPos - push.cameraPos);
    vec3 R = reflect(I, N);
    outColor = textureLod(samplerCube(uCubeMap, uSampler), R, push.lod);
}
