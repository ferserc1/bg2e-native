# M04 — Integrated lightmap generation in bake

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M03 backend/effect/surface/history review and prior scene/material safe-edit contracts.

## Working product

A runnable `examples/draw/lightmaps`, derived from deferred_renderer, generates UV set 2 and bakes maps for suitable scene objects, applies their metadata/textures and renders with live indirect GI/AO disabled. Public baking resources live in bg2e::bake; the atlas modifier remains in geo.

## Scope exclusions

- No standalone batch application or headless application yet; StandaloneBakerContext/Batch implementation is completed in M08.
- No move of GenerateUv2AtlasModifier out of geo, no ResourceManager, no default mesh instancing.
- Do not duplicate private AO/GI/denoise implementations inside bake; reuse compatible public draw::pipeline objects.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M04-01](step_01_bake-contracts.md) | Bake namespace and public configuration/pixel contracts |
| [M04-02](step_02_uv2-surface.md) | Safe UV2 mesh rebuild and depthless UV surface primitives |
| [M04-03](step_03_integrated-context.md) | Integrated context and shared AO/GI bake execution |
| [M04-04](step_04_accumulation-output.md) | Bake accumulation, composition, dilation and typed pixels |
| [M04-05](step_05_uv-preview.md) | Bake UV map preview and UI integration |
| [M04-06](step_06_working-example.md) | Build examples/draw/lightmaps with live GI disabled |
| [M04-07](step_07_review.md) | Review M04 reusable maps and revise exports/apps/standalone |

## Milestone exit gate

- UV2 generation safely rebuilds target CPU/GPU geometry and tracing state, preserving clone independence.
- Integrated bake preparation, target/frame ownership, progress/reset/cancel and scene-generation checks work on supported backends.
- AO/GI accumulation, valid-mask handling, RGB multiplier composition/dilation, typed pixel output and applied UV2 metadata are correct.
- UvMapPreviewRenderer and LightmapSettings/LightmapPixels are in bake; no new bake implementation lives in render.
- The example disables live indirect passes while preserving IBL/direct/emission and consuming actual baked textures.
- M04 review revises offscreen/export/editor/standalone steps using actual bake contracts and verification.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
