# M05-03 — Headless settings and exact scene averaging

[Step](step_03_accumulation-settings.md) · [Progress](../progress.md) · [Execution](headless-execution.md)

## Implemented boundary

DrawOffscreenApplicationConfig adds frames (default 1, range 1..1048576),
graphicsConfig, an optional borrowed persistedDefaults service, unknownSettings,
internalExtentOverride and scaleOverride. Output extent is the job target, never
an imported display/window size. The driver validates dimensions/count/timestep
and conflicting or out-of-range overrides, reads the strict settings JSON, then
calls configure before allocating the Engine. Target/type validation uses the
actual initialized catalog before submitting any scene sample.

DefaultOffscreenApplicationDelegate binds RenderSettingsPreferences directly at
the stopped initial prepareFrame boundary. No UI or connect/queue is used.
Precedence is live defaults, explicitly opted-in persistence if there is no
explicit document, explicit document, then explicit internal extent or scale.
An explicit document bypasses persistence, following the existing settings
convention; omitted properties retain live defaults. Frames is a job argument,
not a duplicated renderer preference. Only one resolution override is allowed;
fixed extent disables the imported scale policy. Scale enables the actual
ResolutionSettings policy; output extent is unchanged. Requested unavailable
RT/FSR settings remain those of the ordinary renderer/fallback.

Unknown IDs reject by default. SkipUnknownOrUnavailable is explicit and warnings
remain in settingsResult(). Override extendSettingsCatalog to map application IDs
such as example-owned lightPose targets to actual headless scene objects; no
implicit light/name matching occurs. Malformed imports fail; a failed environment
load is not treated as a successful import of the previous image.

## Linear sample and final conversion contracts

Renderer has an optional supportsLinearRecording/recordLinear/recordFinal seam
and sceneFingerprint. Existing window-only custom renderers need not implement
it. Custom accumulating renderers opt in, supply the documented linear image and
stable scene identity, and extend the live settings catalog with their renderer
ID. RendererDeferred implements the seam. Ordinary record still calls both
stages once, preserving interactive behavior and injected final-pass dispatch.
Split calls validate the matching updated slot/serial and prohibit duplicate
linear/final recording. The default renderer returns its retained reconstructed
RGBA16F output after all sky/lighting/transparency and optional reconstruction;
its image has transfer usage for public synchronous readback/upload.

pipeline::FrameAccumulator is independent of TemporalAccumulator and
bake::UvTemporalAccumulator. It averages finite signed linear RGB with one equal
weight per completed sample, ignoring source alpha and resolving alpha to one.
The reference implementation synchronously reads tight RGBA16F samples, sums RGB
in CPU double precision, divides once after exactly N contributions, and uploads
the resolved RGBA16F mean. resolve() also exposes float host values after N.
Ordinary floating-point rounding applies; the final half conversion occurs once,
not once per running average. Nonfinite RGB, premature resolve, wrong dimensions,
oversampling and invalid counts reject. No tone mapping, gamma, clamp, coverage,
confidence weighting, exponential feedback or reprojection occurs in this object.
A rejected numeric sample commits neither partial pixels nor its count.

This implementation deliberately pays one synchronous GPU readback and CPU
O(width*height) work per sample, plus one final upload; it is a correctness-first
public reference, not a GPU throughput implementation. It uses the existing
public native staging APIs, with no new backend dependencies or shaders. Host
memory is O(width*height), independent of N. Final resolved image storage remains
RGBA16F, matching the existing reconstruction/final pass ABI.

## Scheduler, completion and ownership

Each scene immediateSubmit finishes before completeFrame reads and contributes
the sample. Exactly N such completions are counted. On the last contribution,
the driver submits resolveFrame once using the same acquired slot/serial; only
recordFinal runs there, including live final color/tone and optional FinalProcessor
(SMAA). It does not update/traverse/render another scene sample. completeResolve
publishes hasResult only after that submission completes; didRenderFrame follows
with the surface in TransferSrc. Intermediate callbacks have hasResult false.
Read/export only in the last callback while the surface/device are alive.
completedFrames remains inspectable after cleanup; surface/image ownership does
not survive application teardown. finish rejects early termination before N or
missing final completion. Internal completion hooks are final overrides so a user
didRenderFrame override cannot accidentally bypass counting or publication.

The generic DrawOffscreenApplicationDelegate retains its independent continuation
policy. Its new configuration/completion/resolve/finish hooks default to no-ops;
existing custom frame drivers still request their own number of ordinary frames.
The default accumulating delegate uses config.frames; do not override continuation
to request additional samples. Overlays are excluded and ordinary helpers/readback
submissions never count as scene samples. Cleanup retains pending image owners
through the application's GPU drain and clears settings bindings before renderer
resources disappear.

## Time, histories and mutations

Default zero timestep freezes scene time; the first delta is always zero. History
state resets before sample zero. AO/GI/reflection histories and FSR then advance
normally for every requested sample, including the first: no hidden or discarded
warm-up frames. The uniform mean is over the fully reconstructed scene signals;
correlated/history-filtered samples are not independent raw Monte Carlo samples.
No-RT/no-effect raster with ordinary resampling is valid (stationary identical
samples give the same image); unavailable FSR keeps reduced internal resolution.
N=1 uses the same reconstruction and final conversion with an identity average.

Nonzero fixed timestep explicitly averages an animation sequence: sample zero
at delta zero, then each subsequent sample at the requested timestep. Settings,
renderer generation, root identity and output extent remain fixed. Frozen jobs
also compare the actual non-jittered camera, scene/material/texture identities,
lights and renderer history generation after update. A mutation rejects the job
instead of mixing generations. Start a fresh job to restart. Opaque writes into
existing GPU allocations cannot be inferred from pointer identity: callers must
serialize edits outside the job or explicitly invalidate histories, which causes
a frozen in-progress job to reject. Example-owned frame-counter animation must be
disabled even if delta is zero. Built-in G-buffer final debug views reject because
they do not consume the averaged reconstructed scene; lighting diagnostics remain
intentional linear inputs to the mean. Custom final passes must honor the same
linear-to-final-only contract.

## Validation and handoff

No configuration, build, shader compilation, C++ fixture or GPU execution was
performed. Agent checks cover source signatures/callers, resource lifetime,
completion order, transfer usage, full GPL headers, lexical delimiters, includes,
local documentation links and git diff --check.

Manual fixtures: tests/draw/frame_accumulation checks known 1/3 mean 2, signed HDR,
N=1, resets, rejection atomicity and half transfers through a host Image fixture.
The optional deferred branch of tests/draw/offscreen also compares two completed
HDR inputs 0.25/0.75 against one HDR input 0.5 through the actual final pipeline,
checks scale/output separation, exact scene counts, one final conversion and
publication timing. These are authored checks, not observed passing results.
Project lead should compile and run them on available Vulkan/Metal backends,
check UI imports/unknown IDs/override precedence, failure cleanup, frozen mutation
rejection and nonzero timestep, and regress ordinary windowed rendering and
production offscreen execution. See the fixture READMEs for concrete scenarios.

M05-04 owns the actual scene-bearing CLI and argument parsing; M05-05 owns encoded
output/orientation/color-domain checks. No encoded image product is claimed here.
M08 reuses settings import and headless execution but retains independent UV
sample scheduling/coverage and bake accumulation.
