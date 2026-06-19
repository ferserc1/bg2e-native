#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 worldNormal;
    float3 albedo;
};

struct LightUBO {
    float4 lightPosition;   // xyz = world position
    float4 lightColor;      // rgb = color, a = intensity
    float4 ambient;         // rgb = ambient term
};

// Buffer indices match ShaderBinding.metal:
//   set 2, binding 0 (light)                 -> buffer(1)
//   set 2, binding 1 (acceleration structure) -> buffer(2)
fragment float4 fragMain(VertexOut in [[stage_in]],
                         constant LightUBO& light [[buffer(1)]],
                         instance_acceleration_structure topLevelAS [[buffer(2)]])
{
    float3 N = normalize(in.worldNormal);

    float3 toLight  = light.lightPosition.xyz - in.worldPos;
    float  distance = length(toLight);
    float3 lightDir = toLight / distance;
    float  ndotl    = max(dot(N, lightDir), 0.0);

    // Cast a visibility ray from the shaded point toward the light.
    const float bias = 0.02;
    float shadow = 1.0;

    ray r;
    r.origin       = in.worldPos + N * bias;
    r.direction    = lightDir;
    r.min_distance = bias;
    r.max_distance = distance - bias;

    intersector<instancing, triangle_data> shadowIntersector;
    shadowIntersector.assume_geometry_type(geometry_type::triangle);
    shadowIntersector.force_opacity(forced_opacity::opaque);
    shadowIntersector.accept_any_intersection(true);

    intersector<instancing, triangle_data>::result_type result =
        shadowIntersector.intersect(r, topLevelAS, 0xFF);

    if (result.type != intersection_type::none)
    {
        shadow = 0.15;
    }

    float3 diffuse = in.albedo * light.lightColor.rgb * light.lightColor.a * ndotl * shadow;
    float3 ambient = in.albedo * light.ambient.rgb;

    return float4(ambient + diffuse, 1.0);
}
