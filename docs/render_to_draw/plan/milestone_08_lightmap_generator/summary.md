# M08 — Standalone lightmap_generator on draw/bake

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M04–M07 bake/pixel/scene/headless application reviews and current CLI feature inventory.

## Working product

A runnable `apps/draw/lightmap_generator` uses windowless draw/GPU execution and public bake StandaloneBakerContext/SceneAssembler/Batch/output resources. It matches the current CLI map-generation, target validation, UV2, accumulation, output and overwrite behavior without a viewport renderer or production Engine.

## Scope exclusions

- No reuse of integrated window frame scheduling as a substitute for standalone context state.
- No move of UV2 generation from geo or duplicate rendering-effect algorithms inside the app.
- No retained production CLI dependency in the new executable; the original target remains reference until M09.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M08-01](step_01_standalone-context.md) | Public standalone bake context and scene generation |
| [M08-02](step_02_scene-assembly.md) | Bake model/prefab/context assembly and UV2 |
| [M08-03](step_03_batch-output.md) | Bake batch/output writer and protected complete exports |
| [M08-04](step_04_cli-application.md) | Build apps/draw/lightmap_generator and configuration flow |
| [M08-05](step_05_product-validation.md) | Standalone CLI parity, multiple targets and deployment |
| [M08-06](step_06_review.md) | Review M08 and prepare exhaustive production removal |

## Milestone exit gate

- Standalone preparation/generation/update/cleanup and target validation work through bake over headless draw/GPU resources.
- Model/prefab/context assembly, UV2 and scene/material/environment/light capture match actual existing behavior.
- Bake-specific batch/output orchestration lives in bake; generic file/image/scene IO remains in DB.
- Byte/float maps and optional model copies preserve metadata, dilation and protected-input/preflight behavior.
- CLI settings/configuration/backend/errors/output contracts are tested; unsupported required RT baking fails explicitly.
- M08 review updates the exhaustive removal gate and final documentation plan.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
