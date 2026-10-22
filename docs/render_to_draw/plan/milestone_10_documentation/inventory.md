# M10-01 — Cumulative documentation inventory audit

[Progress](../progress.md) · [Policy](../documentation-policy.md) · [Next: API audit](step_02_api-reference.md)

## Baseline and result

Delivered 2026-10-22 at HEAD `e2a32164a781dbf420a4464b25ddce9feb862c6a`,
with a clean entry working tree. Lead continuation accepts M09-08 and closes M09;
backend/scenario coverage remains unspecified. This delivery audits documentation
and changes two localized writer descriptions. No engine, shader, CMake, product
script or optimization/profiling implementation changes were made.

The inventory accounts for all **229 maintained pages**, all **232 M09 review
handoff rows**, nine subsequently added pages and twelve removed pages. There
are no missing destinations, source-anchor paths or unowned maintained pages.
Every retained foundation page remains included. This is an inventory gate;
detailed signature, tutorial, build and consistency audits remain M10-02–05.

## Traceable artifacts

| Artifact | Meaning and scope |
|---|---|
| [page-inventory.tsv](page-inventory.tsv) | 241 rows: original page, M09 disposition/hash, current keep/update/remove decision, destination/hash, source anchors, documentation and audit owners. All 229 surviving destinations match the actual doc/ page set exactly. |
| [handoff-coverage.tsv](handoff-coverage.tsv) | 216 explicit historical page references across all nine documentation.md handoffs, resolved through M09 dispositions. This counts references, not unique pages; shorthand/grouped foundation coverage is recorded below and in the full page inventory. |
| [product-coverage.tsv](product-coverage.tsv) | All 33 current source registrations, source directories and existing guides. App target names are checked against APP_TARGET_NAME; examples/probes against their local CMake registrations. No configure or binary evidence. |
| [header-coverage.tsv](header-coverage.tsv) | All 317 current public .hpp files, hashes and maintained module discovery entries. This is module-level discovery coverage, not proof that each symbol has a complete reference. |
| [inventory-audit.json](inventory-audit.json) | Counts, revision, scope and zero path/ownership reconciliation errors. |
| [documentation-audit.json](documentation-audit.json) | Fresh maintained links/anchors/includes check following the localized corrections. |

Source anchors may name files or module directories. Existing M09 source anchors
are retained, with actual included headers and linked source/product/script paths
added where available. Pages whose old anchors only named historical reviews now
also have a current source-registration or public umbrella owner. The source
catalog and module discovery entries are inputs to M10-02, not semantic proofs.

## Distributed coverage reconciliation

Each report below remains a historical delivery record. Earlier production/draw
coexistence, interim names and pending-work statements do not override current
maintained pages or source. M09 supplies the cumulative authoring owner for every
page; earlier domain ownership is preserved here and in the explicit mapping.

| Handoff | Final maintained coverage / source ownership | Next audit |
|---|---|---|
| [M01](../milestone_01_deferred_renderer/documentation.md) | Draw runtime/renderer/resources/settings/environment, GPU primitives/native resources, DB references and tutorial 10; draw/gpu/db/app headers and current examples. | M10-02/03 |
| [M02](../milestone_02_manipulation/documentation.md) | Manipulation, material/viewpoint/camera utilities, UI previews/input and tutorial 11; manipulation/utils/UI headers and example. | M10-02/03 |
| [M03](../milestone_03_ray_tracing/documentation.md) | Effect/surface/tracing/motion/history/denoise references and advanced lighting/settings; draw pipeline headers and paired shaders. | M10-02/03/05 |
| [M04](../milestone_04_lightmaps/documentation.md) | Bake context/executor/UV/output/preview, integrated architecture/tutorial 12 and deployment. Its table paths are relative to doc/; they are included in the explicit mapping. | M10-02/03 |
| [M05](../milestone_05_offscreen/documentation.md) | Typed draw offscreen ownership, exact camera samples, PNG output, architecture/tutorial 13; app/draw offscreen headers and example. | M10-02/03/04 |
| [M06](../milestone_06_model_edit/documentation.md) | Model editor workflow, complete DB IO, settings/editor UI and clone/safe edits; current apps/model_edit and shared engine headers. | M10-02/04 |
| [M07](../milestone_07_bg2e_composer/documentation.md) | Composer/root/factory/custom clone, parsed DB/import workers/HTTP, scene/resource UI and bake workflow; current apps/bg2e_composer and shared APIs. | M10-02/04 |
| [M08](../milestone_08_lightmap_generator/documentation.md) | All 31 authored/updated pages remain resolved, including standalone context/baker/assembly/batch/writer, FileIO/archive, tutorial, CLI and architecture. Standalone scheduling is present in the bake index. | M10-02/03/04 |
| [M09](../milestone_09_cleanup/documentation.md) | Whole maintained set, including foundation math/geo/JSON/reflection/input/window/workspace pages, production boundaries, extensions, migration and deployment; final public headers and registered products. | M10-02–06 |

