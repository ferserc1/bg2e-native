# M08-07 — Cumulative documentation delivery

[Technical review](review.md) · [Documentation policy](../documentation-policy.md) ·
[Progress](../progress.md) · [Source/evidence record](product-validation.md)

## Baseline and scope

Date: 2026-10-21. Starting/finishing HEAD remains
78943d5820e69363c942d71136f479a0df87128e, with inherited uncommitted review,
implementation and earlier documentation retained. M08-06 is accepted by project
lead continuation instruction; compilation/checks are assumed successful under
the execution protocol, backend/scenario detail not specified.

This step updates maintained doc/ through delivered M08, not just historical plan
handoffs. It adds **9 maintained pages** and updates **22** (31 affected pages),
with no retired pages, source/CMake/shader/script behavior change or production
API removal. Production render/CLI guidance remains explicitly scoped alongside
experimental draw/bake. M09 still owns canonical promotion/cleanup and final
breaking-name reconciliation; M10 audits already-authored coverage.

## New maintained coverage and source mapping

| New page | Actual source/caller and delivered content |
|---|---|
| [StandaloneBakerContext](../../../../doc/api/bake/StandaloneBakerContext.md) | bake public header + StandaloneBakerContext.cpp: borrowed original offscreen Engine/Device, states, preparation/source generations, exclusive public pass injection and immutable/completed reuse. |
| [StandaloneLightmapBaker](../../../../doc/api/bake/StandaloneLightmapBaker.md) | Header and shared implementation: exact internal synchronous samples, target/settings/generation states, result/readPixels, reset/cancel/cleanup and one-live-handle lifetime. |
| [StandaloneBakeSceneAssembler](../../../../doc/api/bake/StandaloneBakeSceneAssembler.md) | Header/cpp + shared scene/CPU parsers: BG2/VWGLB/OBJ/context/prefab roots, enabled stable identities, staged independent UV2, deferred/custom hooks and retained Scene/root ownership. |
| [StandaloneBakeBatch](../../../../doc/api/bake/StandaloneBakeBatch.md) | Header/cpp and main caller: actual Options/Progress/Result, copied settings, serial target release, preflight, CPU callbacks/current validity and complete success/cancellation. |
| [Bake LightmapOutputWriter](../../../../doc/api/bake/LightmapOutputWriter.md) | Header/cpp + native DB snapshot: protected inputs/OBJ/MTLs/custom declarations, CPU/uploaded geometry, typed maps/metadata, real independent model resources and final authoritative manifest/failure boundaries. |
| [DB FileIO](../../../../doc/api/db/FileIO.md) | db/file + texture_archive headers/cpp and ObjMesh inventory: OS-enforced no-replace/explicit replacement, native CPU/GPU archive contracts, little-endian/explicit mip limits and separation from bake policy. |
| [Standalone tutorial](../../../../doc/tutorials/draw/standalone_lightmaps.md) | Actual assembler/batch/main/context calls: explicit bake include, caller-owned Engine, retained Scene, UV2 before realization, complete protected model batch and direct serial sample recipe. |
| [Draw lightmap generator](../../../../doc/apps/draw_lightmap_generator.md) | Actual app parser/main/CMake and shipped config: all 25 parser options/help, backend, live codec/CLI-last, counts/formats/UV2/copies, status/signals, manifest, package/standalone requirements and validation scope. |
| [Standalone ownership architecture](../../../../doc/architecture/draw_standalone_lightmaps.md) | Context/assembler/batch/writer + db/draw_resource_io: diagram, lifetime/source/completion separation, chart/count/material/file boundaries, current native reuse/profiling and independent evidence limits. |

All five actual standalone types are documented by name; tentative SceneAssembler
or Batch aliases are not taught. `<bg2e/bake/all.hpp>` is explicit because current
root all.hpp does not include it and still exposes coexistence branches. No
future canonical accessor, ResourceManager, additional illumination texture slot,
parallel job or unsupported renderer feature is presented as available.

## Updated pages and dispositions

