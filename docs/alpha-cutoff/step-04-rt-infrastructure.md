# Step 04 — Ray tracing infrastructure: BLAS opaque bit, any-hit pipeline support, shared material bindings and alpha-test helpers

## Goal

Prepare the RT foundations used by steps 05–07:

1. Build all BLAS geometry **without** `VK_GEOMETRY_OPAQUE_BIT_KHR` (BLAS are
   shared between materials, so the flag cannot be decided at build time).
2. Extend `factory::RayTracingPipeline` with any-hit shader support.
3. Restructure `lib/rt_material_data.glsl` so the material bindings live in the
   header behind a `RT_MATERIAL_SET` macro, reusable from any shader stage.
4. Add the shared alpha-test evaluation + candidate-traversal ray query
   helpers.
5. Create the shared any-hit shader `rt_alpha_test.rahit.glsl`.

> **Ordering constraint**: change (1) must ship together with steps 05–07.
> Without the opaque bit, every hit in an inline ray query becomes a *candidate*
> that only commits if confirmed — the existing opaque query variants use
> `gl_RayFlagsOpaqueEXT`, which still commits immediately (ray flags override
> geometry flags), so existing call sites keep working even if they are not
> migrated yet.

## Files

| File | Action |
|---|---|
| `lib/src/bg2e/render/vulkan/rt/RTAccelerationStructureSize.cpp` | Modify |
| `lib/include/bg2e/render/vulkan/factory/RayTracingPipeline.hpp` | Modify |
| `lib/src/bg2e/render/vulkan/factory/RayTracingPipeline.cpp` | Modify |
| `shaders/src/glsl/lib/rt_material_data.glsl` | Modify (major) |
| `shaders/src/glsl/rt_alpha_test.rahit.glsl` | **Create** |
| `shaders/src/glsl/rt_gi.rchit.glsl` | Modify (bindings move to header) |
| `shaders/src/glsl/rt_reflections.rchit.glsl` | Modify (bindings move to header) |

## 4.1 Remove the BLAS opaque bit — `RTAccelerationStructureSize.cpp:77-81`

```cpp
_geometry = {};
_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
// No VK_GEOMETRY_OPAQUE_BIT_KHR: BLAS are shared between materials and any of
// them may require alpha testing. All triangle hits arrive as candidates and
// are resolved by any-hit shaders (traceRayEXT) or confirmed manually after
// the alpha test (rayQueryEXT).
_geometry.flags = 0;
_geometry.geometry.triangles = _trianglesData;
```

The TLAS path (`_geometry.flags = 0` at line 42) is unchanged.

## 4.2 `RayTracingPipeline` any-hit support

### Header — `RayTracingPipeline.hpp`

```cpp
void setRayGenShader(const std::string& fileName, const std::string& entryPoint = "main");
void setMissShader(const std::string& fileName, const std::string& entryPoint = "main");
void setClosestHitShader(const std::string& fileName, const std::string& entryPoint = "main");
// Optional. Must be called after setClosestHitShader(); it attaches the
// any-hit stage to the same triangles hit group.
void setAnyHitShader(const std::string& fileName, const std::string& entryPoint = "rahitMain");
```

Add a member:

```cpp
bool _hasAnyHitShader = false;
```

### Implementation — `RayTracingPipeline.cpp`

```cpp
void RayTracingPipeline::setAnyHitShader(const std::string& fileName, const std::string& entryPoint)
{
    if (!_hasClosestHitShader)
    {
        throw std::runtime_error("Invalid ray tracing pipeline factory configuration: setAnyHitShader() must be called after setClosestHitShader()");
    }
    if (_hasAnyHitShader)
    {
        throw std::runtime_error("Invalid ray tracing pipeline factory configuration: the number of any hit shaders must be at most 1");
    }
    _hasAnyHitShader = true;

    uint32_t stageIndex = addShaderStage(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, fileName, entryPoint);

    // Attach to the existing triangles hit group (the last group pushed by
    // setClosestHitShader). The SBT stores one handle per GROUP, so no SBT
    // layout changes are needed.
    _groups.back().anyHitShader = stageIndex;
}
```

