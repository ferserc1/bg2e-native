# M03 — Complete advanced ray tracing, histories and denoise

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M01/M02 products and reviews. Advanced RT design choices are not approved by the earlier query-shadow implementation.

## Working product

The existing `examples/draw/10_deferred_renderer` and `examples/draw/11_manipulation` expose supported RTAO/RTGI, reflections and their temporal/denoise controls through live reflected settings. No new example is created. Vulkan and Metal use explicit equivalent backend shader implementations; no-RT raster behavior remains valid.

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
| [M03-11](step_11_documentation.md) | Update maintained documentation cumulatively through M03 |

## Milestone exit gate

- The final [documentation step](step_11_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- Backend tracing design is recorded against current GPU docs and actual query/resource table code before implementation.
- AO/GI/reflections support the current production quality/settings/material-cutout behavior with appropriate neutral fallbacks.
- Public effect injection, separate scalar/HDR histories and denoise operate with correct rendered-scene/layer/view sequences.
- Both existing examples run with effect toggles/edits/resize/pause; disabling RT does not allocate/use unsupported tracing state.
- The existing SMAA spatial-AA facility is preserved as an optional public final-processing component; it is not MSAA and does not change the default scaling path.
- M03 review updates M04–M10, especially shared pass inputs, bake reuse, headless accumulation and settings serialization.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Reuse public M01 pipeline/environment/query primitives, canonical/dense identity mapping, live codec and optional SDK FSR. Do not promote the eight-pair alpha table to an arbitrary RT material ABI. The project lead owns M03-01 decisions; general arrays, object/effect histories, denoise and optional SMAA remain real work. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M03-11](step_11_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).