| Updated maintained pages | Reconciliation |
|---|---|
| [Bake index](../../../../doc/api/bake/index.md), [draw index](../../../../doc/api/draw/index.md) | Removed standalone-not-implemented claims and connected actual standalone references/tutorial/app/architecture without changing integrated scope. |
| [LightmapOutput](../../../../doc/api/bake/LightmapOutput.md), [BakedIndirectMaps](../../../../doc/api/draw/BakedIndirectMaps.md) | Connected persisted resources while preserving raw GI half-precision rejection, final float32 multiplier/file encoding and independent interactive half-range bounds. |
| [DB index](../../../../doc/api/db/index.md), [production DB writer](../../../../doc/api/db/LightmapOutputWriter.md) | Two writers are distinct, not namespace aliases; old per-target byte/failure behavior remains production reference, new bake writer owns protected generation publication. |
| [Complete draw IO](../../../../doc/api/db/DrawResourceIO.md) | Corrected current bool-noexcept saves, explicit false checks, parent-directory creation and hardlink-unavailable exclusive reserve/copy fallback; retained native full-resource snapshots and separate batch transaction. |
| [App index](../../../../doc/api/app/index.md), [LightmapGenerator](../../../../doc/api/app/LightmapGenerator.md), [OffscreenApplication](../../../../doc/api/app/OffscreenApplication.md) | Separate production/new tools and no-camera UV finish path from exact-N camera delegates; preserve native owner cleanup and existing production overload. |
| [ShaderLibraries](../../../../doc/api/gpu/ShaderLibraries.md) | Connected actual standalone paired-tree/runtime deployment and precompiled-library needs; no source-registration-as-package-success claim. |
| [Apps index](../../../../doc/apps/index.md), [integrated app](../../../../doc/apps/draw_lightmaps.md) | Maintained standalone guide discoverable; integrated material adoption is not a complete file batch. |
| [Architecture index](../../../../doc/architecture/index.md), [MainLoop architecture](../../../../doc/architecture/MainLoop_render_draw_gpu.md), [integrated bake architecture](../../../../doc/architecture/draw_integrated_lightmaps.md) | Public standalone owners and app lifecycle linked alongside existing strategy/completion models; no new graphics strategy or global promotion claimed. |
| [Production lightmap family index](../../../../doc/architecture/lightmaps/index.md), [production output/material page](../../../../doc/architecture/lightmaps/output_and_materials.md), [lightmap overview](../../../../doc/lightmap_baking.md), [production settings](../../../../doc/api/render/LightmapSettings.md) | Preserved accurate production recipes/limits, distinguished delivered draw explicit RGB AO-slot interpretation and native HDR output, removed implied future dedicated-slot promise. |
| [Safe updates](../../../../doc/safe_update_scene.md), [draw tutorial index](../../../../doc/tutorials/draw/index.md) | Stopped standalone source freezing/invalidation and complete publication versus partial interactive adoption; usable tutorial entry. |

No maintained production page is deleted or relabelled canonical draw. The existing
[production CLI guide](../../../../doc/apps/lightmap_generator/index.md) and
[app source README](../../../../apps/draw/lightmap_generator/README.md) remain valid
coexistence/useful source guides; maintained doc/ now supplies its own full
reference instead of relying only on a README or a migration report.

## Retained cumulative coverage

M01–M07 coverage records remain the baseline:
[M01](../milestone_01_deferred_renderer/documentation.md),
[M02](../milestone_02_manipulation/documentation.md),
[M03](../milestone_03_ray_tracing/documentation.md),
[M04](../milestone_04_lightmaps/documentation.md),
[M05](../milestone_05_offscreen/documentation.md),
[M06](../milestone_06_model_edit/documentation.md),
[M07](../milestone_07_bg2e_composer/documentation.md).

Retained foundation/source mappings checked where affected:

- [Integrated context](../../../../doc/api/bake/IntegratedBakerContext.md),
  [executor](../../../../doc/api/bake/LightmapBakeExecutor.md),
  [settings](../../../../doc/api/bake/LightmapSettings.md),
  [UV surface/reload](../../../../doc/api/bake/UvSurfacePass.md),
  [UV diagnostic](../../../../doc/api/bake/UvMapPreviewRenderer.md) and
  [UI bridge](../../../../doc/api/bake/BakeLightmapPreview.md): current public
  M04 headers/implementations and integrated example remain their source. M08
  reuses, rather than changes, raw samples, counts, masks, charts and injection.
