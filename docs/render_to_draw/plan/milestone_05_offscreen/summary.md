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
