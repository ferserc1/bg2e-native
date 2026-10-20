# M03-05 implementation handoff

[Step](step_05_rtgi.md) · [API](../../../../doc/api/draw/RTGlobalIllumination.md) · [Manual validation](../../../../tests/draw/rtgi/README.md)

## Public contracts

RTGlobalIllumination, RTGIConfiguration/Settings/Context and rtgiBindingLayout now exist under draw::pipeline. The pass is independent of RendererDeferred and reuses SurfaceInputs plus TracingSceneResources/TracingSnapshot. Configuration selects draw.rtgi.v1, shaderRoot/prefix and optional shared noise. It accepts retired-slot preparation from a queue or an externally retained matching-capacity snapshot. Invocation resources remain separate for successive transparent composites. Cleanup drains before releasing Device-backed resources.

RendererConfiguration appends globalIllumination and IndirectLightingMode. RendererDeferred exposes globalIllumination(), usesGlobalIllumination(), indirectLightingMode() and setIndirectLightingMode(). Modes are IBL, AmbientOcclusion and GlobalIllumination. The original M03 implementation default was AO with GI disabled; the project lead later changed the global renderer default to GI selected and GI/reflections enabled (see the default-policy amendment in the M03 summary and progress record). Explicit modes and effect settings remain authoritative. Selection does not overwrite either component's enabled/settings values. In GI mode AO uses a null prepared snapshot and produces neutral one; in other modes GI is cleared to invalid zero without scene traversal. Disabled or unsupported GI preserves requested mode/settings and falls back to ordinary IBL/baked arithmetic. Native pipeline unavailability on Vulkan does not disable compute-query AO.

## Native execution and ABI

Vulkan uses sequential traceRayEXT calls in raygen, miss and one closest-hit/any-hit triangle group, maxRecursionDepth=1. Closest-hit returns dense instance/primitive/barycentrics/distance; miss returns no hit; any-hit uses shared traceAccept. Shading and shadow rays execute sequentially in raygen, so secondary shadow calls do not require recursion depth two. There is no Vulkan query dependency for GI. Metal uses independently deployed compute kernels with nearest committed triangle queries and shared native traceAccept. Both exclude blended geometry and retain canonical identity separately from dense TLAS IDs.

All twelve surface/noise/capacity combinations are provided: camera/world, hash/blue, 32/128/256. Each Vulkan combination has rgen/rmiss/rchit/rahit; each Metal combination has comp. Existing CMake discovery handles these paths; no build-system edits were made. ShaderLib resolves the native stages.

Tracing set 1 retains M03-04's four storage buffers, TLAS and independent image/sampler tables. Vulkan exposes it to RayGeneration and AnyHit; Metal exposes it to Compute. Surface set 0 keeps the AO map: UBO binding0/native buffer1; normals/position/coverage bindings1..3/native textures0..2; nearest sampler binding4/native9; output binding5/native texture4; optional noise bindings6/7/native texture3/sampler10. Environment irradiance is binding8/native texture5 and binding9/native sampler11. Scalar cube bindings use ordinary sampled resources, not material argument-buffer arrays.

GIUniforms is 784 bytes: inverseVP at0; parameters(maxDistance,bias,shadowSamples,useShadows) at64; counts(samples,bounces,instances,renderedSample) at80; noise metadata at96; source/output extents at112; shared 656-byte LightUniforms at128. The logical capacity reservation is five sampled images, three samplers and fifteen other resources. AO and GI can choose different profiles; share a snapshot only when capacity and Device match.

## Transport and composition

Cosine hemisphere sampling cancels the Lambert PDF/PI in path throughput. Output RGB is incoming diffuse radiance/pi before receiver albedo, with alpha validity. Hit outgoing radiance includes albedo-colored scalar emission and Lambert direct lighting, then throughput multiplies the decoded hit albedo. Hit normals use transformed geometric triangles oriented against the incident ray, without normal maps. Albedo and emission honor each slot's UV set/scale, sRGB decode, channel/inversion and LOD0 through the retained MaterialData tables. Cutout rejection resumes traversal toward the nearest accepted rear hit. Transforms/submesh bases come from the shared validated snapshot.

Point, directional and spot lights use the shared draw light ABI, inverse-square/spot attenuation and flags.w (affectsReflections) participation, matching the production reduced GI light set. useShadows/castShadows and shadowSamples control cutout-aware visibility; sourceSize perturbs shadow directions/positions. Shadow sequences use deterministic hash sampling; transport uses optional shared BlueNoise or hash. Environment irradiance already includes PI and is divided by PI when used as escaped/terminal radiance. Ambient closes each path once at escape or the terminal bounce, never at every intermediate hit. Nonfinite output becomes invalid zero; finite HDR is bounded to half-float range.

Output is sampled/storage/color-attachment RGBA16_SFLOAT. Quality dimensions and camera/explicit receivers follow M03-04. Neutral is zero with alpha zero, including uncovered/empty/disabled/unavailable cases; a valid black estimate has alpha one. Deferred lighting replaces only the receiver's diffuse IBL with GI when valid, preserving specular IBL, sheen, material AO, baked multiplier, direct PBR and emission. New contracts append .gi before .v1 and the globalIllumination scaled HDR input; Metal uses texture/sampler15. Existing base/shadow/AO variants remain available. Explicit old lighting contracts opt GI out; GI-only contracts opt AO out. Actual baked map presence/replacement policy remains M04.

## Validation and downstream limitations

The manual fixture exercises emissive HDR/albedo, cutout UV continuation, bounded distance versus excluded geometry, environment escape, direct-light participation, blue noise, multiple bounces, subclass dispatch, camera/explicit agreement and disabled/empty validity. It retains the shared dense-ID/submesh/geometry cache checks. No agent build, shader compilation or runtime execution occurred. Lead validation must cover both native shader trees and hardware diagnostics, nested/mirrored transforms, shadow occlusion, all quality/profile variants and existing deferred/manipulation examples.

No history, denoiser, UI/reflected settings integration, new example target or bake orchestration is introduced. The original delivery had a separate eight-cutout-pair SoftShadowPass; the later M07 correction expands it under the [shared capacity policy](../milestone_07_bg2e_composer/tracing-capacity-16384.md). Its remaining alpha fallback is per texture slot, not whole-scene; do not disable direct shadows when testing larger GI material tables below effective capacity. M03-06 reuses native hit/table/surface contracts; M03-07 owns HDR history validity/invalidation and rendered-view keys; M03-08 filtering; M03-09 visible controls and example integration; M03-10 cross-effect review. M04/M08 reuse explicit surfaces, matching-capacity snapshots and caller-owned completion discipline.
## Later M07 capacity amendment

The [16384 correction](../milestone_07_bg2e_composer/tracing-capacity-16384.md)
supersedes this original delivery's small native profiles and Metal buffer8 ABI.
GI now uses draw.rtgi.v2, exact device-clamped capacity, Vulkan specialization ID0
and Metal image/sampler argument-record buffers8/9. Packed material ownership and
transport semantics remain; texture-table exhaustion warns/degrades rather than
exiting. Original numerical/native coverage claims above remain historical.
