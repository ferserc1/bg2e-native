# M05-03 — Headless configuration import and exact frame accumulation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M05-02 and M03 histories/settings codec.

## Scope and deliverable

- `lib/include/bg2e/draw/RenderSettingsPreferences.hpp`
- `lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp`
- `lib/include/bg2e/draw/pipeline/FrameAccumulator.hpp`
- `lib/include/bg2e/app/DrawOffscreenApplicationDelegate.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Define CLI/config precedence: defaults, persisted defaults if explicitly enabled, imported graphics document, then explicit CLI resolution/frame overrides. Use the M01 codec with active headless renderer/upscaler targets.
2. Define --frames N as exactly N rendered-scene samples for one frozen view by default. Reset all relevant histories at job start; presentation counts and unrendered updates do not count.
3. Accumulate the scene in a declared linear/HDR domain before final encoding using public pipeline accumulation support. If existing TemporalAccumulator only provides reprojected/EMA histories, add the smallest public frame-average mode/object rather than mislabel warm-up as accumulation.
4. Specify behavior with FSR and effect histories, jitter, animated scenes/fixed timestep and N=1. Preserve separate output extent and lower internal resolution; no-RT/no-effect accumulation is still meaningful and valid.

M02 overlays are optional final-output passes, and its selection/viewpoint readbacks
are synchronous editor jobs outside scene-frame submission. Declare whether
offscreen output includes any registered overlay; default scene averaging should
exclude editor selection/gizmos unless the job explicitly opts in. Only completed
scene samples count toward N. A MaterialModifier/editor action or scene reload
must be serialized outside the frozen accumulation interval or reset the job's
sample/history generation before further samples (see [M02 review](../milestone_02_manipulation/review.md)).

## Verification and exit conditions

- N=1 matches the single-frame conversion; a controlled varying sample sequence verifies the configured accumulated result and N stopping condition.
- An exported UI configuration imports without UI; malformed dimensions/counts/config types fail before starting the job and unsupported options yield declared diagnostics.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M05-04/M05-05 demonstrate output semantics. M08 imports the same graphics configuration but has independent UV-sample accumulation rules.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-12 headless settings handoff

Use [the live catalog and codec](../milestone_01_deferred_renderer/reflected-settings.md) directly, with no window/UI/delegate construction. Bind actual offscreen renderer/effect objects through SettingsTarget and stable component/type/property IDs; serialize/validate/apply using SettingsCodec at a stopped boundary. Numeric strings preserve exact uint32/double/int64 enum values. Reject unsupported options by default or explicitly skip with diagnostics. Explicit imported files bypass persisted defaults; omitted properties retain current live values. Never route this protocol through legacy JsonNode parse/toString or duplicate values in an offscreen preferences model.

## M01-13 resolution/upscaler contracts

Read [upscaling.md](../milestone_01_deferred_renderer/upscaling.md). Reuse ResolutionSettings and open Upscaler component identifiers/live catalog settings. Resolve internal extent from the renderer policy, keep reconstructed HDR/output extent separate and call explicit safe resize/reset for offscreen callers without DefaultRenderLoopDelegate. FSR SDK accumulation does not satisfy this step's required scene accumulation/output semantics.

## M01-16 reviewed baseline

Use actual SettingsCatalog/SettingsCodec and renderer/upscaler target identifiers; facade connect/queue methods require a windowed DefaultRenderLoopDelegate and are not the headless execution interface. Direct validated import applies at the new stopped job boundary. Current product startup explicit file bypasses persisted defaults; this step intentionally adds its documented CLI override/precedence policy. SDK FSR state cannot replace exact linear scene sample averaging. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M03-06 reflection contract

[Reflection handoff](../milestone_03_ray_tracing/rt-reflections.md) defines output-relative quality independent of internal surfaces and unpremultiplied HDR hit radiance plus fractional confidence. Freeze scene/receiver/quality while advancing rendered sample keys for requested samples; accumulation must distinguish confidence from GI validity and retain IBL fallback for misses. Existing shared snapshot completion/ownership rules apply.

## M03-07 temporal boundary

[Temporal handoff](../milestone_03_ray_tracing/temporal.md) supplies independent effect/view/layer histories, bounded nominal and per-pixel convergence, rolling/frozen settings and explicit reset generation. It is not an exact UV bake average or a requested-frame scheduler. Select actual submitted scene samples, retain confidence semantics, freeze scene state and distinguish nominal frameCount from pixel counts restarted by rejection. Explicit stationary surfaces can use same-UV correspondence; define the bake signal separately.

## M03-10 reviewed implementation requirement

Implement the planned FrameAccumulator as exact linear HDR scene averaging, independent of bounded/reprojected TemporalAccumulator. Freeze scene and explicit sample keys; define whether effect histories/FSR warm up or contribute and count only requested completed scene samples. Final tone/color and optional SMAA must operate on the resolved average once. UI exports may contain example-owned lightPose targets: provide explicit scene-ID mapping/catalog extensions or reject/skip with diagnostics, never silently import into unrelated objects.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
