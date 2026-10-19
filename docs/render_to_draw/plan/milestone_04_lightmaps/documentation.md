# M04 cumulative documentation coverage

[Plan](../index.md) · [Progress](../progress.md) · [Technical review](review.md) ·
[Documentation step](step_08_documentation.md)

Delivery date: 2026-10-18 (Europe/Madrid). Starting HEAD:
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`. The checkout was clean at the beginning
of this documentation turn. Existing M04 documentation, including the full ordered
tutorial, was already present at this revision and was inspected and retained.
This delivery adds missing conceptual/raw-executor coverage and reconciles current
pages; it does not claim to have authored the existing tutorial in this turn.
No source, shader or CMake modifications, commits, resets or worktree replacement.
M04-07 is accepted by project lead continuation instruction; backend/scenario
coverage remains unspecified. Documentation closure awaits lead acceptance.

## Maintained page inventory and source mapping

Paths in the following tables are relative to `doc/` unless stated otherwise.
Updated means changed in this delivery; retained means inspected within this scope
and kept as the current canonical guidance. No page was retired: production
render remains supported through M08. No parallel versioned API snapshot is added.

| Page | Disposition | Actual source/caller and coverage |
|---|---|---|
| api/bake/index.md | Updated | bake/all.hpp, BakerContext.hpp, LightmapBaker.hpp; navigation includes raw executor and integrated architecture. |
| api/bake/LightmapSettings.md | Retained | bake/LightmapSettings.hpp/.cpp; eleven live accessor properties, type ID, library versus example defaults, paired validation and frozen jobs. |
| api/bake/IntegratedBakerContext.md | Updated | bake/IntegratedBakerContext.hpp/.cpp and example sample(); adds exact synchronous one-slot completion recipe and failure retirement. Existing generations/cancel/reset/publication rules retained. |
| api/bake/LightmapBakeExecutor.md | Added | bake/LightmapBakeExecutor.hpp/.cpp; all public methods, raw BakeSample ownership, shared draw effects, matching tracing capacities, required profiles and exclusive injection. |
| api/bake/UvSurfacePass.md | Retained | bake/UvSurfaceMesh, UvSurfacePass, Uv2Reload and app/DrawUv2SafeReload.cpp; explicit UV2/world surface formats, connectivity charts, stopped staged replacement and consumer invalidation. |
| api/bake/LightmapOutput.md | Retained | bake/UvTemporalAccumulator, LightmapCompositionPass and IntegratedBakerContext; raw sums/integer counts, composition/reference division, exact nearest-original-seed dilation, typed RGB and synchronous final validation. |
| api/bake/UvMapPreviewRenderer.md | Retained | bake/UvMapPreviewRenderer.hpp/.cpp; CPU read-only UV diagnostic, RGBA8 upload, top-row convention and resolution/image owner replacement. |
| api/bake/BakeLightmapPreview.md | Retained | ui/BakeLightmapPreview.hpp/.cpp; draw UI session registration, retained image/sampler, setResult/setImage/rebind/clear ownership. |
| api/draw/BakedIndirectMaps.md | Retained | base/MaterialAttributes, draw/pipeline material packing, GBuffer and paired raster/deferred shaders; explicit scalar/bakedAO/bakedGI, 32 attachment bytes, stored/live metadata and HDR display limits. |
| api/draw/RendererDeferred.md | Retained | draw/RendererDeferred and DefaultRenderLoopDelegate; current bake/material links and production coexistence are accurate. |
| api/gpu/ShaderLibraries.md | Retained | lib/CMakeLists.txt and cmake/utils.cmake; distinct draw/bake trees, AIR/targets, aggregate dependency, incremental bundle copy and RuntimeShaders install. |
| api/ui/Material_Editors.md | Updated | example setApplied(), MaterialAttributes and draw editor path; clarify stored/live adoption, stopped invalidation and unbaked originals without inventing editor baking/export. |
| api/ui/UvMapPreview.md | Retained | ui/UvMapPreview versus bake/UvMapPreviewRenderer and ui/BakeLightmapPreview; production/draw preview boundary is explicit. |
| api/ui/reference.md | Retained | draw preview navigation/ownership already present; native UI types remain implementation-only. |
| api/app/Uv2SafeReload.md | Retained | app/Uv2SafeReload.hpp and DrawUv2SafeReload.cpp; actual regenerateDraw weak/generation/identity guards, token and callbacks. |
| api/app/OffscreenApplication.md | Updated | app/OffscreenApplication production API; explicitly disambiguate production Vulkan configuration from unavailable draw offscreen initialization. |
| api/db/LightmapOutputWriter.md | Updated | db/LightmapOutputWriter production API; distinguish actual production batch/model-copy writer from new bake typed pixels. |
| apps/draw_lightmaps.md | Retained | examples/draw/12_lightmaps/src/main.cpp and target registration; actual controls/defaults, serial completion, resource correction and no file/CLI claims. |
| apps/index.md | Retained | Production applications and experimental example are separately linked. |
| tutorials/draw/12_lightmaps.md | Retained and checked | All 17 consecutive C++ blocks assemble to the shipped main.cpp modulo whitespace. Ordered scene/settings/UV2/bake/adoption/preview/cleanup construction, actual controls and failure guidance remain canonical. |
| tutorials/draw/index.md | Retained | Tutorial 12 entry points at the delivered example and correct prerequisites. |
| architecture/draw_integrated_lightmaps.md | Added | Current bake/draw public passes and example; UV2/surface/snapshot flow, exact completion, raw/result/CPU/display precision, material interpretation and extension boundaries. |
| architecture/index.md | Updated | Link architecture directly to the new conceptual guide; tutorial remains linked there. |
| architecture/lightmaps/index.md | Updated | Preserve production diagram/order/CLI; explicitly direct draw users to its actual raw-accumulate-then-compose architecture and material rules. |
| lightmap_baking.md | Updated | Preserve production editor/standalone workflows; add actual integrated draw ordering and explicit slot/metadata/export boundary. |
| safe_update_scene.md | Retained | Current draw bake section already requires retiring/discarding before mutation and refetching consumers; no production safe-update migration claimed. |
| deferred_render_model.md | Retained | Explicitly production-focused with current integrated bake/tutorial links. Legacy G-buffer descriptions do not describe the draw formats. |
| ../scripts/PACKAGING.md | Updated | Existing package/standalone workflow plus actual paired native draw/bake install/resources and recovery from older missing registration. No scripts executed. |

## Retained cumulative foundations and production coverage

The prior [M03 coverage ledger](../milestone_03_ray_tracing/documentation.md)
remains the cumulative M01–M03 baseline. Settings/catalog/codec, renderer and
upscaler replacement, shared AO/GI/reflection signals, histories, denoise, SMAA,
manipulation, deep clone and native primitive guides are not replaced by bake.
M04-specific cross-links and boundaries above were checked; no claim of a new
repository-wide source audit is made by retaining that earlier ledger.

The nine production architecture pages under architecture/lightmaps/ remain
production guidance: index is updated, while uv_atlas.md, uv_surface_projection.md,
lighting_passes.md, accumulation.md, contexts.md, standalone_scene_and_batch.md,
preview_and_editors.md and output_and_materials.md retain their render/CLI scope.
The geo atlas/validator, production render bake API, production application/CLI,
DB format and UI preview references remain necessary. Their public names are not
aliases for experimental bake services. The app reload reference already covers
both actual overloads and was checked against its header. Existing production
shader instructions remain valid alongside the paired trees.

No rename/removal is required for accurate math/json/input/window/workspace or
other foundation pages in this documentation-only step. Repository-wide final
promotion/retirement and complete retained-page reconciliation belong to M09;
M10 audits that accumulated result. This is an affected-page inventory, not a
claim that all maintained pages were reauthored or verified on hardware.

## Static evidence and limits

- Verified 505 local links/anchors and balanced fences across 34 affected pages: bake,
  draw/material, UI, app/DB, tutorial, application, conceptual, deployment and
  handoff pages. Links inside code fences are excluded from Markdown link parsing.
- The tutorial's 17 C++ blocks concatenate to the current example's exact
  non-whitespace contents. The added synchronous excerpt uses the same
  immediateSubmit(CommandBuffer*) callback, prepare/record/complete ordering and
  member owners. It is explicitly an excerpt, not a separate compilable fixture.
- Checked public executor declarations/implementation, selected effect settings,
  raw formats, environment ownership, empty-GI rejection and exclusive-pass
  cleanup. Compared the data-flow guide with context/output APIs and actual
  example beginBake/sample/releaseBake/setApplied call sites.
- Checked deployment claims against both paired compiler invocations, distinct
  targets/AIR paths, shared aggregate, draw/bake copies and RuntimeShaders install.
- `git diff --check` passes. Only Markdown changes; source license/style rules
  introduce no newly modified C/C++/shader files to inspect.

No compilation, configuration, native shader compilation, C++ fixture,
application/GPU run, package/install run, image encoding or file export was
performed. Tutorial equality and links establish source consistency, not runtime
success. Lead continuation accepts M04-07; it provides no new numerical,
performance, SDK or backend-specific artifacts. Lead verification remains the
Vulkan/Metal/no-RT and lifecycle/display/format checklist in the
[example guide](../../../../examples/draw/12_lightmaps/README.md).

## Remaining ownership and transition

| Owner | Remaining implemented-product documentation obligation |
|---|---|
| M05-01–07 | Windowless draw lifecycle/configuration, exact N camera samples, settings precedence and encoded image output; preserve UV/sample versus camera/image-average distinction. |
| M06 / M07 | Complete draw IO and GPU-only image materialization, editor/import/dirty-state/bake integration and clone/round-trip instructions. Update the same material/preview pages. |
| M08 | Actual standalone/assembly/batch lifecycle, protected-input/preflight/overwrite rules and byte/float output files. Forward declarations are not documented as services. |
| M09 | Canonical draw production names/defaults; remove retired render/app/shader recipes and reconcile all retained pages and package/standalone resources. |
| M10 | Final symbol/link/snippet/product/coverage audit of previously authored documentation; no deferred M04 substantive authoring. |

M04-08 is the final ordered M04 step, delivered `awaiting_project_lead`.
Continuation accepts this documentation closure and selects
[M05-01](../milestone_05_offscreen/step_01_execution-selection.md). Its prerequisite
now explicitly includes documentation acceptance. No M05 code is implemented or
authorized by this delivery. No unresolved documentation architecture decision
blocks that transition; recorded numeric/performance/export limitations remain.
