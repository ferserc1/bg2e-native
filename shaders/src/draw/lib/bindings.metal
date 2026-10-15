#ifndef DRAW_BINDINGS_METAL
#define DRAW_BINDINGS_METAL
#include <metal_stdlib>
using namespace metal;
struct DrawMaterialData {
    float4 albedo;
    float4 fresnelTint;
    float4 sheenColor;
    float4 factors;
    float4 emissionAlpha;
    float4 uvScale[3];
    uint4 uvSets[2];
    uint4 channels;
    uint4 invert;
};
struct DrawFrameData {
    float4x4 view;
    float4x4 projection;
    float4x4 inverseViewProjection;
    float4 cameraPosition;
    float4 extentAndDelta;
    uint4 frameInfo;
};
struct DrawObjectData {
    float4x4 model;
    float4x4 normal;
    DrawMaterialData material;
    uint4 identity;
};
struct DrawLightData {
    float4 positionIntensity;
    float4 color;
    float4 directionAngle;
    float4 parameters;
    uint4 flags;
};
struct DrawLightUniforms {
    uint4 count;
    DrawLightData lights[8];
};
struct DrawEnvironmentData {
    float4 parameters;
};
float2 drawSelectUV(DrawMaterialData m, uint slot, float2 uv0, float2 uv1)
{
    uint index = 0u;
    float2 scale = float2(1.0);
    if (slot == 0u)
    {
        index = m.uvSets[0].x;
        scale = m.uvScale[0].xy;
    }
    else if (slot == 1u)
    {
        index = m.uvSets[0].y;
        scale = m.uvScale[1].xy;
    }
    else if (slot == 2u)
    {
        index = m.uvSets[0].z;
        scale = m.uvScale[1].zw;
    }
    else if (slot == 3u)
    {
        index = m.uvSets[0].w;
        scale = m.uvScale[0].zw;
    }
    else if (slot == 4u)
    {
        index = m.uvSets[1].x;
        scale = m.uvScale[2].xy;
    }
    else if (slot == 5u)
    {
        index = m.uvSets[1].y;
        scale = m.uvScale[2].zw;
    }
    return (index == 0u ? uv0 : uv1) * scale;
}
float4 drawDecodeSample(DrawMaterialData m, uint slot, float4 sampleValue)
{
    if ((m.uvSets[1].w & (1u << slot)) != 0u)
    {
        float3 c = max(sampleValue.rgb, float3(0.0));
        float3 low = c / 12.92;
        float3 high = pow((c + 0.055) / 1.055, float3(2.4));
        sampleValue.r = c.r <= 0.04045 ? low.r : high.r;
        sampleValue.g = c.g <= 0.04045 ? low.g : high.g;
        sampleValue.b = c.b <= 0.04045 ? low.b : high.b;
    }
    return sampleValue;
}
float drawScalarSample(float4 value, uint channel, uint invertValue)
{
    float result = value[channel];
    return invertValue != 0u ? 1.0 - result : result;
}

#define DRAW_VERTEX_DATA_ARGUMENTS constant DrawFrameData& drawFrame [[buffer(2)]], constant DrawObjectData& drawObject [[buffer(3)]]
#define DRAW_FRAGMENT_DATA_ARGUMENTS constant DrawFrameData& drawFrame [[buffer(6)]], constant DrawObjectData& drawObject [[buffer(7)]], constant DrawLightUniforms& drawLights [[buffer(4)]], constant DrawEnvironmentData& drawEnvironment [[buffer(5)]]
#define DRAW_MATERIAL_TEXTURE_ARGUMENTS texture2d<float> drawAlbedo [[texture(0)]], sampler drawAlbedoSampler [[sampler(0)]], texture2d<float> drawMetalness [[texture(1)]], sampler drawMetalnessSampler [[sampler(1)]], texture2d<float> drawRoughness [[texture(2)]], sampler drawRoughnessSampler [[sampler(2)]], texture2d<float> drawNormal [[texture(3)]], sampler drawNormalSampler [[sampler(3)]], texture2d<float> drawAO [[texture(4)]], sampler drawAOSampler [[sampler(4)]], texture2d<float> drawEmission [[texture(5)]], sampler drawEmissionSampler [[sampler(5)]]
#define DRAW_ENVIRONMENT_TEXTURE_ARGUMENTS texturecube<float> drawSky [[texture(6)]], sampler drawSkySampler [[sampler(6)]], texturecube<float> drawIrradiance [[texture(7)]], sampler drawIrradianceSampler [[sampler(7)]], texturecube<float> drawPrefilter [[texture(8)]], sampler drawPrefilterSampler [[sampler(8)]], texture2d<float> drawBRDF [[texture(9)]], sampler drawBRDFSampler [[sampler(9)]]
float4 drawSampleMaterial(texture2d<float> image, sampler imageSampler, DrawMaterialData m, uint slot, float2 uv0, float2 uv1)
{
    return drawDecodeSample(m, slot, image.sample(imageSampler, drawSelectUV(m, slot, uv0, uv1)));
}
float4 drawSampleMaterialLod(texture2d<float> image, sampler imageSampler, DrawMaterialData m, uint slot, float2 uv0, float2 uv1, float lod)
{
    return drawDecodeSample(m, slot, image.sample(imageSampler, drawSelectUV(m, slot, uv0, uv1), level(lod)));
}
#endif
