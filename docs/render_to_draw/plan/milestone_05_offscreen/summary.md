# M05 — OffscreenApplication and accumulated scene export

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M01–M04 reviews; GPU offscreen support exists but application execution has no experimental equivalent yet.

M02 makes overlays optional through `RendererConfiguration::overlays`; the
manipulation example's final-output overlay is editor presentation state, not an
implicit scene signal. M05 must explicitly state whether an offscreen sample
includes configured overlays and must never count synchronous picking/viewpoint
submissions as rendered-scene samples. See [M02 review](../milestone_02_manipulation/review.md).

## Working product

A runnable `examples/draw/13_offscreen_application`, based on deferred_renderer, writes one completed scene image into the directory passed on the command line. Output/internal rendering resolution, number of accumulated frames, backend and an exported graphics-settings file are configurable; it starts without a window or UI.

## Scope exclusions

- No model_edit/composer/lightmap_generator copies and no standalone bake orchestration yet.
- No unrelated redesign of MainLoop; use its execution-selection pattern as the reference.
- No hidden window/UI initialization or reinterpretation of frames as mere presentation counts.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M05-01](step_01_execution-selection.md) | Public offscreen contracts and production/draw init overloads |
| [M05-02](step_02_draw-offscreen-context.md) | Windowless draw Engine and frame execution |
| [M05-03](step_03_accumulation-settings.md) | Headless configuration import and exact frame accumulation |
| [M05-04](step_04_working-example.md) | Build examples/draw/13_offscreen_application |
| [M05-05](step_05_readback-export.md) | Completed image readback, encoding and headless regression checks |
| [M05-06](step_06_review.md) | Review M05 configuration, accumulation and execution contracts |
| [M05-07](step_07_documentation.md) | Update maintained documentation cumulatively through M05 |

## Milestone exit gate

- The final [documentation step](step_07_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- OffscreenApplication keeps production initialization and adds an explicit draw overload selecting the corresponding execution implementation.
- A draw context/device/surface and non-template renderer delegate render headlessly with actual scene updates/samples.
- The requested frame count produces an accumulated scene image, not just N warm-up calls whose last frame is mislabeled as an average.
- CLI dimensions/internal scale/settings precedence, output conversion/orientation and completion are specified and validated.
- No-RT/default resampling output works, and optional effect/FSR availability is accurately exposed.
- M05 review revises M06–M10, especially headless configuration and standalone bake lifecycle reuse.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

The current Engine is window-only and the codec is usable without UI; those are distinct facts. Add actual generic/offscreen execution, direct stopped-boundary import and exactly N linear scene samples. Reuse existing tight raw color readback; FSR SDK state is not averaging or export. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M05-07](step_07_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).

## M03-06 reflection contract

[Reflection handoff](../milestone_03_ray_tracing/rt-reflections.md) defines output-relative quality independent of internal surfaces and unpremultiplied HDR hit radiance plus fractional confidence. Freeze scene/receiver/quality while advancing rendered sample keys for requested samples; accumulation must distinguish confidence from GI validity and retain IBL fallback for misses. Existing shared snapshot completion/ownership rules apply.

## M03-07 temporal boundary

[Temporal handoff](../milestone_03_ray_tracing/temporal.md) supplies independent effect/view/layer histories, bounded nominal and per-pixel convergence, rolling/frozen settings and explicit reset generation. It is not an exact UV bake average or a requested-frame scheduler. Select actual submitted scene samples, retain confidence semantics, freeze scene state and distinguish nominal frameCount from pixel counts restarted by rejection. Explicit stationary surfaces can use same-UV correspondence; define the bake signal separately.

## M03-09 integration contract

Read [integration](../milestone_03_ray_tracing/integration.md). Reuse public
requested settings and matching metadata instance addresses independent of device
availability. FinalProcessor/SMAAProcessor apply optional final opaque color AA
after reconstruction/color and before overlays/UI; default remains resampling.
Headless device-level SMAA returns borrowed source or fresh owned RGBA16 output
under an explicitly retired slot/serial contract. Do not filter exact bake averages
with spatial AA or reuse denoised display histories for sample accumulation.
Legacy query shadow overflow now reports sceneSupported/status and neutral whole
scene visibility; advanced material tables retain their independent capacities.
Review the lead validation matrix before claiming backend coverage.

## M03 reviewed dependency

[M03 technical review](../milestone_03_ray_tracing/review.md) is the actual source/ownership/signal/settings baseline. Its pending-plan impact table and the concrete additions in this milestone's step files supersede historical statements that explicit profiles or effect controls are still unimplemented. Keep exact averaging, chart isolation, required-job capabilities and actual validation evidence distinct from interactive fallback/history behavior.


## M04 reviewed dependency

The [M04 technical review](../milestone_04_lightmaps/review.md) records actual
integrated bake/sample/result ownership, RGB UV2 material semantics, preview
implementation and native shader deployment. Its impact table and concrete
pending-step requirements supersede assumptions that M04 already implements a
headless scheduler, batch writer, full GPU-only resource export or camera-image
averaging. Continuation acceptance does not establish hardware-specific numeric,
performance or exported-file evidence. Preserve production references until M09.


## M05-02 implemented baseline

[Windowless execution](headless-execution.md) now owns the generic draw context and
serial completed-frame driver. The public default offscreen delegate reuses the
same polymorphic renderer/scene/generation lifecycle as the windowed delegate,
with overlays excluded, one default requested frame and configurable fixed scene
time. Slot defaults use the generic surface; window/UI entry points retain their
window accessor and reject headless contexts. M05-02 is delivered awaiting lead
validation. Exact image averaging/import/CLI/encoding remain M05-03–05; neither
ordinary frame completion nor the manual probe closes this milestone.

## M05-03 implementation handoff

See [actual settings/averaging contracts](accumulation-settings.md). The public
reference accumulator uses synchronous completed RGBA16F readback and CPU double
sums; final color and optional SMAA run once after the resolved mean. No runtime
validation is claimed.


## M05-04 delivered example

[draw_offscreen_application](../../../../examples/draw/13_offscreen_application/README.md)
is the registered headless CLI target. Its scene reuses deferred example assets,
materials, transparency, environment and lights with camera/light animation off.
It writes a single non-overwriting scene.png after the exact accumulated result
is complete. CLI options, validation, settings import policy and output contract
are documented in its README. M05-05 verifies the temporary PNG by decoding it
and comparing exact RGBA bytes and dimensions before publishing scene.png. Row
zero is the top row, alpha is preserved, and no additional export color conversion
is applied. Independent visual inspection and cross-backend orientation checks
remain for the project lead. M05-06 records the source-backed contracts, verification
limits and M06–M10 pending-step impact in [review.md](review.md); the next required
step is the cumulative maintained-documentation update, M05-07.


## M05-07 documentation closure delivery

[Coverage and source mapping](documentation.md) records maintained reference,
architecture, tutorial 13, CLI/build/output and navigation updates through M05.
This final documentation step is delivered awaiting project lead acceptance.
Continuation closes M05 and selects M06-01; no later application is delivered by
this milestone. Runtime/backend/artifact evidence remains unspecified beyond the
lead continuation convention.
