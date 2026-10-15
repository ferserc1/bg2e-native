#ifndef DRAW_RASTER_VERTEX_METAL
#define DRAW_RASTER_VERTEX_METAL
#include "../lib/bindings.metal"
struct RasterVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord0 [[attribute(2)]];
    float2 texcoord1 [[attribute(3)]];
    float3 tangent [[attribute(4)]];
};
struct RasterVaryings {
    float4 position [[position]];
    float3 worldPosition;
    float3 worldNormal;
    float3 worldTangent;
    float2 uv0;
    float2 uv1;
    float tangentSign;
};
#endif
