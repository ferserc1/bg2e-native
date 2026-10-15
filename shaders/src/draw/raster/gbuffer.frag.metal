#include "vertex.metal"
struct GBufferOut {
    float4 albedo [[color(0)]];
    float4 normal [[color(1)]];
    float4 material [[color(2)]];
    float4 fresnel [[color(3)]];
    float4 sheen [[color(4)]];
    float4 baked [[color(5)]];
};
fragment GBufferOut fragMain(RasterVaryings in [[stage_in]], constant DrawObjectData& drawObject [[buffer(7)]], DRAW_MATERIAL_TEXTURE_ARGUMENTS)
{
    GBufferOut out;
    DrawMaterialData m = drawObject.material;
    float4 a = drawSampleMaterial(drawAlbedo, drawAlbedoSampler, m, 0u, in.uv0, in.uv1) * m.albedo;
    uint flags = m.uvSets[1].z;
    if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))
    {
        discard_fragment();
    }
    float3 n = normalize(in.worldNormal);
    float3 t = normalize(in.worldTangent - n * dot(n, in.worldTangent));
    float3 mapped = drawSampleMaterial(drawNormal, drawNormalSampler, m, 3u, in.uv0, in.uv1).xyz * 2.0 - 1.0;
    mapped = t * mapped.x + cross(n, t) * mapped.y * in.tangentSign + n * mapped.z;
    n = dot(mapped, mapped) > 1e-8 ? normalize(mapped) : n;
    float metallic = clamp(drawScalarSample(drawSampleMaterial(drawMetalness, drawMetalnessSampler, m, 1u, in.uv0, in.uv1), m.channels.x, m.invert.x) * m.factors.x, 0.0, 1.0);
    float roughness = clamp(drawScalarSample(drawSampleMaterial(drawRoughness, drawRoughnessSampler, m, 2u, in.uv0, in.uv1), m.channels.y, m.invert.y) * m.factors.y, 0.04, 1.0);
    float ao = clamp(drawSampleMaterial(drawAO, drawAOSampler, m, 4u, in.uv0, in.uv1)[m.channels.z], 0.0, 1.0);
    float emission = max(drawScalarSample(drawSampleMaterial(drawEmission, drawEmissionSampler, m, 5u, in.uv0, in.uv1), m.channels.w, m.invert.z) * m.emissionAlpha.x, 0.0);
    out.albedo = float4(a.rgb, (flags & 4u) == 0u ? 1.0 : a.a);
    out.normal = float4(n * 0.5 + 0.5, emission);
    out.material = float4(metallic, roughness, ao, m.factors.z);
    out.fresnel = float4(m.fresnelTint.rgb, float(flags) / 255.0);
    out.sheen = float4(m.sheenColor.rgb, m.factors.w);
    out.baked = float4(1.0);
    return out;
}
