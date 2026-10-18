# Lightmap accumulation and output

Include `<bg2e/bake/all.hpp>` or the public UvTemporalAccumulator and
LightmapCompositionPass headers. The integrated baker now records both primitives
in the same command as each raw sample, commits history after confirmed completion,
and publishes a current LightmapResult only after final GPU completion and strict
readback validation. No camera, renderer initialization or production render API
is needed by these Device-backed output primitives.

## Accumulation

UvTemporalAccumulator accepts BakeSample and the immediately preceding completed
AccumulationResult. Source/target generation, geometry, mode, extent and zero-based
sampleIndex must match. Per-slot serials advance strictly. The caller supplies a
completed slot and retains owners until submission completion; generic GPU APIs
cannot inspect fence state. Fresh outputs keep the previous committed history
unchanged if recording/submission is discarded. Integrated cancellation never
commits a pending history; a fresh snapshot/serial can retry a discarded update.

Sums are RGBA32F (RGB raw sums, A sticky error), counts are R32_UINT valid update
counts. Each raw signal is already a samplesPerPixel estimate, so frozen equal
ray counts allow equal update weighting. These are ordinary float32 sums with
normal rounding, not rolling means or denoised display histories. Integer counts
are exact up to the settings limit of 1048576 updates. No receiver albedo, exposure,
clamp or irradiance normalization is applied to raw history. Blue-noise/sample
keys remain the integrated baker's completed update index.

Covered AO requires finite visibility in [0,1], replicated RGB. Covered GI requires
finite nonnegative RGB and alpha exactly one; valid black increments the count.
Invalid samples set a sticky error and do not increment it. Coverage/chart changes
also set errors. Uncovered texels have zero count/sum. The current public GI pass
uses RGBA16 and saturates at 65504, so this accumulator conservatively rejects any
channel at or above that boundary. It can reject an exactly representable boundary
value; it never silently accepts saturated HDR as a valid float lightmap. NaN,
infinity, sum overflow and invalid/missing covered samples fail final validation.

validateCompleted(result,requestedUpdates) is for completed submissions only.
It reads native float/uint/byte color payloads using the existing packed readPixels
API. Every covered texel must have all requested updates, no sticky error, and a
valid positive chart. Empty coverage fails. Missing or partially valid covered GI
is a failed job, not a successful neutral fallback. No new GPU staging primitive
or implicit format conversion is introduced.

## Composition and dilation

LightmapCompositionPass records after the final accumulation. AO is the exact
count-normalized visibility, replicated RGB. GI divides the count-normalized
incoming radiance/pi by environment irradiance/pi sampled at the signed world
normal, channelwise. A zero reference channel produces zero. Receiver albedo and
PI are never applied again. The same immutable environment snapshot/sampler is
retained as the raw GI estimate. Invalid/nonfinite/negative reference or output
fails validation. The output and intermediate multiplier are RGBA32F: HDR above
one and above half-float range remains unclipped if float32 can represent it.
Alpha is always one, independent of coverage/validity.

The dilation profile uses original R32_UINT charts and an RG32_UINT nearest-seed
image. A horizontal scan finds the nearest covered original seed in each row; a
vertical scan selects the globally nearest candidate within the requested
Euclidean pixel radius. Ties use chart ID, then original row-major seed index.
Covered interiors always retain their own composed value; uncovered padding
copies one original texel, never an average or a previously dilated value. Thus
adjacent charts cannot exchange interior values even with identical world guides.
Uncovered pixels outside every radius remain neutral white. Radius zero is valid.
Coverage and chart masks remain original in AccumulationResult.surface; padding
never becomes evidence of valid accumulated samples or baked presence.

The two scans are O(width*height*radius), clipped to atlas bounds, with a constant
number of dispatches. Large user-selected radii remain potentially expensive;
the setting is not silently capped. There is no iterative descriptor allocation
per padding pixel or quadratic radius search. All writes have explicit transitions
before subsequent sampled reads. Native Vulkan/Metal profiles share bindings,
16-byte parameters, integer seed arithmetic and top-row/texture-v=0 orientation.