- [Scene API](../../../../doc/api/scene/index.md) and
  [reference IO](../../../../doc/api/db/SceneReferences.md): actual Node/Scene,
  component factory/schema hooks, detached roots, clone independence, parsed CPU
  data versus stopped draw resource loading still apply to assembly. Unknown
  schema skip is distinct from retained custom dependency requirements.
- [Engine](../../../../doc/api/draw/Engine.md),
  [default offscreen delegate](../../../../doc/api/draw/DefaultOffscreenApplicationDelegate.md),
  [camera tutorial](../../../../doc/tutorials/draw/13_offscreen_application.md):
  actual Engine/offscreen driver/default delegate retain their camera sample
  contract; the new tutorial does not redefine those samples as UV updates.
- [TracingSceneResources](../../../../doc/api/draw/TracingSceneResources.md),
  [update synchronization](../../../../doc/api/draw/UpdateSynchronization.md),
  [profiling](../../../../doc/architecture/cpu_profiling.md) and current GPU/effect
  references retain exact v3/v5 capacity/residency, recording completion, immutable
  acceleration/pool reuse, lazy invalidation, conservative regions and narrow
  final-color updates. No namespace cleanup or workload/timing wait conflation.
- Existing model editor/Composer, Workspace, input/window/delegate/geometry/
  reflection and GPU tutorials retain their scopes and real earlier source.
  Standalone delivery introduces no new public change in those foundations.
  MainLoop joined workers/custom import/source ownership and corrected bool
  SelectableList coverage remain in the accepted M07 baseline.
- Existing scripts/PACKAGING, create/update-library scripts and shared bundle/
  paired shader/install rules remain source authorities. This step reads them,
  documents VULKAN_SDK/platform/paired-tree/default-template constraints and does
  not modify CMake/scripts or claim native distribution execution.

The repository-wide final source/page reconciliation still belongs to M09-08/M10.
There is no known substantive standalone authoring gap deferred from M08 to M10.

## Static verification and evidence ownership

- Checked 31 affected maintained pages: **409** local link targets/anchors, code
  fences, project include paths and embedded JSON. C++ lambda syntax inside code
  fences is excluded from Markdown-link parsing. Existing deliberate Markdown
  hard-break spaces are retained; git diff --check passes.
- Compared all **25** parser option names with the maintained CLI guide, plus
  standalone help/exit behavior, exact schema/type/property identifiers and the
  shipped typed config example. API signature/default/member tables match current
  public headers, and examples follow actual app/context/batch call ordering.
- Tutorial snippets use explicit delivered headers and caller-supplied live owners;
  atlas options target the final resolution, source settings precede preparation,
  one live baker releases before another and CPU result ownership is separate
  from GPU lifetime. Signatures/examples are statically checked, **not compiled**.
- Source comparison confirmed complete DB saves return bool noexcept, create
  parents and retain exclusive reserve/copy fallback. The updated recipe checks
  false rather than expecting an exception to report save failure.
- Stale global availability/old-save-contract searches and review-to-coverage
  checks passed. No new engine feature, source/shader/style change or native
  execution was needed for documentation.

Earlier M08-05 ten synthetic inspector tests remain their original CPU evidence;
they are not rerun or upgraded here. M08-01–06 continuation acceptance does not
supply named Vulkan/Metal, actual geometry reload, chart/lighting numerics,
cancellation/fault, installed-package or measured optimized-counter results.
Those limitations and required lead checks remain visible in product-validation.md,
source guides and the maintained app/architecture pages. No configure/build,
shader compilation, application/native probe, package operation or GPU test was
performed by this documentation step.

## Review and transition handoff

The M08 technical review's maintained standalone documentation gap is now addressed
by the pages above. Its original review history is preserved; a closure note is
appended to review.md. M08-07 awaits lead acceptance. The linked next step is
M09-01: refresh the candidate inventory and actual incoming references before
retirement; this delivery does not start deletion or promote draw globally.
M09-08 reconciles final names/data/source policies in maintained pages, and M10
audits this authored coverage rather than recreating it from historical plans.
