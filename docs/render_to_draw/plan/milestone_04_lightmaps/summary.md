# M04 — Integrated lightmap generation in bake

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M03 backend/effect/surface/history review and prior scene/material safe-edit contracts.

M02 distinguishes partial CPU/live material edits from geometry reloads and exposes
weak typed selection identity plus application-owned queued invalidation. UV2 is
`texCoord1`/material selector 1; same-topology `Drawable::reload()` rebuilds GPU
geometry but does not itself promise a new selection ID or bake generation. M04
must own an explicit bake/source generation and refresh extracted render/bake
resources after UV/topology changes. See [M02 review](../milestone_02_manipulation/review.md).

## Working product

A runnable `examples/draw/12_lightmaps`, derived from deferred_renderer, generates UV set 2 and bakes maps for suitable scene objects, applies their metadata/textures and renders with live indirect GI/AO disabled. Public baking resources live in bg2e::bake; the atlas modifier remains in geo.

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
| [M04-06](step_06_working-example.md) | Build examples/draw/12_lightmaps with live GI disabled |
| [M04-07](step_07_review.md) | Review M04 reusable maps and revise exports/apps/standalone |
| [M04-08](step_08_documentation.md) | Update maintained documentation cumulatively through M04 |

## Milestone exit gate

- The final [documentation step](step_08_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- UV2 generation safely rebuilds target CPU/GPU geometry and tracing state, preserving clone independence.
- Integrated bake preparation, target/frame ownership, progress/reset/cancel and scene-generation checks work on supported backends.
- AO/GI accumulation, valid-mask handling, RGB multiplier composition/dilation, typed pixel output and applied UV2 metadata are correct.
- UvMapPreviewRenderer and LightmapSettings/LightmapPixels are in bake; no new bake implementation lives in render.
- The example disables live indirect passes while preserving IBL/direct/emission and consuming actual baked textures.
- M04 review revises offscreen/export/editor/standalone steps using actual bake contracts and verification.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Reuse existing depthless MRT/float/integer formats/raw completed color transfer and public shared environment snapshots. No bake namespace/context, UV surface profile or real baked RGB application was delivered by M01. Keep topology/UV edits, generation validity, typed LightmapPixels and effect reuse explicit. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M04-08](step_08_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).

## M03-01 tracing checkpoint dependency

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and bake infrastructure remain pending. After M03 implementation/review, reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

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


## M04-07 technical review delivery

[Review](review.md) reconciles the actual integrated example and public bake
contracts, intentional CPU-preview/six-slot-material refinements, HDR precision
limits and corrected paired bake deployment. Corrected M04-06 is accepted by
project lead continuation with backend/scenario coverage unspecified. The review
updates concrete pending M05–M10 requirements and leaves M04-08 cumulative
documentation as the next required step; it does not close M04 or start M05.


## M04-08 documentation delivery

The [coverage inventory](documentation.md) reconciles the canonical maintained
bake, material, preview, tutorial, application, architecture and deployment pages
through actual M04 implementation. M04-07 is accepted by project lead continuation.
Documentation is delivered awaiting acceptance; continuation closes this milestone
and selects M05-01. Source-consistent documentation does not add backend numeric,
performance or exported-file evidence. No M05 work is implemented here.
