# TemporalAccumulator and camera motion

`<bg2e/draw/pipeline/TemporalAccumulator.hpp>` and `<bg2e/draw/pipeline/MotionVectorGenerator.hpp>` are included by draw/all.hpp. They provide public experimental draw histories independently of FSR's SDK context and private RendererDeferred targets.

RendererConfiguration::temporal accepts a compatible shared component. A null value selects the default TemporalAccumulator, enabled with maxFrames=32. RendererDeferred::temporal() exposes controls. Initialize an injected component with TemporalConfiguration containing shaderRoot/prefix, draw.temporal.v1 and a maxHistories budget (default 256). The lifecycle, beginFrame, record, invalidate and availability methods are virtual. Unsupported formats or unavailable kernels produce a raw signal pass-through; no history images are allocated in that path. Histories do not require ray-tracing support.

```cpp
renderer.temporal()->setMaxFrames(64);
renderer.temporal()->setFreezeAtLimit(false);
renderer.invalidateHistories(); // At a synchronized application mutation boundary.
```

The renderer accumulates active AO, GI and reflection signals before lighting. Histories are keyed by signal kind, view and layer, independently of frame slots. Opaque layer zero and each transparent submesh have separate identities. The frame sequence is actual sceneFrameNumber; UI-only presentations, picks, overlays and paused retained scene images do not submit temporal samples. Presentation serial identifies an acquired, completed resource slot only.

Standalone clients call initialize(device,backend,slots), beginFrame(slot,serial,sceneFrameNumber) once for the retired slot, then record(TemporalInput) once per effect/view/layer. Submit that recording in sequence on the same Device queue before recording a dependent successor. Sequence numbers must be nondecreasing within a lifecycle; gaps for a key restart its convergence. Discarded/failed submission requires invalidate before further use, because recording publishes provisional chronology. Input is borrowed: sampled R16 visibility or RGBA16 radiance, SurfaceInputs, TemporalView with current unjittered/jittered matrices, external scene/reset generation and optional reflection roughness/threshold. CameraDepth and explicit world profiles reuse the public effect surface contract. Explicit stationary surfaces can set view.reproject=false; UV bake accumulation is still a distinct M04 contract.

TemporalResult::image is the sampled result. owner retains an immutable accumulated image; on disabled/unavailable pass-through it is empty and image borrows the original signal. Keep borrowed input images alive through completion. Histories own separate world position/coverage and signed normal/per-pixel count images at signal resolution. Surface and signal extents may differ; changes reset correspondence. Each submitted invocation retains both history versions, descriptors and uniforms in the acquired slot; the next frame never overwrites a live image. Returned owners may survive invalidate/cleanup but must be released before destroying the Device. Cleanup drains; logical invalidate retains submitted owners without waiting per frame.

Camera reprojection maps current reconstructed world positions into the previous jittered clip space. Prior position and world-normal tests reject disocclusion. Thresholds are absolute world distance plus relative camera distance, and a normal cosine. The previous/current unjittered matrices remain available through TemporalView and the separate MotionVectorGenerator camera reference. Its motion convention is previous-minus-current framebuffer pixels with negative clip Y; jitter alone produces no unjittered camera motion. It does not generate moving-object vectors or replace the existing FSR motion texture generator.

Visibility averages scalar [0,1] and returns one for uncovered pixels. GI averages valid RGB with alpha one; invalid GI clears the pixel history and preserves IBL fallback. Reflection RGB is accumulated weighted by fractional confidence, while confidence is averaged separately; output remains unpremultiplied. Eligible zero-confidence misses participate, but roughness-rejected/uncovered pixels cannot revive older reflections. Valid black GI/hits remain distinct from invalid signals.

maxFrames is 1..1024. Before the bound, accepted per-pixel samples use an incremental arithmetic mean. At the bound, freezeAtLimit=false uses rolling weight 1/maxFrames; true retains a converged accepted pixel until rejection/reset. frameCount(key) and TemporalResult::frameCount report a bounded nominal consecutive-view count, not proof that every pixel converged: reprojection rejection restarts individual pixel counts. Later headless jobs must select actual rendered samples explicitly. setEnabled, frame bound/freeze and rejection controls invalidate histories.

Renderer::invalidateHistories() is the generic application hook; RendererDeferred resets effect history generation and the active upscaler. resetUpscaler() remains compatible and now invokes that full hook. Scene attachment, resize, explicit camera changes, inspector changes and environment replacement reset it. An exact extraction/settings signature also detects mesh/material/resource identity, transforms, lights, camera identity/projection, effect quality/mode/noise and temporal/shadow settings. Node/submesh identity must be unique when temporal correspondence is active. Object movement conservatively resets histories and FSR; previous-object motion vectors remain future work. Notify invalidateHistories for camera teleports on the same scene camera, in-place image uploads and custom shader/resource edits that retain identity. Applications remain responsible for safe mutation boundaries and requested redraws, including partial editing failures. Generic material utilities acquire no renderer ownership.

See [ABI and implementation handoff](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/temporal.md) and [manual fixture](../../../tests/draw/temporal/README.md). Compilation/runtime validation remains with the project lead. [Spatial filtering](DenoiseFilter.md) is available after temporal accumulation.

RendererDeferred applies the optional public [DenoiseFilter](DenoiseFilter.md) after temporal resolution and before lighting; filtered output is separate from temporal history.

## Live controls and current use

Both existing examples expose this component through the live [settings catalog](Settings.md).
Compatible replacement metadata must return its matching settingsInstance address.
Requested enabled/settings remain serializable independently of availability/status;
check graph selection as well. [SurfaceInputs](SurfaceInputs.md) and
[TracingSceneResources](TracingSceneResources.md) describe reusable inputs/owners.
Device must outlive retained results/snapshots. No UI, exact bake-average or
headless application scheduler is implicitly created by a standalone effect.
