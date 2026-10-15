#include <metal_stdlib>
using namespace metal;
struct FinalPush {
    float4 color;
    float4 outputExtent;
    uint4 display;
};
float4 processColor(float4 position, constant FinalPush& settings, texture2d<float> source, sampler sourceSampler)
{
    float2 uv = position.xy / float2(settings.outputExtent.xy);
    float2 size = float2(source.get_width(), source.get_height());
    float2 coordinate = uv * size - 0.5;
    float2 base = floor(coordinate);
    float2 weight = coordinate - base;
    float2 uv00 = (base + 0.5) / size;
    float2 uv10 = (base + float2(1.5, 0.5)) / size;
    float2 uv01 = (base + float2(0.5, 1.5)) / size;
    float2 uv11 = (base + 1.5) / size;
    float4 result = mix(mix(source.sample(sourceSampler, uv00, level(0.0)), source.sample(sourceSampler, uv10, level(0.0)), weight.x), mix(source.sample(sourceSampler, uv01, level(0.0)), source.sample(sourceSampler, uv11, level(0.0)), weight.x), weight.y);
    if (settings.display.x == 0u)
    {
        result.rgb = 1.0 - exp(-max(result.rgb, float3(0.0)) * exp2(settings.color.x));
        result.rgb = pow(result.rgb, float3(1.0 / settings.color.w));
        result.rgb = clamp((result.rgb + settings.color.z - 0.5) * settings.color.y + 0.5, float3(0.0), float3(1.0));
    }
    else if (settings.display.x == 7u)
    {
        result.rgb = float3(result.r);
    }
    // Convert the display-encoded value back to linear for native sRGB output.
    // The attachment encoder then produces the same display value as UNORM.
    if (settings.display.y != 0u)
    {
        float3 c = max(result.rgb, float3(0.0));
        float3 lo = c / 12.92;
        float3 hi = pow((c + 0.055) / 1.055, float3(2.4));
        result.r = c.r <= 0.04045 ? lo.r : hi.r;
        result.g = c.g <= 0.04045 ? lo.g : hi.g;
        result.b = c.b <= 0.04045 ? lo.b : hi.b;
    }
    return float4(result.rgb, 1.0);
}
float4 processDepth(float4 position, constant FinalPush& settings, depth2d<float> source, sampler sourceSampler)
{
    float2 uv = position.xy / float2(settings.outputExtent.xy);
    float2 size = float2(source.get_width(), source.get_height());
    float2 coordinate = uv * size - 0.5;
    float2 base = floor(coordinate);
    float2 weight = coordinate - base;
    float2 uv00 = (base + 0.5) / size;
    float2 uv10 = (base + float2(1.5, 0.5)) / size;
    float2 uv01 = (base + float2(0.5, 1.5)) / size;
    float2 uv11 = (base + 1.5) / size;
    float4 result = mix(mix(float4(source.sample(sourceSampler, uv00)), float4(source.sample(sourceSampler, uv10)), weight.x), mix(float4(source.sample(sourceSampler, uv01)), float4(source.sample(sourceSampler, uv11)), weight.x), weight.y);
    if (settings.display.x == 0u)
    {
        result.rgb = 1.0 - exp(-max(result.rgb, float3(0.0)) * exp2(settings.color.x));
        result.rgb = pow(result.rgb, float3(1.0 / settings.color.w));
        result.rgb = clamp((result.rgb + settings.color.z - 0.5) * settings.color.y + 0.5, float3(0.0), float3(1.0));
    }
    else if (settings.display.x == 7u)
    {
        result.rgb = float3(result.r);
    }
    // Convert the display-encoded value back to linear for native sRGB output.
    // The attachment encoder then produces the same display value as UNORM.
    if (settings.display.y != 0u)
    {
        float3 c = max(result.rgb, float3(0.0));
        float3 lo = c / 12.92;
        float3 hi = pow((c + 0.055) / 1.055, float3(2.4));
        result.r = c.r <= 0.04045 ? lo.r : hi.r;
        result.g = c.g <= 0.04045 ? lo.g : hi.g;
        result.b = c.b <= 0.04045 ? lo.b : hi.b;
    }
    return float4(result.rgb, 1.0);
}
fragment float4 fragMain(float4 position [[position]], constant FinalPush& settings [[buffer(0)]],
    texture2d<float> image0 [[texture(0)]],
    sampler sample0 [[sampler(0)]],
    texture2d<float> image1 [[texture(1)]],
    sampler sample1 [[sampler(1)]],
    texture2d<float> image2 [[texture(2)]],
    sampler sample2 [[sampler(2)]],
    texture2d<float> image3 [[texture(3)]],
    sampler sample3 [[sampler(3)]],
    texture2d<float> image4 [[texture(4)]],
    sampler sample4 [[sampler(4)]],
    texture2d<float> image5 [[texture(5)]],
    sampler sample5 [[sampler(5)]],
    texture2d<float> image6 [[texture(6)]],
    sampler sample6 [[sampler(6)]],
    depth2d<float> image7 [[texture(7)]],
    sampler sample7 [[sampler(7)]])
{
    if (settings.display.x == 0u || settings.display.x == 8u)
    {
        return processColor(position, settings, image0, sample0);
    }
    if (settings.display.x == 1u)
    {
        return processColor(position, settings, image1, sample1);
    }
    if (settings.display.x == 2u)
    {
        return processColor(position, settings, image2, sample2);
    }
    if (settings.display.x == 3u)
    {
        return processColor(position, settings, image3, sample3);
    }
    if (settings.display.x == 4u)
    {
        return processColor(position, settings, image4, sample4);
    }
    if (settings.display.x == 5u)
    {
        return processColor(position, settings, image5, sample5);
    }
    if (settings.display.x == 6u)
    {
        return processColor(position, settings, image6, sample6);
    }
    return processDepth(position, settings, image7, sample7);
}
