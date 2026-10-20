# M03-06 implementation handoff

[Step](step_06_rt-reflections.md) · [API](../../../../doc/api/draw/RTReflections.md) · [Manual fixture](../../../../tests/draw/rt_reflections/README.md)

## Public and renderer contracts

Public RTReflections, RTReflectionSettings/Configuration/Context/Quality and rtReflectionBindingLayout live in draw::pipeline. The pass consumes M03-04 SurfaceInputs and TracingSnapshot, logical LightUniforms and retained EnvironmentResources. Receiver material is sampled RGBA8 UNORM or RGBA16 float with G=roughness at the surface extent. Explicit cameraPosition supplies view direction for either receiver profile. Caller owns surface/material/output image lifetime. Prepared snapshots must match Device and capacity; there is no second geometry/material ABI. Default components own their tracing service/cache; externally coordinated effects can share a matching snapshot through prepareSnapshot.

RendererConfiguration appends reflections. RendererDeferred exposes reflections()/usesReflections(), allocates capability-supported targets, prepares each retired slot and records every composite. Reflections are independent of IBL/AO/GI selection; enabled settings remain intact when unsupported. Explicit lighting contracts lacking .reflections opt out. Capability selection for shadow/AO/GI inputs now tests their individual contract tags so all combinations remain valid. Existing shader variants are retained.

Low/Medium/High/Ultra reflection quality scales display output by 1/3, 1/2, 2/3 and 1, nearest integer rounding, clamped to one. Receiver extent is independent. Runtime target reconfiguration detects reflection quality changes. Output-only resize and internal-only scaling rebuild appropriate graph resources, but reflection dimensions depend only on display and quality. Settings support samples, positive bias/distance, roughness threshold/spread, shadow samples/toggle and shared noise; see API ranges. No public Vulkan descriptors are exposed.

## Native execution and binding ABI

Twelve surface/noise/capacity profiles supply sixty native wrapper files: Vulkan rgen/rmiss/rchit/rahit and Metal comp. Vulkan performs all primary/shadow rays sequentially from raygen at recursion depth one. It reuses rtgi_payload/hit/any helpers as the existing dense hit record and cutout stage implementation, without invoking GI transport. Metal computes nearest committed triangles with the same candidate predicate. Explicit native sources share logical semantics; no shader translation layer or build-system change is introduced.

Set 1 is the existing packed scene ABI: vertices/indices/instances/materials, TLAS, image/sampler arrays. Set 0 extends GI's map with receiver material binding10/native texture6. Other bindings remain UBO0/buffer1; normal1/texture0; depth-position2/texture1; coverage3/texture2; nearest sampler4/native9; output5/texture4; optional noise6/texture3 and7/sampler10; irradiance8/texture5 and9/sampler11. Capacity reserves six images, three samplers and sixteen resources. Vulkan scene visibility is RayGeneration+AnyHit; Metal is Compute.

ReflectionUniforms is 816 bytes: inverseVP0, parameters(distance,bias,shadowSamples,useShadows)64, counts(samples,reserved,instances,renderedSample)80, noise96, source/output extents112, cameraPosition/maxRoughness128, roughnessSpread144, LightUniforms160. All arrays/resources are retained through completion. The requested renderedSample controls noise independently of presentation serial and acquired slot.

## Shading and signal semantics

Mirrors use one deterministic ray; glossy surfaces use the production GGX-like spread around the reflection direction, increase samples with roughness and avoid below-surface directions. Cutout rejection resumes traversal. Hit albedo, scalar emission and normal mapping honor slot UV0/UV1/scale, color decoding and channel/inversion at LOD0. Blended geometry stays excluded. Normals interpolate source vertices and use inverse transpose; tangents use model transformation, Gram-Schmidt orthogonalization, raster's fallback basis and determinant sign. Source meshes carry XYZ tangents only; object handedness is +1 and mirrored transforms supply the sign. A normal texture selecting UV1 still uses this same source tangent convention, matching raster rather than inventing a second tangent basis. Degenerate interpolated normals fall back to oriented geometric normals.

Hit lighting is albedo-colored emission, diffuse environment irradiance/PI and participating Lambert direct lights with point/spot/directional attenuation. flags.w uses affectsReflections; non-shadow-casting lights contribute normally. Shadow queries honor cutouts, bias/distance, sample count and source size. Reflection environment misses are not shaded by the trace pass; the lighting consumer retains prefiltered IBL for those samples. This respects the current draw environment contract where irradiance includes PI and specular prefilter is linear radiance.

RGBA16 RGB averages finite accepted geometry-hit outgoing radiance, before receiver BRDF, unpremultiplied; alpha is hits/actualSamples. A black hit is valid; a miss/uncovered/roughness-rejected/empty/disabled signal is zero-confidence black. Invalid hit lighting contributes to neither mean nor confidence; radiance is bounded to half range. The lighting consumer replaces prefiltered specular proportionally to confidence and applies F0*A+B once, material AO and baked multiplier as before. Existing diffuse GI/IBL, direct PBR, sheen, emission and unlit behavior remain intact. Actual baked presence policy is later M04 work.

New lighting contracts append .reflections before .v1 and a scaled reflections HDR input. Native texture16 carries reflected radiance; it reuses nearest sampler0 to remain within Metal's sixteen fragment sampler slots even with shadows+AO+GI. FullscreenPass appends an optional independent native sampler map; the original default mapping is unchanged. Vulkan still initializes every separate logical sampler binding. Metal scalar sampler aliases here bind the same sampler object and resolve to the same native entry.

## Validation and downstream handoff

No agent configure/build, shader compilation or fixture/runtime execution occurred. Static inspection covers native maps/ABI, source lifetimes, paired wrappers, source includes/delimiters/preprocessor, logical lighting bindings, Metal sampler limits and documentation links; git diff --check is required. Lead validates Vulkan and Metal native diagnostics, missing capability fallback and all existing deferred/manipulation regressions.

The manual fixture checks shared geometry/dense/submesh/UV data, deterministic red mirror emission, rear blue emission through cutouts, distance misses, glossy partial confidence without RGB premultiplication, roughness rejection, shared noise, camera/explicit agreement and a 2x2 display-relative target over a 1x1 receiver. Mirrored normal-map lighting, shadow occluders, full resize/FSR behavior and partial-confidence composition require the additional lead scenarios.

M03-07 must keep reflection histories separate from scalar AO and diffuse GI and preserve fractional confidence, valid black and rejected masks; do not treat alpha as binary GI validity or premultiplied radiance. M03-08 filtering must preserve those semantics and receiver edges. M03-09 adds reflected settings/UI and validates existing examples. M05 freezes scene state while varying rendered sample keys; reflection resolution remains output-relative. Legacy SoftShadowPass still has a separate eight-cutout-pair limit; disable it when checking larger effect tables. No native GPU primitives, production render migration, history, denoiser, new example or bake context is added in this step.
## Later M07 capacity amendment

The [16384 correction](../milestone_07_bg2e_composer/tracing-capacity-16384.md)
supersedes this original delivery's small native profiles and Metal buffer8 ABI.
Reflections use draw.rt_reflections.v2, exact device-clamped capacity, Vulkan
specialization ID0 and Metal image/sampler record buffers8/9. Material ownership
and transport semantics remain; texture-table exhaustion warns/degrades rather
than exiting. Original coverage claims remain historical.
