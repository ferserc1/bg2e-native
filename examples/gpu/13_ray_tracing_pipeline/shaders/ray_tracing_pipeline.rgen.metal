#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

struct CameraData {
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 misc; // x=frameCount, y=width, z=height
};

// Mirror of the C++ InstanceData struct.
//   shapeType (emission.a): 0 = plane, 1 = sphere, 2 = cube
struct InstanceData {
    float4 albedo;         // rgb, a = roughness
    float4 emission;       // rgb, a = shapeType
    float4 normalOrCenter; // plane: world normal; sphere/cube: world center; w = sphere radius
    float4 extents;        // cube: world-space half-extents
};

constant int   MAX_BOUNCES = 8;
constant float PI = 3.14159265359;

static uint pcg(thread uint& state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}
static float rnd(thread uint& state) { return float(pcg(state)) / 4294967295.0; }

// Cosine-weighted hemisphere sample around the normal n.
static float3 cosineHemisphere(float3 n, thread uint& state) {
    float r1 = rnd(state);
    float r2 = rnd(state);
    float phi = 2.0 * PI * r1;
    float r = sqrt(r2);
    float3 up = abs(n.x) > 0.9 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, n));
    float3 bitangent = cross(n, tangent);
    return normalize(tangent * (cos(phi) * r) +
                     bitangent * (sin(phi) * r) +
                     n * sqrt(1.0 - r2));
}

kernel void rgenMain(
    texture2d<float, access::read_write> outputImage [[texture(0)]],
    texture2d<float, access::read_write> accumImage  [[texture(1)]],
    device const CameraData& cam [[buffer(1)]],
    instance_acceleration_structure tlas [[buffer(2)]],
    device const InstanceData* instances [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]])
{
    if (gid.x >= uint(cam.misc.y) || gid.y >= uint(cam.misc.z)) return;

    uint seed = (gid.x * 1973u + gid.y * 9277u + uint(cam.misc.x) * 26699u) | 1u;

    // Sub-pixel jitter for anti-aliasing; each frame draws a fresh sample.
    float2 jitter = float2(rnd(seed), rnd(seed));
    float2 pixelCenter = float2(gid) + jitter;
    float2 inUV = pixelCenter / float2(cam.misc.y, cam.misc.z);
    float2 d = inUV * 2.0 - 1.0;

    float4 origin = cam.viewInverse * float4(0, 0, 0, 1);
    float4 target = cam.projInverse * float4(d.x, d.y, 1, 1);
    float4 direction = cam.viewInverse * float4(normalize(target.xyz), 0);

    float3 rayO = origin.xyz;
    float3 rayD = direction.xyz;

    intersector<instancing, triangle_data> rayIntersector;
    rayIntersector.assume_geometry_type(geometry_type::triangle);
    rayIntersector.force_opacity(forced_opacity::opaque);
    // Closest-hit search (not any-hit): we need the nearest surface to shade.
    rayIntersector.accept_any_intersection(false);

    float3 radiance = float3(0.0);
    float3 throughput = float3(1.0);

    for (int b = 0; b < MAX_BOUNCES; ++b)
    {
        ray r;
        r.origin = rayO;
        r.direction = rayD;
        r.min_distance = 0.001;
        r.max_distance = 10000.0;

        intersector<instancing, triangle_data>::result_type hit =
            rayIntersector.intersect(r, tlas, 0xFF);

        if (hit.type == intersection_type::none)
        {
            // Ray escaped the box: closed Cornell box -> black background.
            break;
        }

        InstanceData inst = instances[hit.instance_id];
        int shapeType = int(inst.emission.a);
        float3 worldPos = rayO + rayD * hit.distance;

        // Analytic surface normal.
        float3 n;
        if (shapeType == 0)        // plane
        {
            n = normalize(inst.normalOrCenter.xyz);
        }
        else if (shapeType == 1)   // sphere
        {
            n = normalize(worldPos - inst.normalOrCenter.xyz);
        }
        else                       // cube: dominant axis of the local position
        {
            float3 local = (worldPos - inst.normalOrCenter.xyz) / max(inst.extents.xyz, float3(1e-4));
            float3 a = abs(local);
            if (a.x > a.y && a.x > a.z)      n = float3(sign(local.x), 0.0, 0.0);
            else if (a.y > a.z)              n = float3(0.0, sign(local.y), 0.0);
            else                             n = float3(0.0, 0.0, sign(local.z));
        }
        if (dot(n, rayD) > 0.0)
        {
            n = -n;
        }

        radiance += throughput * inst.emission.rgb;

        // Glossy reflection model: roughness interpolates between a perfect
        // mirror (0) and a diffuse cosine-weighted bounce (1).
        float3 refl = reflect(rayD, n);
        float3 diff = cosineHemisphere(n, seed);
        float roughness = inst.albedo.a;
        float3 newDir = normalize(mix(refl, diff, roughness));
        if (dot(newDir, n) < 0.0)
        {
            newDir = diff;
        }

        throughput *= inst.albedo.rgb;
        rayO = worldPos + n * 0.001;
        rayD = newDir;

        float maxThroughput = max(throughput.r, max(throughput.g, throughput.b));
        if (maxThroughput < 0.001)
        {
            break;
        }
    }

    // Monte Carlo accumulation: running mean of the per-frame estimates.
    float frameCount = cam.misc.x;
    float3 prev = accumImage.read(gid).rgb;
    float3 accum = (frameCount <= 1.0) ? radiance
                                       : mix(prev, radiance, 1.0 / frameCount);
    accumImage.write(float4(accum, 1.0), gid);

    // Reinhard tone mapping + gamma correction for the 8-bit output image.
    // Store as .bgr because the image is byte-copied into the BGRA8 swapchain.
    float3 mapped = accum / (accum + float3(1.0));
    mapped = pow(mapped, float3(1.0 / 2.2));
    outputImage.write(float4(mapped.bgr, 1.0), gid);
}
