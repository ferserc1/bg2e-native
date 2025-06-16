#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"
#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec3 inViewPos;
layout(location = 4) in vec3 inFragPos;
layout(location = 5) in flat int inLightCount;
layout(location = 6) in vec3 inTangent;
layout(location = 7) in vec3 inBitangent;

layout(location = 8) in mat3 inTBN;

layout(location = 11) in flat Light[LIGHT_COUNT] inLights;

layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;

layout(set = 2, binding = 0) uniform samplerCube giTex;


layout(push_constant) uniform PushConstant
{
    float gamma;
} pushConstant;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

void main() {
    vec3 color = texture(albedoTex, inUV0).rgb * objectData.material.albedo.rgb;
    vec3 normal = inNormal;//normalize(inTBN * sampleNormal(normalTex, inUV0));


    //mat3 TBN = mat3(inTangent, inBitangent, normal);
    mat3 TBN = inTBN;
    normal = normalize(TBN * sampleNormal(normalTex, inUV0));
    //normal = TBN * sampleNormal(normalTex, inUV0);

    vec3 ambientLight = texture(giTex, inNormal).rgb;

    vec3 viewDir = normalize(inViewPos - inFragPos);
    vec3 Lo = vec3(0.0f);
    for (int i = 0; i < 1/*inLightCount*/; ++i)
    {
        Light light = inLights[i];
        vec3 lightDir = normalize(light.position - inFragPos);
        float distance = length(light.position - inFragPos);
        float attenuation = 1.0 / (distance * distance);
        float diff = max(dot(normal, lightDir), 0.0f) * attenuation;
        Lo += diff * light.color.rgb * light.intensity * color;
    }
    
    outColor = lineal2SRGB(vec4(Lo, 1.0f) /* vec4(ambientLight, 1.0)*/, pushConstant.gamma);
}
