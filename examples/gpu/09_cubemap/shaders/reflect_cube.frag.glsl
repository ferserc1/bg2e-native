#version 450

layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler   uSampler;

layout(set = 3, binding = 0) uniform CubeRenderSettings {
    uint mode;
    uint _pad0;
    uint _pad1;
    uint _pad2;
} settings;

layout(push_constant) uniform PushConstants {
    vec3 cameraPos;
} push;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragLocalPos;

layout(location = 0) out vec4 outColor;

void main()
{
    if (settings.mode == 0) {
        vec3 N = normalize(fragNormal);
        vec3 I = normalize(fragWorldPos - push.cameraPos);
        vec3 R = reflect(I, N);
        outColor = texture(samplerCube(uCubeMap, uSampler), R);
    }
    else {
        vec3 D = normalize(fragLocalPos);
        outColor = texture(samplerCube(uCubeMap, uSampler), D);
    }
}