## Integrated publication and typed pixels

IntegratedLightmapBaker::record records raw effect, accumulation and, on the last
update, composition plus dilation. completeFrame follows actual GPU completion,
checks scene/target again, validates the final sums/counts/masks, then reads the
same dilated GPU image. It marks Complete and publishes image/CPU pixels only if
all checks succeed. The last counter increment also occurs after validation. A
final numeric/readback failure leaves Failed with no result; cancellation or stale
generations never publish it. Reset clears history/output/cache at a stopped
boundary. Accumulation/composition resources belong to the context so destroying
a target handle while commands are pending does not release them.

Final validation/readback is synchronous at completion and can wait for other
Device submissions. It is not an asynchronous staging implementation. Intermediate
samples do not read back. readPixels() returns cached typed data for the current
completed job after source/target checks; result() returns empty for stale/missing
jobs. The image is the same padded RGBA32F texture sampled by consumers; CPU export
does not recompute a different border treatment.

LightmapCompositionPass::convertRgba32 is a GPU-free checked conversion helper.
RGB32F strips alpha while preserving finite nonnegative linear HDR, including
black and values above one; it ignores exposure. RGB8 applies 2^exposureEV for GI
only, clamps to [0,1], and rounds floor(value*255+0.5). AO ignores exposure. No
gamma, tone mapping, premultiplication or receiver material factor is applied.
Invalid floats and non-one alpha are diagnosed before byte clamping. Rows stay
v=0 first with tightly packed RGB output. The raw codec and mode remain explicit.

## Configuration, ownership and remaining work

AccumulationConfiguration and CompositionConfiguration expose shaderRoot, profile
prefixes and interface IDs (bake.accumulation.v1 / bake.composition.v1).
BakePassConfiguration accepts exclusive accumulator/composition subclasses in
addition to UV/AO/GI passes. They start clean, initialize at the stopped job
boundary, preserve exact counts/raw signal semantics and retain partial-command
owners. Their returned geometry/generation metadata is validated. Required storage,
sampled/readback formats and shader pipelines must exist; no lower-precision or
neutral unavailable fallback is accepted. cleanup drains before releasing slots.

Native binding map (set zero): uniform binding 0 is Metal buffer 1, nearest sampler
binding 1 is Metal sampler 0. N sampled inputs use Vulkan bindings 2+i / Metal
textures i; storage outputs follow at bindings 2+N+j / textures N+j. Accumulation
N=6 inputs are raw signal, previous sums/counts, coverage, chart, previous chart;
its outputs are sums/counts. AO composition N=4 uses sums/counts/coverage/normal;
GI adds irradiance cube as input 4 and sampler binding 8 / Metal sampler 1. The
seed pass reads chart only; the apply pass reads composed multiplier/chart/seed.
Accumulation's uvec4 is (mode,hasPrevious,sampleIndex,reserved); composition's is
(mode,resolution,dilationPixels,requestedUpdates). Mode 0 is AO, 1 is GI.

Each retained slot keeps its invocation descriptors, uniforms and direct input/
output images; it does not retain an unbounded chain of older history objects.
External results keep their own images alive after slot reuse/reset. Device and
Engine outlive every owner. In-place resource contents still require explicit
source invalidation; finished maps should be retained before applying a scene
material edit that invalidates the job.

Optional DenoiseFilter remains rejected because its profile has no chart guide;
there is no chart-unsafe smoothing of raw averages. UV/result previews and
material adoption are implemented by the [integrated example](../../tutorials/draw/12_lightmaps.md).
GPU-only texture ownership is not image/model export. See
[integrated lifecycle](IntegratedBakerContext.md),
[manual output fixtures](../../../tests/bake/output/README.md), and
[implementation handoff](../../../docs/render_to_draw/plan/milestone_04_lightmaps/accumulation-output.md).
