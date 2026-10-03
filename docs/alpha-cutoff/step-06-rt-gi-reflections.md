# Step 06 — Alpha-tested primary rays in RTGI and RT reflections

## Goal

Reflection and GI primary rays currently trace with `gl_RayFlagsOpaqueEXT`,
so a ray hitting a cutout hole shades the discarded texel as if it were solid.
Attach the shared any-hit shader (step 04) to both pipelines and drop the
opaque ray flag so failing fragments are ignored and the ray continues.

Depends on: step 04 (any-hit pipeline support, `rt_alpha_test.rahit.glsl`).

## Files

| File | Action |
|---|---|
| `lib/src/bg2e/render/deferred/RTReflections.cpp` | Modify |
| `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp` | Modify |
| `shaders/src/glsl/rt_reflections.rgen.glsl` | Modify |
| `shaders/src/glsl/rt_gi.rgen.glsl` | Modify |

## 6.1 `RTReflections.cpp` — `createPipeline()` (line ~121)

### Descriptor set 0 stage mask — add ANY_HIT:

```cpp
_dsLayout = dsLayoutFactory.build(
    _engine->device().handle(),
    VK_SHADER_STAGE_RAYGEN_BIT_KHR |
    VK_SHADER_STAGE_MISS_BIT_KHR |
    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
    VK_SHADER_STAGE_ANY_HIT_BIT_KHR
);
```

### Pipeline creation — attach the any-hit shader (line ~156):

```cpp
vulkan::factory::RayTracingPipeline rtPipelineFactory(_engine);
rtPipelineFactory.setRayGenShader("rt_reflections.rgen.spv");
rtPipelineFactory.setMissShader("rt_reflections.rmiss.spv");
rtPipelineFactory.setClosestHitShader("rt_reflections.rchit.spv");
rtPipelineFactory.setAnyHitShader("rt_alpha_test.rahit.spv");   // entry point: rahitMain

_pipeline = rtPipelineFactory.build(_pipelineLayout, 1, "RTReflections::Pipeline");
auto sbt = rtPipelineFactory.createSBT("RTReflections::SBT");
```

`createSBT()` needs no changes: the SBT stores one handle per shader **group**
and the any-hit stage lives inside the existing triangles hit group.

The material binding layout (set 1) already comes from the shared
`_materialDataBinding->createLayout()` call — after step 05 it is cached with
`ANY_HIT` included in its stage mask. (If step 05's explicit layout creation
were skipped, pass the stages here:
`_materialDataBinding->createLayout(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR)`.)

## 6.2 `RTGlobalIllumination.cpp` — `createPipeline()` (line ~109)

Identical edits:

```cpp
_dsLayout = dsLayoutFactory.build(
    _engine->device().handle(),
    VK_SHADER_STAGE_RAYGEN_BIT_KHR |
    VK_SHADER_STAGE_MISS_BIT_KHR |
    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
    VK_SHADER_STAGE_ANY_HIT_BIT_KHR
);
```

```cpp
rtPipelineFactory.setRayGenShader("rt_gi.rgen.spv");
rtPipelineFactory.setMissShader("rt_gi.rmiss.spv");
rtPipelineFactory.setClosestHitShader("rt_gi.rchit.spv");
rtPipelineFactory.setAnyHitShader("rt_alpha_test.rahit.spv");
```

## 6.3 `rt_reflections.rgen.glsl` — drop the opaque ray flag (line ~140)

```glsl
traceRayEXT(
    tlas,
    gl_RayFlagsNoneEXT,        // was: gl_RayFlagsOpaqueEXT — any-hit must run
    0xff,
    0,
    0,
    0,
    worldPos + normal * pc.rayBias,
    0.001,
    rayDir,
    pc.maxDistance,
    0
);
```

## 6.4 `rt_gi.rgen.glsl` — drop the opaque ray flag (line ~92)

```glsl
traceRayEXT(
    tlas,
    gl_RayFlagsNoneEXT,        // was: gl_RayFlagsOpaqueEXT
    0xff,
    0, 0, 0,
    origin,
    0.001,
    dir,
    pc.maxDistance,
    0
);
```

## Behavior and performance notes

- With the BLAS opaque bit removed (step 04) and `gl_RayFlagsNoneEXT`, every
  triangle hit invokes the any-hit shader. Materials with `alphaCutoff <= 0`
  return immediately from `rtAlphaTestHit()` without sampling the texture, so
  the cost on fully opaque scenes is one any-hit invocation + one SSBO read
  per candidate.
- A candidate that passes the test (or has no alpha test) continues to the
  closest-hit shader exactly as before; `gl_HitTEXT`, `gl_PrimitiveID`,
  barycentrics and the payload flow are unchanged.
- A candidate that fails the test is discarded via `ignoreIntersectionEXT`
  and traversal continues — for GI this means the ray may escape to the
  irradiance-map miss path; for reflections the sample counts as a miss
  (`didHit = 0`), lowering the reflection certainty alpha, which is the
  desired behavior (the envmap shows through the hole).
- The miss shaders and the payload layouts are untouched.
- The any-hit shader does not declare the ray payload — legal in GLSL and
  what allows one `.rahit` module to serve both pipelines.

## Verification

- Mirror-like surface (`roughness < 0.01`) reflecting an alpha-textured
  object: the reflection must show the holes (background/envmap through
  them), not solid quads.
- `RTGlobalIllumination` debug view with an alpha-textured occluder between
  two surfaces: color bleeding and occlusion must respect the cutout.
- `RTReflectionMask` debug view (certainty alpha): pixels whose reflection
  ray passes through a hole show reduced certainty instead of a solid hit.
