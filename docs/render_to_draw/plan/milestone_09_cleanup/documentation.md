# M09-08 — Cumulative production documentation

[Review](review.md) · [Progress](../progress.md) · [Documentation entry](../../../../doc/index.md)

Delivered 2026-10-21, HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50`
plus inherited uncommitted work. Lead continuation accepts M09-07; this delivery
awaits acceptance. No implementation, CMake, shader or product-script edits were
made. All 1,327 step-entry implementation/profiling files retain their hashes.

## Page dispositions and navigation

[documentation-dispositions.tsv](documentation-dispositions.tsv) reconciles all
232 original maintained pages with the delivery: 83 updated, 137 retained,
12 retired and nine added; 229 pages remain. Every row records old/new hash,
canonical destination, source anchors and verification scope. The original
M09-07 documentation-handoff.tsv is preserved as historical input.

Retired doc/api/render pages are removed and incoming maintained links now target
draw/bake owners. No former rendering API usage recipe is kept as a compatibility
promise. The three obsolete conceptual guides are rewritten around current
contracts. Mixed old lightmap architecture pages now route concepts to detailed
canonical bake references instead of duplicating removed native signatures.
Foundation geometry/JSON/reflection/input/window/workspace documentation remains
available; affected examples, links and resource/UI boundaries were reconciled.

New entrypoints doc/index.md and doc/api/index.md connect API, tutorials, apps,
concepts, migration and deployment. New references document Renderer injection,
Upscaler lifecycle and BakerContext ownership. Build/distribution, production
performance and migration pages provide final cross-module guidance. The product
catalog maps all 33 registrations to actual source directories and guide pages.

## Cumulative coverage and source mapping

Paths in this table are relative to the repository root.

| Coverage | Maintained documentation | Source / executable anchor |
|---|---|---|
| Production boundaries and breaking changes | doc/migration_to_draw.md; doc/api/index.md; draw/app/GPU indexes | lib/include/bg2e/all.hpp; draw/Renderer.hpp; app/MainLoop.hpp |
| Renderer/component lifecycle and custom injection | doc/api/draw/Renderer.md; RendererDeferred.md; tutorial 10 | draw/Renderer.hpp, DefaultRenderLoopDelegate.hpp, RendererDeferred.hpp; examples/draw/10_deferred_renderer |
| Live settings, exact typed codec and precedence | doc/api/draw/Settings.md; reflection guides; offscreen tutorial | draw/SettingsCatalog.hpp, SettingsCodec.hpp, RenderSettingsPreferences.hpp; UI GraphicsSettingsWindow |
| Upscaler extension and unsupported SDK/no-RT behavior | doc/api/draw/Upscaler.md; Settings.md; advanced-lighting architecture | draw/Upscaler.hpp, FsrUpscaler.hpp; pipeline effect headers; explicit native shader contracts |
| Clone, material aliases and safe edits | doc/api/draw/Resources.md; doc/safe_update_scene.md; manipulation tutorial | draw/Drawable.hpp; manipulation/SelectionManager.hpp; canonical DrawableComponent |
| Scene, factory, parsing/realization and complete saves | scene/DB indexes; DrawResourceIO.md; SceneReferences.md; model/editor tutorials | scene/ComponentFactoryRegistry.hpp; db/scene.hpp and draw resource IO; apps/draw/model_edit and bg2e_composer |
| AO/GI/reflections, histories, denoise and bindings | draw effect references; doc/deferred_render_model.md; architecture/draw_advanced_lighting.md | draw/pipeline headers and shaders/src/draw; no shader abstraction invented |
| Integrated UV samples, signals, chart/coverage and adoption | doc/lightmap_baking.md; bake/index.md; LightmapOutput.md; tutorial 12 | bake/BakerContext.hpp, IntegratedBakerContext.hpp, UvSurfacePass.hpp; examples/draw/12_lightmaps |
| Standalone job/scene/batch/output lifetime | bake standalone references; tutorial standalone_lightmaps; architecture/draw_standalone_lightmaps.md | StandaloneBakerContext, StandaloneLightmapBaker, StandaloneBakeSceneAssembler, StandaloneBakeBatch, LightmapOutputWriter |
| Exact camera accumulation and PNG output | tutorial 13; apps/draw_offscreen_application.md; offscreen references | app/OffscreenApplication.hpp; DrawOffscreenApplicationDelegate.hpp; draw/DefaultOffscreenApplicationDelegate.hpp; examples/draw/13_offscreen_application |
| UI context, queued edits, previews and native GUI seams | UI index/reference/quick start; UserInterface.md; GPU bridge guides | ui/UserInterface.hpp, DrawTextureWidgets.hpp, DrawMaterialEditor.hpp; gpu/GuiRenderer.hpp, StandaloneGuiWindow.hpp |
| Applications, CLI preflight, settings/output/errors | doc/apps/index.md; draw_model_edit, draw_composer, draw_lightmap_generator and import-service guides | Three canonical apps/draw products; unchanged preference/import-service identities |
| All supported examples and probes | doc/apps/products.md; numbered draw/GPU tutorials | final-products.tsv: 3 apps, 13 draw examples, 13 GPU examples, 4 executable probes; manual fixtures separate |
| Platform build, paired resources, SDK/package/standalone | doc/architecture/build_and_distribution.md; GPU ShaderLibraries.md; app deployment sections | scripts/PACKAGING.md and creator/updater scripts; paired draw/bake registrations and runtime resources |
| R01–R14 preservation and profiling | doc/architecture/production_performance.md; cpu_profiling.md; tracing/temporal/transparent guides; Composer Performance | Current revisions, arrays, exact completion, acceleration reuse, pools/leases, lazy histories, fallbacks, typed changes, CPU/GPU instrumentation |

All listed functionality is already implemented. Default resolution remains
disabled/1.0; optional effect requests and available runtime support are separate.
Native Metal upscalers, DLSS, forward/MSAA and old binary/API compatibility are
not advertised. Standalone baking and camera export retain distinct sample,
color, output and lifetime contracts. No bulk authoring is assigned to M10.

## Static verification and limits

Reproducible check:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_documentation.py --output-dir /tmp/bg2e-doc-audit
```

[documentation-audit.json](documentation-audit.json) records 229 pages, 1,870
local links/anchors, 111 project-include occurrences and zero errors. Two explicit
illustrative headers in reflection recipes describe user-defined MyComponent,
not missing engine headers. Literal source paths were additionally checked;
retired renderer headers/names/forward recipes and old app/example paths are
absent from maintained usage guidance. The scene index names removed scene::vk
bindings only to explain the breaking change.

Snippets/includes, declarations, source-backed tutorial call sites, current
catalog targets and native boundary signatures were inspected statically. This
is not a C++ parser, a compilation claim or an exhaustive behavioral proof for
all tutorial fragments. The audit excludes fenced examples from Markdown-link
parsing, does not fetch external URLs and does not validate rendered diagrams.
No configure/build/C++ execution, shader compilation, package install/launch,
standalone creator execution, binary inspection, output round-trip or performance
measurement was performed. Lead acceptance of M09-07 has unspecified backend and
scenario detail; documentation does not manufacture that missing evidence.

## Transition

M09-08 is awaiting project lead acceptance. Continuation accepts this delivery
and selects M10-01 documentation inventory audit. M10 audits the accumulated
coverage and makes localized corrections, preserving step IDs and the honest
native/performance limitations above. The existing M09-07 review remains history;
its documentation handoff is appended separately. No new engine feature or build
system change is required by this documentation delivery.