Also reset `_hasAnyHitShader = false;` in `reset()` (line ~354).

The shader build system already compiles `*.rahit.glsl` with entry point
`rahitMain` (`cmake/utils.cmake:60,123-124`), and `createSBT()` copies hit
**group** handles — an any-hit stage inside the group does not change the SBT
layout.

## 4.3 Restructure `rt_material_data.glsl`

Move the binding declarations that today are duplicated in both `.rchit`
shaders into the header, parameterized by `RT_MATERIAL_SET`, and add the
alpha-test helpers. New full content of the header (license header preserved):

```glsl
#ifndef RT_MATERIAL_DATA_GLSL
#define RT_MATERIAL_DATA_GLSL

#include "uniforms.glsl"

#define MAX_RT_OBJECTS 256

// Descriptor set index for the bindings declared below. Shaders that already
// use set 1 for something else (e.g. deferred_composite_rt) must define
// RT_MATERIAL_SET before including this header.
#ifndef RT_MATERIAL_SET
#define RT_MATERIAL_SET 1
#endif

// Must match C++ RTMaterialData struct layout (56 bytes)
struct RTMaterialData {
    vec4 albedo;          // base::Color (rgba)
    vec2 albedoScale;
    uint indexOffset;     // firstIndex of the submesh in the shared index buffer
    float lightEmission;
    uint lightEmissionChannel;
    uint lightEmissionInvert;
    vec2 lightEmissionScale;
    uint lightEmissionUVSet;
    float alphaCutoff;
};

// scalar layout: packs RTMaterialData to 56 bytes, matching the C++ struct
// stride. std430 would round the array stride up to 64 bytes.
layout(scalar, set = RT_MATERIAL_SET, binding = 0) readonly buffer MaterialDataBuffer {
    RTMaterialData materials[];
};

layout(scalar, set = RT_MATERIAL_SET, binding = 1) readonly buffer VertexBuffer {
    RTVertex vertices[];
} vb[MAX_RT_OBJECTS];

layout(scalar, set = RT_MATERIAL_SET, binding = 2) readonly buffer IndexBuffer {
    uint indices[];
} ib[MAX_RT_OBJECTS];

layout(set = RT_MATERIAL_SET, binding = 3) uniform sampler2D albedoTex[MAX_RT_OBJECTS];
layout(set = RT_MATERIAL_SET, binding = 4) uniform sampler2D lightEmissionTex[MAX_RT_OBJECTS];

// Must match C++ geo::Vertex / VertexPNUUT struct layout (52 bytes)
struct RTVertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord0;
    vec2 texCoord1;
    vec3 tangent;
};

float sampleRTLightEmission(sampler2D tex, vec2 uv, RTMaterialData mat)
{
    float value = texture(tex, uv * mat.lightEmissionScale)[mat.lightEmissionChannel];
    if (mat.lightEmissionInvert != 0)
    {
        value = 1.0 - value;
    }
    return value * mat.lightEmission;
}

// Interpolated albedo UV for a triangle hit. `matIdx` is the instance custom
// index, `primitiveId` the BLAS-relative triangle id (gl_PrimitiveID or
// rayQueryGetIntersectionPrimitiveIndexEXT), `bary` the barycentric weights
// with bary.x = 1 - u - v, bary.y = u, bary.z = v.
vec2 rtHitAlbedoUV(uint matIdx, uint primitiveId, vec3 bary, out uint nmatIdx)
{
    nmatIdx = nonuniformEXT(matIdx);
    RTMaterialData mat = materials[matIdx];
    uint base = mat.indexOffset + primitiveId * 3u;
    uint idx0 = ib[nmatIdx].indices[base + 0u];
    uint idx1 = ib[nmatIdx].indices[base + 1u];
    uint idx2 = ib[nmatIdx].indices[base + 2u];
    RTVertex vert0 = vb[nmatIdx].vertices[idx0];
    RTVertex vert1 = vb[nmatIdx].vertices[idx1];
    RTVertex vert2 = vb[nmatIdx].vertices[idx2];
    vec2 uv = vert0.texCoord0 * bary.x
            + vert1.texCoord0 * bary.y
            + vert2.texCoord0 * bary.z;
    return uv * mat.albedoScale;
}

// Alpha test for a hit: true if the fragment survives (should be shaded /
// confirmed as occluder). Materials with alphaCutoff <= 0 skip the texture
// fetch entirely.
bool rtAlphaTestHit(uint matIdx, uint primitiveId, vec3 bary)
{
    RTMaterialData mat = materials[matIdx];
    if (mat.alphaCutoff <= 0.0)
    {
        return true;
    }
    uint nmatIdx;
    vec2 scaledUV = rtHitAlbedoUV(matIdx, primitiveId, bary, nmatIdx);
    float alpha = texture(albedoTex[nmatIdx], scaledUV).a * mat.albedo.a;
    return alpha >= mat.alphaCutoff;
}

#endif
```