The distributed policy's API, lifecycle/settings/clone, capabilities/native binding,
custom renderer/upscaler, tutorials, apps/CLI, package/standalone and navigation
families all have maintained destinations and audit owners. Removed render API
pages resolve to canonical draw/bake guidance; their old paths are absent and
maintained incoming local links pass. Historical docs/render_to_draw links are
evidence links, not additional maintained API snapshots. No substantive missing
authoring requirement was identified at inventory depth.

Optimization coverage explicitly retains TemporaryResourcePool, TemporalInvalidation,
TransparentRegions, UpdateSynchronization and TracingSceneResources references,
plus architecture/cpu_profiling and production_performance. Source owners include
draw pipeline/cache/completion code, base CPU/GPU telemetry, GPU timing transport
and ui/Performance with Composer integration. The optimization and profiling
[reports](../../../optimizations/render_to_draw/summary.md) and
[profiling evidence](../../../profiling_and_stats/summary.md) retain their separate
native/performance validation state. R01–R14 are semantic checkpoints for M10-02/05,
not measured performance results established by this inventory.

Settings discovery includes SettingsCatalog/SettingsCodec/RenderSettingsPreferences,
bake/LightmapSettings and the actual standalone parser. The latter still binds
`bg2e.bake.standalone.v1` with target `bake`; app binary renaming does not rename
that settings identity. Exact schema, property and precedence audits remain M10-02/04.

## Findings and localized corrections

1. **Post-delivery source refinement:** HEAD moved apps/draw products to
   apps/model_edit, apps/bg2e_composer and apps/lightmap_generator, with targets
   model_edit, bg2e_composer and lightmap_generator. The current maintained product
   catalog already reflects this. Historical final-products.tsv remains unchanged;
   product-coverage.tsv records the actual current names. Build/package scripts
   also changed in HEAD and must be audited as current source in M10-04.
2. **Historical hash drift:** 21 maintained pages differed from M09 delivery hashes
   at entry, consistent with HEAD's app relocation changes. Both hashes are kept
   rather than reporting a false pristine M09 baseline. Two additional pages now
   differ because of this audit's correction: 23 updated rows in the final inventory.
3. **Localized stale availability:** bake/index.md and bake/LightmapOutputWriter.md
   still described a separate production DB writer as available. Current source
   has only the bake writer; the existing DB migration page already explained its
   removal. Corrected both descriptions and kept that migration link.
4. **Header inventory boundary:** 316 headers live under lib/include/bg2e/;
   the root lib/include/bg2e.hpp umbrella makes 317. Both the final source catalog
   and this inventory include that external umbrella; no header disappearance is
   inferred from counting the inner directory alone.

## Checks and limits

Reconciled page sets/unique dispositions/destinations/hashes, all explicit historical
page references and existing source anchors. Checked current product guides and
registrations, all public .hpp module discovery entries, preserved optimization
page families and settings source owners. Fresh static maintained-document check:
229 pages, 1,870 local links/anchors, 111 project includes, zero errors. Two
illustrative MyComponent includes retain the existing documented exception.

The maintained link/include check can be repeated from the repository root:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_documentation.py --output-dir /tmp/bg2e-m10-doc-audit
```

No compiler, CMake configure, shader compiler, native executable/test, packaging,
external URL fetch, output reload or profiling capture was run. Hash/path/module
checks do not establish complete per-symbol semantics, snippet compilation or
backend behavior. The lead reviews this inventory and localized corrections;
continuation accepts M10-01 and selects M10-02. No new executable is introduced.
