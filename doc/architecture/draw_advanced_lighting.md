# Advanced lighting in the draw renderer

`bg2e::draw` is the production high-level renderer over Vulkan and Metal GPU
backends. The [deferred model](../deferred_render_model.md) introduces its frame
flow; native bindings and effect contracts below remain explicit.

The actual scene graph order is G-buffer/cutout-aware shadows, raw AO/GI/reflection
signals, independent temporal histories, spatial denoise, lighting/composition,
HDR reconstruction (resampling or optional FSR), final color, optional SMAA,
overlays, then UI. No filtered signal replaces raw temporal history. Each effect
invocation/view/layer has retained resources associated with a completed slot.

| Signal | Raw output and meaning | Composition |
|---|---|---|
| AO | R16 scalar visibility; neutral one | Multiplies ambient without discarding material AO |
| GI | RGBA16 incoming diffuse radiance/pi before receiver albedo; binary alpha validity | Replaces diffuse IBL when valid, preserving specular IBL |
| Reflection | RGBA16 mean accepted geometry-hit radiance; fractional hit confidence, unpremultiplied | Blends with prefiltered IBL using confidence, then receiver BRDF once |

Valid black GI/hits differ from invalid signals. Reflection misses preserve specular
IBL; GI unavailable/disabled restores diffuse IBL. GI and AO modes retain each
other's settings and are selected through renderer.indirectLighting. Reflection
participation is independent. Both blended classes remain excluded as secondary
geometry/occluders. Hit lighting is an approximation, not arbitrary recursive PBR
or transparent secondary transport. Native material sampling initially uses LOD0.

## Capability and requested-state separation

| Facility | Requirement / effective report |
|---|---|
| AO / query shadows | Compute queries and acceleration structures; effect-specific formats |
| GI / reflections on Vulkan | Native ray-tracing pipeline with raygen/miss/closest/any-hit, recursion depth one |
| GI / reflections on Metal | Independently authored compute intersection-query kernels |
| Advanced textured tracing | Nonuniform indexed resources, complete device-clamped tables up to 16384, v3 native effect shaders and required output formats |
| Temporal / denoise / SMAA | Their own formats and native kernels; RT not required |
| FSR | Supported Vulkan backend, SDK/runtime and camera requirements; primitive RT support is unrelated |

available/status, graph selection and requested enabled/mode are distinct. UI
requests remain serializable without device support. Unavailable interactive
features retain usable raster/IBL/resampling. Malformed scene data remains an error;
exhausted tracing image/sampler tables warn and use neutral samples for excess
slots without dropping geometry. Direct SoftShadowPass uses the same 16384/device
ceiling policy with reserves (2,1,9), Vulkan specialization and Metal argument
records5/6. Its alphaTextureLimit reports the effective capacity. Exceeding that
bound uses neutral alpha only for excess slots, never whole-scene shadow removal.
Both direct shadows and advanced material arrays retain every instance and the
authored scalar values. Texture-count reductions are deferred until API migration
is complete; object-local material/resource ownership remains unchanged.

## Histories, filtering and mutation

sceneFrameNumber counts submitted scene work, independently of presentation/UI.
Temporal keys separate signal/view/layer; reprojection rejection restarts pixel
counts. maxFrames bounds the arithmetic then rolling/frozen display history.
frameCount is nominal convergence, not an exact whole-image N-sample result.
Transform-only object motion preserves global histories and FSR state. It permits
rolling updates during motion and for exactly 3*maxFrames subsequent consecutive
scene records per active signal/view/layer, then restores the authored freeze
preference. There are no previous-object motion vectors. AO/GI/reflection trails,
stale lighting on stationary receivers and FSR trails are accepted Phase 0 limits;
the finite window does not guarantee exact convergence.
Spatial denoise uses geometry guides/confidence and keeps HDR unpremultiplied.
The display denoiser has no chart-ID guide. Exact UV bake averaging and chart-safe
padding now belong to [bake](../api/bake/LightmapOutput.md); exact camera-image
averaging and windowless execution remain separate pending products.

Queued edits happen at a synchronized boundary and request scene work. Settings,
light, material, membership and lifecycle changes retain global SDK/engine resets;
transform-only changes use the rolling window. Explicit general editor callbacks
may still invalidate or reattach scenes and must be reviewed separately. Direct camera teleport,
in-place image upload or custom shader changes retaining identity require explicit
Renderer::invalidateHistories. Catch partial material failures and notify as well;
MaterialModifier itself owns no renderer. Pause animation for convergence comparisons.

## Reuse and native resources

[Tracing snapshots](../api/draw/TracingSceneResources.md) and
[explicit/camera surfaces](../api/draw/SurfaceInputs.md) are public. Standalone
clients own input images, output, completed slots/serials and ordered submissions;
Device must outlive every returned owner. Failed/discarded history recording needs
invalidation before a successor. Cleanup drains at stopped lifecycle boundaries.

Native sources live in shaders/src/draw/{rtao,rtgi,rt_reflections,
temporal_accumulation,denoise,smaa} and shared draw/lib kernels. GLSL and Metal
are independently authored. Deploy compiled native libraries for the selected
backend/profile. SMAA also retains the official iryku_smaa lookup bytes/licenses.
See [distribution](../apps/draw_deferred_renderer.md).

Actual backend/scenario coverage is not inferred from source inspection. The lead
accepted M03 implementation by continuation; specific GI/reflection measurements
were not supplied. The [manual validation matrix](../../tests/draw/advanced_integration/README.md)
lists remaining device/numerical/failure-path checks.

See [integrated UV2 lightmaps](../tutorials/draw/12_lightmaps.md) for immutable
bake snapshots, exact completion and explicit material adoption.

Phase 1 temporal diagnostics are exposed through renderer reflection settings:
temporalDiagnosticView (Off/Tokens/LocalHistory), temporalDiagnosticSignal and
exact temporalDiagnosticLayer. Replay remains an independent default-off opt-in.
LocalHistory captures the kernel's actual accepted flag in optional GPU storage;
red means restart/first sample, green accepted, black uncovered and amber no capture.
The diagnostic ABI is separate from base v1/v2 and cannot change accumulated signal
values. Only active diagnostics allocate images/dispatch and use spatial reconstruction.
No ordinary CPU readback/wait/submission is introduced. See
[TemporalAccumulator](../api/draw/TemporalAccumulator.md) and
[probe instructions](../../tests/draw/temporal_surface/README.md).
Native acceptance remains pending; stable receiver identity does not detect changes
to indirect illumination on an otherwise stationary surface.
