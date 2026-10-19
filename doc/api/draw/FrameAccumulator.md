# FrameAccumulator

Header: `<bg2e/draw/pipeline/FrameAccumulator.hpp>`.
Namespace: `bg2e::draw::pipeline`.

A synchronous reference arithmetic mean of an exact requested count of completed
linear camera-image samples. It is independent of rendered-view temporal histories
and UV-space lightmap accumulation.

| Method | Contract |
|---|---|
| `reset(gpu::Size2D, uint32_t)` | Start a fresh image/count; nonzero extent and 1..1048576 samples; validates host-size overflow |
| `add(std::span<const float> rgba)` | Exactly width×height×4 floats; finite signed RGB; source alpha ignored; validates all RGB before committing any contribution |
| `add(gpu::Image&)` | Completed output-sized single-mip/single-layer R16G16B16A16_SFLOAT image; synchronous tightly packed readback through GPU |
| `completedFrames()` | Successful contributions so far |
| `complete()` | Exactly the requested count has arrived |
| `resolve()` | Host float RGBA mean available only after completion; alpha is one |
| `resolveInto(gpu::Image&)` | Matching single-mip/single-layer RGBA16F destination; validates finite half-float range and uploads once |

No tone map, gamma, clamp, confidence weighting, reprojection or exponential
feedback is applied. Oversampling, premature resolve, invalid numeric samples and
wrong dimensions/format reject. Nonfinite RGB rejection preserves both earlier
sums and the completed count. Final half conversion rejects magnitudes above 65504.
Host sums use double precision with ordinary floating-point rounding; this is not
an exact real-number computation. Source samples and resolved GPU images remain
bounded by RGBA16F storage.

The reference pays one synchronous GPU readback and O(width×height) CPU work per
sample, then one final upload; host memory is O(width×height), independent of N.
GPU add requires a completed submission and TransferSrc-capable source; destination
upload requires TransferDst usage. It is not callable as an asynchronous command
recording pass. The accumulator owns its CPU sums, not the input/output image or
Device. Keep the Device and images alive through their operations.

Normally use [DefaultOffscreenApplicationDelegate](DefaultOffscreenApplicationDelegate.md)
to schedule contributions and final conversion. For custom integration, complete
all submitted work before add/reuse/cleanup and apply final color only once to the
resolved linear mean. [Architecture](../../architecture/draw_offscreen_rendering.md)
explains the difference from effect histories and bake maps.
