# M05 — OffscreenApplication and accumulated scene export

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M01–M04 reviews; GPU offscreen support exists but application execution has no experimental equivalent yet.

## Working product

A runnable `examples/draw/offscreen_application`, based on deferred_renderer, writes one completed scene image into the directory passed on the command line. Output/internal rendering resolution, number of accumulated frames, backend and an exported graphics-settings file are configurable; it starts without a window or UI.

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
| [M05-04](step_04_working-example.md) | Build examples/draw/offscreen_application |
| [M05-05](step_05_readback-export.md) | Completed image readback, encoding and headless regression checks |
| [M05-06](step_06_review.md) | Review M05 configuration, accumulation and execution contracts |

## Milestone exit gate

- OffscreenApplication keeps production initialization and adds an explicit draw overload selecting the corresponding execution implementation.
- A draw context/device/surface and non-template renderer delegate render headlessly with actual scene updates/samples.
- The requested frame count produces an accumulated scene image, not just N warm-up calls whose last frame is mislabeled as an average.
- CLI dimensions/internal scale/settings precedence, output conversion/orientation and completion are specified and validated.
- No-RT/default resampling output works, and optional effect/FSR availability is accurately exposed.
- M05 review revises M06–M10, especially headless configuration and standalone bake lifecycle reuse.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
