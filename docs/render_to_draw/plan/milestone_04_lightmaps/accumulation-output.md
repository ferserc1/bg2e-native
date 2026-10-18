# M04-04 accumulation, composition and typed output

Baseline HEAD: 95190d6d042bb85d6e9d72153b155192dac64377, preserving previous
uncommitted M04 work. M04-03 accepted by project lead continuation instruction;
backend/scenario coverage unspecified. Production render and GPU APIs are untouched.

Delivered public UvTemporalAccumulator and LightmapCompositionPass, configurable
native profiles and exclusive injected output owners through BakePassConfiguration.
Integrated recording now chains raw sample -> new RGBA32F sums/R32_UINT counts ->
last-update AO/GI composition -> exact nearest-original-chart padding. GPU outputs
are independent per invocation; completion commits history, discard/cancel does not.

The actual API, binding map, precision/validity/normalization/dilation policy and
CPU conversion are maintained in [LightmapOutput.md](../../../../doc/api/bake/LightmapOutput.md).
There are ten native shader files (five Vulkan/Metal pairs) across accumulation,
composition and dilation.
No duplicate lighting/material estimator, GPU primitive or CMake change was needed.
Float32 sums have normal arithmetic rounding; integer valid-update counts are exact.
The current public GI estimator saturates half-float output, so raw values at its
65504 boundary are conservatively rejected rather than accepted as unclipped HDR.

Final completeFrame confirms completion, validates every original covered texel's
requested valid updates and numeric state, reads the same padded RGBA32F image,
then publishes Complete/result/cached typed pixels. RGB32F preserves finite HDR;
RGB8 alone clamps/rounds, with exposure only for GI. Invalid/missing/saturated raw
GI, empty coverage, bad reference/division, overflow, stale or cancelled jobs cannot
publish. Incomplete or invalidated result/readPixels remain unavailable. Final
validation/readback is synchronous and can drain other Device work; intermediate
samples do not read back. This is not an asynchronous staging implementation.

Dilation scans each row/column within the requested radius, selects the nearest
original seed in Euclidean distance, and breaks ties by chart then seed index.
Original interiors are fixed; padding copies one chart's original composed value.
No neighbor averaging, cross-chart normal filter, iterative padding feedback or
presence inference occurs. Radius zero is valid; large radii remain potentially
expensive at O(width*height*radius), without a silent setting cap.

[Manual fixtures](../../../../tests/bake/output/README.md) cover byte/float codec,
known two-sample AO/chart padding and known GI irradiance normalization/zero divisor.
The integrated fixture now expects validated final publication; its immediate-submit
callback was corrected to the actual CommandBuffer* API. No fixture was compiled
or executed. Static checks and a script-level nearest-seed/reference comparison are
agent evidence only, not native numerical/backend validation.

M04-05 implements mesh/UV preview and UI image ownership; it can consume retained
validated LightmapResult images, not infer validity from RGB. M04-06 applies UV2
metadata and demonstrates live-indirect-off usage. M05 exports typed pixels; M08
reuses the Device primitives with its own scheduler. Optional DenoiseFilter remains
explicitly rejected until a chart-compatible profile is supplied; raw accumulation
and nearest-chart dilation themselves are chart isolated.
