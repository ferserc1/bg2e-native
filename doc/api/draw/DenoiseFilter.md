# DenoiseFilter

`<bg2e/draw/pipeline/DenoiseFilter.hpp>` provides public scalar/HDR bilateral filtering in experimental draw, independently of production render. draw/all.hpp includes it.

RendererConfiguration::denoise accepts a compatible shared component; null selects an enabled default filter with radius two. RendererDeferred::denoise() exposes it. Active effects flow through raw tracing, temporal accumulation, spatial denoise, then lighting composition. Filtered images never replace temporal history or exact bake accumulators. Disabled temporal accumulation can still feed raw signals into the filter; disabled/unsupported filtering preserves the accumulated signal.

```cpp
renderer.denoise()->setKernelRadius(2);
renderer.denoise()->setDepthThreshold(0.1f);
renderer.denoise()->setNormalThreshold(0.8f);
renderer.denoise()->setSpatialSigma(2.0f);
```

Mutate settings at the application synchronized edit boundary and request a scene frame. The renderer observes virtual getter values in its settings signature and coordinates history/SDK invalidation. Getters/setters are virtual; settingsType() returns live accessor metadata, and settingsInstance() returns its matching base instance address. Bind these together when adding a SettingsCatalog target, with the shared component as owner. A subclass exposing its own metadata must also return the corresponding instance address. No settings value copy is enumerated. Visible controls/preferences bind the live denoise target in both existing examples; see [settings](Settings.md).

Settings include enabled, kernelRadius (0..8), finite nonnegative depthThreshold, normalThreshold (cosine in [-1,1]), depthSigma, normalSigma and spatialSigma (each [1e-6,1e6]). Radius zero returns the borrowed source unchanged. The sigma bounds prevent zero/overflow denominators. Spatial sigma is in signal pixels; normal sigma applies to 1-cosine. Camera depth comparison uses absolute linear view-space Z distance in world units. Explicit world surfaces use Euclidean world-position distance, so geometry-discontinuous neighbors can be rejected without a camera. There is no chart-ID input: adjacent charts with similar world guides can still bleed. This is not a chart-isolated atlas filter.

Standalone clients construct DenoiseConfiguration with deployed shaderRoot/prefix, compatible draw.denoise.v1 and per-slot maxInvocations (default 256), then initialize(device,backend,slots). beginFrame(slot,serial) releases only the caller's completed slot. record(DenoiseInput) filters one (signal kind,view,layer) key per serial. Input borrows a sampled R16_SFLOAT visibility or RGBA16_SFLOAT radiance signal and SurfaceInputs; surface and signal extents may differ. CameraDepth requires a finite invertible affine worldToView matrix and the existing jittered inverse view-projection in SurfaceInputs. Explicit WorldPositionCoverage needs signed normals, positions and coverage, without camera reconstruction. Reflection input additionally requires matching sampled receiver material (RGBA8 UNORM or RGBA16 float, G=roughness) and maxRoughness.

DenoiseResult::image is the result, with owner retaining the fresh filtered image. For disabled/radius-zero/unavailable profiles, owner is empty and image borrows input.signal. Keep caller-owned signal/surface/material alive through completion. Each active invocation retains its own output/uniforms/descriptors in the acquired slot, so other effects/views/layers cannot overwrite it. Returned owners can survive cleanup but Device must outlive them. Cleanup drains; there is no per-frame waitIdle. Output matches input extent/format and has sampled/storage/readback usage. No R8 scalar conversion, fixed camera GBuffer dependency or borrowed mutable scratch image is introduced.

Bilateral weights combine pixel distance, depth/world distance and normal agreement, with hard geometry thresholds. Out-of-bounds neighbors are skipped and weights normalized at borders. Constant HDR values are preserved; radiance is never clamped to [0,1]. Uncovered/nonfinite visibility becomes one. GI invalid centers stay zero-alpha; valid black remains valid, and invalid neighbors are excluded. Reflections average confidence independently and weight RGB by confidence, returning unpremultiplied RGB. Eligible misses participate with zero confidence; roughness-rejected centers remain invalid and cannot be filled by nearby hits. Explicit geometry guides do not infer validity from black RGB.

available() reports any usable profile; available(kind,surface) reports exact scalar/HDR and camera/world support. Missing format or shader profiles return raw input, with status() describing initialization diagnostics. The filter requires no RT support. Configuration and virtual lifecycle/record methods permit compatible shaders, application resources and alternative allocation/filter implementations.

See [native contract and limitations](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/denoise.md) and [manual fixture](../../../tests/draw/denoise/README.md). Compilation/runtime validation remains with the project lead.
