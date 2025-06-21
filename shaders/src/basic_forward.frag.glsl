#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"
#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"
#include "lib/pbr.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec3 inViewPos;
layout(location = 4) in vec3 inFragPos;
layout(location = 5) in flat int inLightCount;
layout(location = 6) in mat3 inTBN;
layout(location = 9) in flat Light[LIGHT_COUNT] inLights;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D metallicTex;
layout(set = 1, binding = 4) uniform sampler2D roughnessTex;
layout(set = 1, binding = 5) uniform sampler2D aoTex;

layout(push_constant) uniform PushConstant
{
    float gamma;
} pushConstant;

layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilteredEnvMap;
layout(set = 2, binding = 2) uniform sampler2D brdfLUT;
layout(set = 2, binding = 3) uniform EnvironmentData {
    float maxReflectionLOD;
} environmentData;

void main()
{
    PBRMaterialData mat = objectData.material;
    vec3 albedo = sampleAlbedo(albedoTex, inUV0, inUV1, mat, pushConstant.gamma).rgb;
    float metallic = sampleMetallic(metallicTex, inUV0, inUV1, mat);
    float roughness = sampleRoughness(roughnessTex, inUV0, inUV1, mat);
    float ambientOcclussion = sampleAmbientOcclussion(aoTex, inUV0, inUV1, mat);
    vec3 normal = sampleNormal(normalTex, inUV0, inUV1, mat, inTBN);

    vec3 viewDir = normalize(inViewPos - inFragPos);
    vec3 F0 = calcF0(albedo, mat);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < inLightCount; ++i)
    {
        Lo += calcRadiance(inLights[i], viewDir, inFragPos, metallic, roughness, F0, normal, albedo);
    }
  
    vec3 ambient = calcAmbientLight(
        viewDir, normal, F0, 
        albedo, metallic, roughness,
        irradianceMap, prefilteredEnvMap,
        environmentData.maxReflectionLOD,
        brdfLUT, ambientOcclussion
    );

    vec3 color = ambient + Lo;

    outColor = lineal2SRGB(vec4(color, 1.0), pushConstant.gamma);
}
