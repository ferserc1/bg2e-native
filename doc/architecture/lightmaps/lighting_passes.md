# Lighting passes attached to the atlas surface

## Shared sample executor

`LightmapBakeExecutor::recordSample()` is an internal static coordinator shared
by both execution modes. It receives the baker, command/frame slot, execution
resources, a context-owned `RayTracingScene`, lighting passes, composition pass,
IBL image/sampler and scene lights. It does not own a render loop or submit work.
Its sequence is:

1. Require remaining accumulation, target-root membership and usable RTGI IBL.
2. Filter captured lights by `affectsReflections` for GI bounce lighting.
3. Record the target's UV surface projection.
4. Run RTAO or RTGI; explicitly initialize the unused mode's image to neutral.
5. Compose a common RGB sample and record atlas accumulation/dilation.

RTAO uses `renderUv(..., samplesPerPixel, 0.1f)` and clears GI to zero.
RTGI clears AO to visibility one and uses `renderUv(..., settings)`.
The executor does not increment completion: integrated update marks the result
after recording, whereas standalone does so after submission and fence wait.

Sources: [executor declaration](../../../lib/src/bg2e/render/LightmapBakeExecutor.hpp),
[implementation](../../../lib/src/bg2e/render/LightmapBakeExecutor.cpp).

## RTAmbientOcclusion UV path

The existing deferred AO class has a distinct UV compute pipeline. `buildUv()`
requires ray-tracing support and an `RTMaterialDataBinding`; it loads
`rt_ao_uv.comp.spv`. Set 0 contains world position, world normal, unsigned mask,
TLAS and R8 storage output. Set 1 supplies ray-intersection material data for
alpha-cutout tests. It dispatches 8×8 workgroups at full atlas resolution.

`renderUv()` checks the depthless surface profile, formats/extents, sample count,
ray distance and nonempty TLAS. Invalid mask texels or near-zero normals produce
neutral visibility. The query origin uses the surface position and normal;
material cutouts are considered by `queryAOCutout`. Its bake radius is fixed
at 0.1 m by the executor, independent of the screen-space AO radius settings.
UV sampling uses the frame index for varying random samples; it does not use
screen reconstruction, camera motion vectors or the screen AO temporal filter.

Sources: [AO class](../../../lib/include/bg2e/render/deferred/RTAmbientOcclusion.hpp),
[UV pipeline](../../../lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp),
[UV shader](../../../shaders/src/glsl/rt_ao_uv.comp.glsl),
[cutout queries](../../../shaders/src/glsl/lib/rt_material_data.glsl).

## RTGlobalIllumination UV path

This existing deferred class owns a separate ray-tracing UV pipeline and Shader
Binding Table. `buildUv()` installs the UV ray-generation shader, shared GI
miss/closest-hit shaders and alpha-test any-hit shader. Pipeline recursion depth
is one: bounce iteration occurs in ray generation rather than recursive closest-hit
calls. It launches one ray-generation invocation per atlas texel.

| Descriptor set | Role |
|---|---|
| 0 | TLAS, RGBA16F output, UV position/normal, irradiance cubemap and unsigned mask. |
| 1 | RT material records, vertex/index buffers and sampled material textures. |
| 2 | Filtered GI light buffer. |

Camera fields are initialized to identity/zero for layout compatibility with
shared shaders; the UV ray generator uses the stored surface attributes instead.
The UV path passes sample count, bounce count, range and origin bias from
`LightmapSettings`, enables shadows, caps bounce-surface shadow samples at 32
and disables blue-noise input. Those choices do not come from the live renderer's
`RTGISettings` quality presets.

At ray hits, `gl_InstanceCustomIndexEXT` selects an RT material/geometry record.
Triangle indices include the submesh's `indexOffset`; barycentric hit attributes
interpolate UV1 for albedo and emission. The closest-hit shader uses geometric
vertex normals and does not apply normal maps. The initial atlas normal uses
inverse-transpose transformation; the shared hit shader currently transforms
bounce normals with `mat3(gl_ObjectToWorldEXT)`, which is a different operation
under nonuniform scale.

The shader returns environment-relative RGB. For reference channel values
above `1e-5`, output is nonnegative gathered light divided by that channel of
irradiance sampled along the initial world normal; other reference channels
produce zero. Values can exceed one. This is the current irradiance-cubemap
approximation, not a claim of a general physically exact light transport solution.
Its detailed bounce/sampling algorithms are outside this atlas-focused guide.

Sources: [GI class](../../../lib/include/bg2e/render/deferred/RTGlobalIllumination.hpp),
[pipeline/SBT and bindings](../../../lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp),
[UV ray generator](../../../shaders/src/glsl/rt_gi_uv.rgen.glsl),
[closest hit](../../../shaders/src/glsl/rt_gi.rchit.glsl),
[miss](../../../shaders/src/glsl/rt_gi.rmiss.glsl),
[alpha any-hit](../../../shaders/src/glsl/rt_alpha_test.rahit.glsl).

## Shared trace scene and bindings

The target surface lookup is independent of scene trace geometry. The TLAS
contains traceable geometry under the complete context root, allowing other
objects and other targets to occlude or contribute GI. Both context variants
own their bake TLAS instead of borrowing the renderer's transient TLAS.
`RTMaterialDataBinding::newDescriptorSet()` receives the TLAS object-instance
list, preserving the material index association. `ReflectionLightDataBinding`
provides the captured GI lights. Context capture is capped at
`BG2E_MAX_FORWARD_LIGHTS` before filtering.

## LightmapCompositionPass

This internal class owns a compute pipeline, layouts and separate color/mask
samplers. `render()` binds AO visibility, normalized GI, coverage and RGBA16F
output and dispatches 8×8 groups. Valid RTAO texels write `vec3(visibility)`;
valid RTGI texels copy normalized RGB. Uncovered texels write white. It applies
neither albedo shading, exposure nor display tone mapping. The following
accumulator restores meaningful alpha coverage from the UV mask, so composition's
white alpha-one background does not accidentally become valid history.

```glsl
vec3 lightMultiplier = pc.mode == 0u
    ? vec3(texelFetch(rtaoVisibility, pixel, 0).r)
    : texelFetch(normalizedRTGI, pixel, 0).rgb;
imageStore(lightmapOutput, pixel, vec4(lightMultiplier, 1.0));
```

Source: [composition shader](../../../shaders/src/glsl/lightmap_composition.comp.glsl).
Class sources: [header](../../../lib/src/bg2e/render/LightmapCompositionPass.hpp),
[implementation](../../../lib/src/bg2e/render/LightmapCompositionPass.cpp).
Continue with [accumulation](accumulation.md).
