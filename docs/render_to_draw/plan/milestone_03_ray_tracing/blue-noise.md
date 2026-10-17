# M03-03 — Generic BlueNoise data and GPU residency

Date: 2026-10-17. Entry revision: `15f67dff9efad51c878740bc1ddbce94f880ed46`, with inherited uncommitted M03-01/02 planning/GPU changes preserved. This step remains uncommitted. M03-02 is accepted by project-lead continuation; compilation/checks are assumed successful under the agreed convention, with backend/scenario coverage unspecified.

## Delivered contracts and ownership

[BlueNoise.hpp](../../../../lib/include/bg2e/utils/BlueNoise.hpp) introduces `utils::BlueNoiseData`, `sampleNoise2D` and `utils::BlueNoise`, exported through utils/all.hpp. The CPU view exposes tile size, channel/layer counts and borrowed immutable bytes; validates exact byte counts/overflow; wraps signed texel coordinates; rejects invalid layers and provides two-dimensional samples without a graphics device. Custom data owners outlive CPU views; GPU build copies their contents. The default is the existing 128x128, RG, four-layer ranked dataset.

The implementation includes the existing private `render/blue_noise_data.h` byte/metadata header without including any render API or native graphics headers. This deliberately preserves one authoritative dataset and leaves production BlueNoise, its shaders, data and generation/validation scripts untouched. The source path is historical, not a utils dependency on render::Engine. M09 must relocate that pure data header into utils and adjust the include before deleting the render directory. No independent duplicate generator/rank family is added.

GPU residency accepts a borrowed gpu::Device, not an Engine. `build(data)` creates a single sampled/upload/readback RGBA8 UNORM 2D image with nearest/repeat zero-LOD sampler, drains producers' completed work and synchronously uploads before publishing image/sampler/metadata together. Errors preserve the prior published snapshot. `cleanup()` drains and drops its references; copied shared image/sampler owners remain usable through replacement/cleanup. Device outlives the utility and all exported owners; stop producers before build/cleanup. No global cache, hidden frame-resource ring, implicit material-table allocation or RT capability requirement exists.

## Layout choice and native sampling

Four 128x128 logical layers are stacked vertically in a 128x512 atlas; channel bytes expand exactly to RGBA8 (missing RGB=0, missing alpha=255). This requires no resampling or filtering and uses the already implemented single-2D upload/view path. It is simpler than introducing physical 2D-array image/view/upload support solely for these small immutable tiles. Logical `layerCount()` is four; physical `arrayLayers()` is one. General image arrays remain unimplemented and must not be assumed by downstream work.

`shaderMetadata()` returns `(tileWidth,tileHeight,sourceChannels,logicalLayers)` as a 16-byte uint4. Atlas dimensions must fit signed native shader coordinates/addressable memory and native image allocation limits. `draw/lib/blue_noise.glsl` and `.metal` provide equivalent `bg2eBlueNoise2` and `bg2eHashNoise2` helpers. GLSL accepts separate texture2D/sampler arguments and constructs the combined sampler2D directly inside texelFetch; Metal reads texture2d directly. Neither uses sampler2DArray. Sampling wraps within each tile before adding the layer's Y offset; ordinary whole-atlas Y-repeat or filtered/mip/sRGB sampling is incorrect. The exported sampler is usable by normalized-sampling callers only with explicit atlas coordinates.

For two-channel sampling, logical layer is `(renderedFrame + sampleIndex*7) % layers` with intentional uint32 arithmetic. Map byte ranks to bin centers `(rank+0.5)/256`, add the fractional R2 rotation from the low 16 bits of `renderedFrame*233 + sampleIndex*61`, and wrap into [0,1). This retains the production sampling convention while avoiding unbounded float sequence conversion. The embedded four-layer sequence repeats every 65536 frame keys at a fixed sample/pixel; it is not a newly designed spatiotemporal distribution.

Noise-disabled/missing callers use `sampleNoise2D(nullptr,...)` and `bg2eHashNoise2`; the deterministic integer hash is explicitly not blue noise. GPU callers select a variant/path without any noise bindings, never a null/unbound sampler. Non-null invalid CPU data throws. Source rank storage supports 1..4 channels; two-dimensional sampling requires at least two. Effect/bake callers own rendered-view/job sample keys, quality and reset policy; acquired/presentation frames never advance utility state automatically. Reserving noise image/sampler entries in an M03 tracing table is an explicit caller decision.

## Static evidence and lead verification

Static asset inspection confirmed 131072 source bytes, metadata 128x128x2x4 and 64 occurrences of each byte rank per channel/layer. Source-byte SHA256: `9cc6319b4297d11b7c53a0e6ded94d96ecba467ff5aa9d88631777cf39fd5680`. The source was not regenerated; histogram inspection does not establish spectral quality. Existing generation and spatial-diagnostic scripts were not executed.

The [manual fixture](../../../../tests/utils/blue_noise/README.md) delivers CPU contract checks and paired compute/readback checks outside RendererDeferred. It compares every atlas byte and native noise/hash sample with CPU values, exercises wrap/layer/near-overflow keys, failed/successful replacement and retained old owners after cleanup. No tests were executed by the agent. Lead compilation/runtime validation remains required on Vulkan/Metal, including scalar/no-RT reuse and current production callers.

Agent checks: source signatures/includes/public exports, existing upload/readback and shared-owner lifetimes, overflow/metadata layout, CPU/GLSL/Metal arithmetic and native bindings, paired shader discovery, original data shape/histograms, new source licenses/delimiters/dependency boundaries, scoped Markdown links and git diff --check. No configure/build, shader compilation, executable/test launch or packaging. CMake, gpu image primitives and production sources/shaders remain unchanged by this step.

Next is M03-04 (RTAO). It and GI/reflections/bake reuse this actual atlas/data/sampling contract rather than copy production sampler2DArray code or add a separate noise dataset. Reusable tracing snapshots, effects and histories are not delivered here.

## M03-03 shader compilation correction

The lead reported glslang rejecting probe.comp.glsl:36 because a constructed sampler2D was passed to a user helper (`sampler constructor must appear at point of use`). The GLSL helper now accepts separate texture2D and sampler arguments and combines them only in texelFetch. The probe passes the separate bindings directly. Bindings, atlas/sequence math, CPU/Metal contracts and fallback are unchanged. Static call-site/signature/point-of-use review and git diff --check completed; correction compilation remains with the lead.