(Keep `sampleRTLightEmission` exactly as it is today; `RTVertex` must be
declared **before** the buffer blocks that use it — reorder accordingly:
struct `RTVertex` first, then the bindings.)

### Alpha-test ray query variants

The existing opaque helpers in `lib/ray_tracing.glsl` stay untouched (the
forward RT shadows shader uses them). The cutout variants live at the end of
`rt_material_data.glsl`, after `rtAlphaTestHit`, because they need the
material bindings:

```glsl
// ---------------------------------------------------------------------------
// Alpha-tested ray queries (GL_EXT_ray_query). BLAS geometry is built without
// VK_GEOMETRY_OPAQUE_BIT_KHR, so triangle hits arrive as candidates; a
// candidate is confirmed only if it passes the material's alpha test.
// Rays are initialized WITHOUT gl_RayFlagsOpaqueEXT.
// ---------------------------------------------------------------------------

// Returns true if the candidate intersection of `rq` passes the alpha test
// (or if the material has no alpha test).
bool rtAlphaTestCandidate(rayQueryEXT rq)
{
    uint matIdx = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false);
    uint primId = rayQueryGetIntersectionPrimitiveIndexEXT(rq, false);
    vec2 b = rayQueryGetIntersectionBarycentricsEXT(rq, false);
    vec3 bary = vec3(1.0 - b.x - b.y, b.x, b.y);
    return rtAlphaTestHit(matIdx, primId, bary);
}

// Cutout version of _shadowRayTest (lib/ray_tracing.glsl):
// true = nothing occludes the ray (lit).
bool _shadowRayTestCutout(
    accelerationStructureEXT tlas,
    vec3 origin, vec3 dir, float tMax
) {
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas,
        gl_RayFlagsTerminateOnFirstHitEXT,
        0xFF, origin, 0.001, dir, tMax);
    while (rayQueryProceedEXT(rq))
    {
        if (rayQueryGetIntersectionTypeEXT(rq, false) ==
                gl_RayQueryCandidateIntersectionTriangleEXT &&
            rtAlphaTestCandidate(rq))
        {
            rayQueryConfirmIntersectionEXT(rq);
        }
    }
    return rayQueryGetIntersectionTypeEXT(rq, true) ==
           gl_RayQueryCommittedIntersectionNoneEXT;
}

float queryShadowCutout(
    accelerationStructureEXT tlas,
    vec3 worldPos, vec3 normal,
    LightData light,
    int maxSamples
);

float hardShadowCutout(
    accelerationStructureEXT tlas,
    vec3 worldPos, vec3 normal,
    LightData light
);

bool queryAOCutout(
    accelerationStructureEXT tlas,
    vec3 worldPos, vec3 normal,
    vec3 rayDir, float radius, float bias,
    out float hitDistance
);
```

`queryShadowCutout` / `hardShadowCutout` / `queryAOCutout` are byte-for-byte
copies of the opaque versions in `ray_tracing.glsl` (including the
soft-shadow cone sampling and `_shadowHash`/`_shadowRandomDirection` reuse —
those helpers are in `ray_tracing.glsl`, which cutout consumers already
include; keep the include order `uniforms.glsl` → `ray_tracing.glsl` →
`rt_material_data.glsl`, or move the hash helpers into a shared spot) with
`_shadowRayTest` replaced by `_shadowRayTestCutout` and the AO query using
the same candidate loop:

