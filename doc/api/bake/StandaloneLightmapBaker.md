# StandaloneLightmapBaker

**Header:** `<bg2e/bake/StandaloneLightmapBaker.hpp>`.
**Namespace:** `bg2e::bake`. Construct through
[StandaloneBakerContext::createBaker](StandaloneBakerContext.md).

One exclusive target job implements LightmapBaker. LightmapBakerConfiguration
contains a retained target Node and shared live LightmapSettings. A validated
settings copy freezes on reset; editing the caller's settings does not change
an already running accumulation. Source/target changes require the appropriate
invalidation/repreparation, not mutation during submitted work.

## Execution and inspection

| Member | Meaning |
|---|---|
| `update()` | Record/submit/complete one synchronous valid UV sample. Complete, cancelled, failed or stale jobs reject further updates. |
| `progress()` | BakeState, completedSamples/requestedSamples, sourceGeneration/targetGeneration and status. |
| `configuration()` | Retained target and live settings object; stop before editing. |
| `frozenSettings()` | Actual settings used by this job. |
| `latestSample()` | Current valid completed raw sample, or null if unavailable/stale. |
| `result()` | Optional current complete LightmapResult, including retained GPU image and explicit UV2/mode/generations. |
| `readPixels()` | Typed cached RGB8/RGB32F completed pixels; throws if incomplete/stale/missing. |
| `resetAccumulation()` | Freeze settings again, new target generation and clean history within the same current scene. |
| `cancel()` | Cancel further publication and clear result/sample/pixel handles. |
| `cleanup()` | Stopped idempotent release; borrowed Engine/Device must still be alive. |
| `record(const BakeFrame&)` | Generic contract implementation; accepts only the exact internal update command once. External command recording is unsupported. |

States are Ready, Running, Complete, Cancelled, Invalidated and Failed. Requested
samples mean accumulation updates, independently of samplesPerPixel/ray count.
Only confirmed valid GPU updates count. Exact internal slot/serial/source and
RecordingCompletion are checked. Final composition, nearest-chart dilation and
strict numeric/count/readback validation precede Complete/result publication.
An exception or abandoned recording cannot publish its partial accumulation.

Results are linear indirect RGB multipliers, not display frames, camera temporal
history or receiver albedo. Covered counts/chart masks define validity; alpha one
is not coverage. [Output semantics](LightmapOutput.md) describes raw GI precision,
normalization, neutral uncovered texels and RGB8 exposure versus float HDR.

A cancelled job may reset only while its original source generation remains
current. Stale scene jobs need new preparation and a new baker. A context retains
only one active job; `cleanup()` alone is not releasing the handle. Reset the
shared baker pointer before createBaker for the next target. Retain returned GPU
owners only inside the Engine/Device lifetime. Stop producers before cancelling,
resetting or cleaning; a signal handler should set a CPU request, not call GPU IO.

Sources: [header](../../../lib/include/bg2e/bake/StandaloneLightmapBaker.hpp),
[implementation](../../../lib/src/bg2e/bake/StandaloneBakerContext.cpp),
[opt-in lifecycle probe](../../../tests/bake/standalone/README.md).
