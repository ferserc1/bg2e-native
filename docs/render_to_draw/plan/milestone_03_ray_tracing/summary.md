# M03 — Complete advanced ray tracing, histories and denoise

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M01/M02 products and reviews. Advanced RT design choices are not approved by the earlier query-shadow implementation.

## Working product

The existing `examples/draw/deferred_renderer` and `examples/draw/manipulation` expose supported RTAO/RTGI, reflections and their temporal/denoise controls through live reflected settings. No new example is created. Vulkan and Metal use explicit equivalent backend shader implementations; no-RT raster behavior remains valid.

## Scope exclusions

- No integrated/standalone baking or offscreen application product yet; expose reusable inputs for M04 without implementing bake orchestration.
- No new example target, renderer family, shader translation system, forward renderer or MSAA.
- No forced native RT pipeline emulation contract for Metal; the existing compute/intersection model is reviewed before selection.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M03-01](step_01_rt-design-checkpoint.md) | Deferred decision checkpoint for advanced RT backends |
| [M03-02](step_02_rt-gpu-data.md) | GPU tracing stages and shader data access selected by checkpoint |
| [M03-03](step_03_blue-noise-utils.md) | Generic BlueNoise data and GPU resource in utils |
| [M03-04](step_04_rtao.md) | Public RTAO implementation and fallback |
| [M03-05](step_05_rtgi.md) | Public RTGI and equivalent backend kernels |
| [M03-06](step_06_rt-reflections.md) | Public traced reflections and resolution semantics |
| [M03-07](step_07_temporal.md) | Rendered-view histories and temporal accumulation |
| [M03-08](step_08_denoise.md) | Public scalar/HDR denoise and reusable surface inputs |
| [M03-09](step_09_existing-example-integration.md) | Integrate effects and verify existing examples only |
| [M03-10](step_10_review.md) | Review M03 backend contracts and prepare bake/headless reuse |

## Milestone exit gate

- Backend tracing design is recorded against current GPU docs and actual query/resource table code before implementation.
- AO/GI/reflections support the current production quality/settings/material-cutout behavior with appropriate neutral fallbacks.
- Public effect injection, separate scalar/HDR histories and denoise operate with correct rendered-scene/layer/view sequences.
- Both existing examples run with effect toggles/edits/resize/pause; disabling RT does not allocate/use unsupported tracing state.
- The existing SMAA spatial-AA facility is preserved as an optional public final-processing component; it is not MSAA and does not change the default scaling path.
- M03 review updates M04–M10, especially shared pass inputs, bake reuse, headless accumulation and settings serialization.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