```glsl
bool queryAOCutout(
    accelerationStructureEXT tlas,
    vec3 worldPos, vec3 normal, vec3 rayDir,
    float radius, float bias,
    out float hitDistance
) {
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas,
        gl_RayFlagsTerminateOnFirstHitEXT,
        0xFF, worldPos + normal * bias, 0.001, rayDir, radius);
    while (rayQueryProceedEXT(rq))
    {
        if (rayQueryGetIntersectionTypeEXT(rq, false) ==
                gl_RayQueryCandidateIntersectionTriangleEXT &&
            rtAlphaTestCandidate(rq))
        {
            rayQueryConfirmIntersectionEXT(rq);
        }
    }
    bool hit = rayQueryGetIntersectionTypeEXT(rq, true) !=
               gl_RayQueryCommittedIntersectionNoneEXT;
    hitDistance = hit ? rayQueryGetIntersectionTEXT(rq, true) : radius;
    return hit;
}
```

Notes on the traversal: `gl_RayFlagsTerminateOnFirstHitEXT` terminates at the
first **committed** hit, so candidates that fail the alpha test (never
confirmed) simply let the query proceed — exactly the required "see through
the hole" behavior. The `rayQueryGetIntersection*EXT(rq, false)` functions
(candidate-time queries) are part of `GL_EXT_ray_query` and available on the
same devices that already expose it.

## 4.4 Shared any-hit shader — `shaders/src/glsl/rt_alpha_test.rahit.glsl` (new)

```glsl
/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *    ... (GPL header, same as other shaders)
 */

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require

#include "lib/rt_material_data.glsl"

// Shared any-hit alpha test for all ray tracing pipelines (GI, reflections).
// Invoked for every candidate triangle hit (BLAS geometry has no opaque bit).
// Fragments below the material's alphaCutoff are ignored so the ray
// continues through the cutout hole. No payload access is required.
void rahitMain()
{
    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    if (!rtAlphaTestHit(gl_InstanceCustomIndexEXT, uint(gl_PrimitiveID), bary))
    {
        ignoreIntersectionEXT;
    }
}
```

With `hitAttributeEXT vec2 attribs;` declared before `rahitMain` (same
pattern as the `.rchit` shaders; the attribute layout must match the closest
hit shaders', which also use `vec2 attribs`).

Because the any-hit shader does not read or write the ray payload, a single
`.rahit` module can serve both the GI and reflections pipelines (their hit
groups only differ in the closest-hit stage). The material bindings are on
set 1 in both pipelines — the default `RT_MATERIAL_SET`.

## 4.5 Update the `.rchit` shaders to use the header bindings

In `rt_gi.rchit.glsl` and `rt_reflections.rchit.glsl`, delete the local
binding declarations (materials/vb/ib/albedoTex/lightEmissionTex, lines
~41-57) — they now come from `rt_material_data.glsl` (already included at
line 8). Everything else in these files keeps compiling unchanged because the
names are identical. (The `queryShadow` → `queryShadowCutout` switch is done
in step 05.)

## Integration points

- `DeferredLayer::build()` must create the shared `_rtMaterialDataBinding`
  layout **once** with all consumer stages before any pipeline uses it
  (detailed in step 05):
  `VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
   VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT`.
  The layout is cached inside `RTMaterialDataBinding`
  (`RTMaterialDataBinding.cpp:46-54`), so later `createLayout()` calls reuse
  it.
- Pipeline layout stage masks in `RTReflections.cpp` / `RTGlobalIllumination.cpp`
  get `VK_SHADER_STAGE_ANY_HIT_BIT_KHR` added (step 06).

## Verification

- Build the shaders: `cmake --build build` compiles the new `.rahit` via the
  existing glslang rule.
- With only this step + step 05 partially applied, ray queries that were not
  migrated must still behave as before thanks to their `gl_RayFlagsOpaqueEXT`
  ray flag.
