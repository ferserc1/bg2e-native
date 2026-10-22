# Shared implementation progress

[Plan index](index.md) · [Execution protocol](execution-protocol.md) · [Milestone review](review-protocol.md)

This is the execution record for all implementation chats. The **Next step to implement** field below identifies the step the next chat must open; the project lead does not need to supply its ID or filename. Each implementing agent must maintain that field according to the execution protocol. Do not infer accepted work from committed planning documents or pre-existing GPU/draw facilities.

## Current position

| Field | Value |
|---|---|
| Workflow state | `awaiting_project_lead` |
| Current milestone | M10 — Final audit of cumulative documentation |
| Active step | M10-06 — GPU 13 presentation crash correction |
| Last implemented step | M10-06 — Final review with requested source corrections |
| Last accepted step | M10-05 — accepted by project lead continuation instruction |
| Awaiting project lead validation | M10-06 — GPU 13 display correction and final acceptance |
| Next step to implement | None — final step delivered; [final review](milestone_10_documentation/review.md) awaits lead acceptance. |
| Next action | `wait_for_project_lead` |
| Condition before starting | No successor; requested source corrections delivered, final lead validation pending. |
| Record initialization baseline | `b955833f314d09a191de13b83c90cacc10666ae7`; current M09-01 entry HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50`, clean checkout. |
| Validation owner | Human user / project lead |
| Agent build/runtime execution | Not performed; delegated to the project lead by default. |

Latest delivery: [GPU 13 presentation correction](milestone_10_documentation/rt-display-correction.md).
Lead reported cross-format copy validation abort. RGBA storage is now sampled by
a fullscreen display pass into the actual surface format; shaders store normal
RGB, Metal closes its compute encoder and resize rebinds display resources.
Example/tutorial corrected; CMake and engine/backend APIs unchanged. Static checks
pass; no compilation/native execution. Rebuild of example and new shaders plus
lead native validation pending; no successor. Earlier notes are historical.

Latest delivery: [requested source corrections](milestone_10_documentation/source-corrections.md).
GPU 11/13 now wait for prior GPU work before shared UBO writes; GPU 13 counts only
successfully acquired frames. All four standalone preflights use canonical paired
bake shader paths. Maintained guides and final review describe current behavior;
both findings are removed from open issues. Static/isolated preflight checks pass.
No compilation, native runtime or full creator/updater execution. Source corrections
and final acceptance await the lead; no successor. Earlier notes are historical.

Latest delivery: [M10-06 final review](milestone_10_documentation/review.md).
M10-05 accepted by continuation. Reconciled 43 plan decisions, R01–R14, nine
historical handoffs, 229 pages, 317 headers and 33 products. Revalidated 350 source
paths; maintained audit passes with 1,913 links/anchors and 111 includes.
Final documentation audit delivered; no successor implementation milestone.
Final lead acceptance pending. F01/F02 remain explicit source requirements;
this review does not certify unconditional implementation-plan closure.
No build/runtime/package/performance execution. Older notes are historical.

Latest delivery: [M10-05 documentation validation](milestone_10_documentation/documentation-validation.md).
M10-04 accepted by continuation. All 229 pages reachable; corrected 16 pages,
30 exact excerpts match source, six diagrams reviewed statically. Checks pass:
1,913 links/anchors, 111 includes, 1,276 unchanged non-document files.
No compilation/runtime execution. Both source follow-ups remain unresolved.
M10-05 awaits lead validation; M10-06 final review is next. Older notes are history.

Latest delivery: [M10-04 apps/CLI/distribution audit](milestone_10_documentation/apps-distribution-audit.md).
M10-03 accepted by lead continuation. Reconciled 18 contract families/17 pages,
38 option rows and 292 derived shader outputs; corrected ten documents.
Fresh maintained check: 1,899 links/anchors, 111 includes, zero errors.
Standalone creator/updater preflight checks an obsolete flat bake path; fresh
canonical builds fail that check. The four scripts are unchanged. This new source
follow-up and prior shared-UBO risk remain open, without native reproduction.
M10-04 awaits lead review; M10-05 is next. Earlier delivery notes are history.

Latest delivery: [M10-03 tutorial audit](milestone_10_documentation/tutorials-audit.md).
M10-02 accepted by lead continuation. Audited 31 tutorial pages/26 examples;
426 numbered code blocks classified, 350 source-token matches and 76 reviewed
recipes. Corrected 14 pages, including current draw 06 environment/skybox usage.
Tutorials 12/13 reconstruct main.cpp ignoring whitespace. Fresh check: 1,891
links/anchors and 111 includes, zero errors; implementation files unchanged.
Primitive GPU 11/13 shared-UBO overlap risk is documented as an unrepaired source
follow-up. No build/runtime evidence. M10-03 awaits acceptance; M10-04 is next.

Latest delivery: [M10-02 API reference audit](milestone_10_documentation/api-reference-audit.md).
M10-01 accepted by lead continuation. Reviewed 37 contract families across 55
maintained pages and 115 source anchors; localized corrections in 18 pages.
Fresh check: 229 pages, 1,881 links/anchors and 111 includes, zero errors.
All 1,276 step-entry non-vendored implementation/build/script/test files unchanged.
No compilation/runtime/native/performance execution. M10-02 awaits acceptance;
M10-03 tutorial audit is next. Earlier delivery notes below are historical.

Latest delivery: [M10-01 inventory audit](milestone_10_documentation/inventory.md).
M09-08 accepted by lead continuation; M09 documentation/milestone closure accepted.
All 229 pages, 317 public headers and 33 current products have inventory owners.
Historical handoffs preserved; later app relocation reconciled at current HEAD.
Two stale writer descriptions corrected; links/includes pass. No build/runtime.
M10-01 awaits acceptance; M10-02 is next. Older delivery notes below are history.

Latest delivery: [M09-08 cumulative documentation](milestone_09_cleanup/documentation.md).
M09-07 accepted by continuation. Final maintained set: 229 pages, 1,870 checked
local links/anchors and 111 project include occurrences; no audit errors.
All 1,327 implementation/profiling files are preserved. No compilation/runtime.
M09-08 awaits acceptance before M10-01. Older delivery paragraphs are historical.

Latest delivery: [M09-07 final production API review](milestone_09_cleanup/review.md).
All 1,620 original manifest paths are reconciled; all planned removals are absent.
The 232-page handoff assigns cumulative authoring to M09-08 and localized auditing
to M10. Final/native/product source audits pass; no compilation/runtime execution.
M09-06 including scene/Metal/VMA corrections is accepted by continuation; native
scenario/backend details remain unspecified. Older pending statements below are
historical and superseded by this acceptance.

Latest delivery: [M09-06 final product validation and build corrections](milestone_09_cleanup/final-product-validation.md).
Lead next reported unresolved VMA functions at shared-library link. Restored the
sole VMA implementation block from the removed render wrapper into existing
gpu/vk/Device.cpp, preserving diagnostics/logging. No CMake change or agent build;
complete build/link validation remains pending.
The relocated Metal GUI source now enables IMGUI_IMPL_METAL_CPP locally before
its backend header, correcting four undeclared functions reported by the lead.
No CMake/vendor/public-header change; compilation remains lead-owned.
Lead reported a macOS arm64 Clang failure: countSceneNodes was missing from
db/scene.cpp after shared API cleanup. Restored the neutral recursive helper;
source audits refreshed with validation-corrections.tsv. Agent compilation was
not performed; correction awaits lead validation. The following paragraph records
the original source-only delivery before this correction.
M09-05 is accepted by continuation; compilation/checks assumed successful under
the convention, backend/scenario details unspecified. Source checks cover 1,210
runtime/build/profiling files, 317 public headers and 11 umbrella roots reaching
255 headers; 33 targets and 33 source-backed validation rows are cataloged.
Four delta ledgers reconcile 592 paths, including 440 absent files. No engine,
app, fixture, shader, CMake, template or product script changed. No configure,
compilation, native/test/package/standalone execution or binary/artifact inspection
occurred. M09-07 reviews the final API/evidence before M09-08 documentation.

M01-01 accepted by project lead continuation; compilation/checks assumed successful under the agreed convention. Backend/scenario coverage not specified. M01-02 accepted by project lead continuation; compilation/checks assumed successful, backend coverage unspecified. M01-03, including the Metal warning correction, accepted by project lead continuation; coverage unspecified. M01-04 accepted by project lead continuation; compilation/checks assumed successful, backend/deployment coverage unspecified. M01-05 accepted by project lead continuation; compilation/checks assumed successful, coverage unspecified. M01-06 accepted by continuation after the lead corrected compilation issues; backend coverage unspecified. M01-07 accepted by project lead continuation; compilation/checks assumed successful, coverage unspecified. M01-08 accepted by project lead continuation; compilation/checks assumed successful, coverage unspecified. M01-09 accepted by project lead continuation; compilation/checks assumed successful, coverage unspecified. M01-10 accepted after lead compilation corrections; irradianceImage rename and explicit CommandBuffer include reviewed as behavior-preserving. M01-11 accepted by continuation; compilation/checks assumed successful, backend coverage unspecified. M01-12, including its type-validation correction, accepted by project lead continuation instruction; backend coverage unspecified. M01-13 accepted by project lead continuation instruction; backend coverage unspecified. M01-14 accepted by project lead continuation after adding virtual Window destructor; backend coverage unspecified. M01-15 accepted by project lead continuation instruction; backend/scenario coverage unspecified. M01 HDR correction explicitly validated; transparency remains deferred. M01-16 explicitly validated by the lead and M01-17 documentation closed with separate validation waived. M02-01 accepted by lead continuation; lead reports validation through this state on macOS Metal and Linux Vulkan. Chronological handoffs below preserve earlier states.

## Rules for maintaining the next-step field

- Always record the step ID, title and relative Markdown link to its actual step file in **Next step to implement**. Do not replace it with “see the plan” or require the project lead to choose the file.
- Before starting work, read that linked step and its milestone summary, reconcile the ledger/prerequisites and any current lead instruction, then set the active step. During partial work the next-step field points to that same step with action `resume`.
- After delivering a step, set **Next step to implement** to the following required step in the current summaries, including milestone review steps. Keep the delivered step in `awaiting_project_lead`, set next action to `wait_for_project_lead`, and name that delivered step in the condition field. The next step is identified but is not authorized by delivery alone.
- When the lead asks to continue, accept the pending delivered step under the agreed validation convention, clear that condition and implement the already-linked next step without asking for its ID or filename.
- A reported failure changes the next-step field to the affected step with action `fix`; correction takes precedence over the successor. After delivering the correction, restore the successor and wait for lead validation again.
- After any plan revision, recompute and write the correct link/condition; a stale pointer must be corrected by the agent, not assigned back to the project lead.
- After delivering the final plan step, use `None — final project lead acceptance pending` with action `wait_for_project_lead`. After final acceptance, use `None — plan complete` with action `complete`.

The ordered summaries and actual ledger determine the correct pointer. The pointer is the required handoff entry point and must agree with them; it is not permission to bypass acceptance or prerequisites. See the [execution protocol](execution-protocol.md) for exceptions such as explicit user direction or interrupted work.

## Touched-step ledger

Add a row for each started step. Steps absent from this table are `not_started`. Include corrections/acceptance in the same row and preserve the chronological history below.

| Step and plan link | State | Implementation revision / working-tree scope | Agent static checks | Project lead validation / acceptance basis | Issues and handoff |
|---|---|---|---|---|---|
| [M10-06](milestone_10_documentation/step_06_final-review.md) | `awaiting_project_lead` | Final review plus requested corrections to GPU 11/13 and four standalone preflights; maintained guides updated. | 18 isolated/static correction checks, shell syntax, maintained links/includes, full draw 12/13 assemblies, source/change boundaries and whitespace pass; no compilation/native execution. | Source corrections and final acceptance pending lead validation. | [review.md](milestone_10_documentation/review.md), [source-corrections.md](milestone_10_documentation/source-corrections.md); both findings repaired; no successor. |
| [M10-05](milestone_10_documentation/step_05_documentation-validation.md) | `accepted` | Whole-document audit and 16-page correction; inherited work preserved. | 229 reachable pages; 1,913 links/anchors, 111 includes; 30 exact excerpts, six static diagrams; 1,276 non-doc hashes unchanged; whitespace checks. | Accepted by project lead continuation instruction; backend/scenario details not specified. | [documentation-validation.md](milestone_10_documentation/documentation-validation.md); both source follow-ups open; next M10-06. |
| [M10-04](milestone_10_documentation/step_04_apps-build-distribution.md) | `accepted` | HEAD e2a32164 plus inherited M10-01–03; ten document corrections including PACKAGING, source/CLI/deployment evidence and final-review handoffs. No executable-script/CMake edits. | 18 contracts/17 pages, 50 source hashes, 38 option rows, 292 derived outputs; 1,899 links/anchors and 111 includes; 1,275 unchanged non-doc files excluding PACKAGING; whitespace checks. | Accepted by project lead continuation; coverage unspecified. | [apps-distribution-audit.md](milestone_10_documentation/apps-distribution-audit.md); standalone path blocker and inherited UBO source follow-up open; next M10-05. |
| [M10-03](milestone_10_documentation/step_03_tutorials.md) | `accepted` | HEAD e2a32164 plus inherited M10-01/02; 14-page tutorial delta and source/snippet/coverage evidence. No engine/example/shader/CMake edits. | 31 pages/26 examples, 426 blocks, full 12/13 source assemblies, 189 source hashes; 1,891 links/anchors/111 includes; 1,276 non-doc hashes unchanged; whitespace checks. | Accepted by project lead continuation; coverage unspecified. | [tutorials-audit.md](milestone_10_documentation/tutorials-audit.md); shared UBO source follow-up explicit; next M10-04. |
| [M10-02](milestone_10_documentation/step_02_api-reference.md) | `accepted` | HEAD e2a32164 plus inherited M10-01; 18-page localized API delta, source/contract mapping, pending audit handoffs. No engine/CMake/shader changes. | 37 contract families, 55 pages, 115 source anchors; 1,881 links/anchors, 111 includes, zero errors; 1,276 preserved non-doc files and whitespace checks. | Accepted by project lead continuation; coverage unspecified. | [api-reference-audit.md](milestone_10_documentation/api-reference-audit.md); next M10-03. |
| [M10-01](milestone_10_documentation/step_01_documentation-inventory.md) | `accepted` | HEAD e2a32164a781dbf420a4464b25ddce9feb862c6a; inventory/source/product/handoff artifacts, two localized writer descriptions and pending audit handoffs. No source/CMake edits. | 241 dispositions reconcile 229 pages; nine handoffs, 317 headers, 33 products; 1,870 links/anchors and 111 includes, zero errors; whitespace checks. No build/runtime. | Accepted by project lead continuation; backend/scenario coverage unspecified. | [inventory.md](milestone_10_documentation/inventory.md); next M10-02. |
| [M09-08](milestone_09_cleanup/step_08_documentation.md) | `accepted` | Maintained documentation, page dispositions/audit and handoff; no runtime/CMake edits. | 229 pages, 1,870 links/anchors, 111 includes; 1,327 implementation hashes unchanged. | Accepted by project lead continuation instruction; backend/scenario coverage unspecified. | [documentation.md](milestone_09_cleanup/documentation.md); next M10-01. |
| [M09-07](milestone_09_cleanup/step_07_review.md) | `accepted` | Review, 1,620-row retirement reconciliation, 232-page handoff and pending M09-08/M10 plan updates; source unchanged. | Final/native/product source audits, original-manifest reconciliation and 1,327 preservation hashes. No build/runtime execution. | Accepted by project lead continuation instruction; backend/scenario coverage unspecified. | [review.md](milestone_09_cleanup/review.md); next M09-08. |
| [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md) | `accepted` | Baseline `b955833f314d09a191de13b83c90cacc10666ae7` plus existing planning edits; new Renderer.hpp, pipeline/RenderLayer.hpp, environment/all.hpp, draw/all.hpp includes and contracts.md. Uncommitted. | Include/dependency, signatures, ownership and FrameContext call-site inspection; `git diff --check`. No builds/runtime tests. | Accepted by project lead continuation instruction; backend coverage unspecified. | Shared component ownership, borrowed per-call bindings, single-sample contracts and validated virtual dispatch; see contracts.md. |
| [M01-02](milestone_01_deferred_renderer/step_02_gpu-rendering-primitives.md) | `accepted` | Baseline `b955833f314d09a191de13b83c90cacc10666ae7` plus preserved planning/M01-01 edits. Common/GraphicsPipeline/CommandBuffer/PhysicalDevice/Device/Image headers; Vulkan and Metal implementations, Vulkan extension loading, new gpu/Rendering.cpp and tests/gpu/mrt_probe fixtures. Uncommitted. | Diff/signatures/ownership/callers, every PixelFormat mapping, Metal vendor-header API spellings, block balance and no higher-level GPU includes; `git diff --check`. No builds, shader compilation or runtime execution. | Accepted by project lead continuation instruction; backend/scenario coverage unspecified. | See gpu-rendering-primitives.md. Probe requires manual linking; CMake untouched. New attachment path is single-level 2D until M01-03. |
| [M01-03](milestone_01_deferred_renderer/step_03_gpu-subresources.md) | `accepted` | Baseline `b955833f314d09a191de13b83c90cacc10666ae7` plus preserved earlier uncommitted work. New gpu/Image.cpp, generic ranges/views/transfer APIs, both native image/command/resource-set implementations, Rendering.cpp, samplers/capabilities and Vulkan presentation usage forwarding; example 06 and expanded manual probe. Uncommitted. | Signatures/callers, mip/layer bounds/layouts, cached-view lifetime, staging/command completion, Metal vendor-header spellings, block/conditional/whitespace/dependency checks, local links and `git diff --check`. No compilation/shader compilation/C++ probes/runtime execution. | Accepted by project lead continuation instruction after the Metal warning correction; coverage unspecified. | See gpu-subresources.md and tests/gpu/subresources/README.md. No CMake edits in M01-03. Accepted after the Metal warning correction. |
| [M01-04](milestone_01_deferred_renderer/step_04_shader-libraries.md) | `accepted` | Baseline `b955833f314d09a191de13b83c90cacc10666ae7` plus preserved earlier uncommitted work. ShaderLib path/entry helpers, paired native draw libraries, CMake compilation/resource/install integration, recursive standalone copies and manual shader probe. Uncommitted. | Signatures/callers, stage entries/bindings, paired output uniqueness, shared includes, bundle/install/standalone paths, shell syntax, local links and `git diff --check`. No configuration/build/shader compilation/runtime execution. | Accepted by project lead continuation instruction; backend/deployment coverage unspecified. | See shader-libraries.md and tests/gpu/shader_libraries/README.md. Concrete renderer shaders remain in M01-08/10/11/13; M01-05 linked pending acceptance. |
| [M01-05](milestone_01_deferred_renderer/step_05_runtime-renderer.md) | `accepted` | Baseline `b955833f314d09a191de13b83c90cacc10666ae7` plus preserved earlier work. New DefaultRenderLoopDelegate header/source, lifecycle/invalidation hooks, FrameContext scene timing, RenderLoop scene counters/boundaries, MainLoop lifetime documentation and manual lifecycle fixture. Uncommitted. | Ownership/queued-change/exception paths, declarations/callers, legacy resize forwarding and presentation timing, aggregate field compatibility, includes/block balance, local links and `git diff --check`. No compilation/runtime execution. | Accepted by project lead continuation instruction; coverage unspecified. | See runtime-renderer.md and tests/draw/renderer_lifecycle/README.md. No CMake or GPU/production implementation edits in this step. Concrete RendererDeferred replacement/settings demonstration remains M01-15 after M01-10/12. M01-06 linked pending acceptance. |
| [M01-06](milestone_01_deferred_renderer/step_06_drawable-resources.md) | `accepted` | Baseline `b955833f314d09a191de13b83c90cacc10666ae7` plus preserved earlier work. New draw Drawable/Texture/MaterialBase headers/sources, typed dual DrawableComponent, shared bounds/camera paths, weak device-keyed draw TextureCache, native sampler bias/border mappings and image-decoder allocation ownership correction. Follow-up renamed new backend-realized Drawable API and internals from `render*` to `draw*`. Uncommitted. | Signatures/callers/includes, native dependency boundaries, mesh/material/texture clone ownership, mip/layer copy/layout restoration, BLAS release/build order, scene modes, cache key identity, native vendor-header symbols, block/parenthesis balance, local links and `git diff --check`. No configuration/compilation/runtime execution. | Accepted by project lead continuation after lead compilation fixes; coverage unspecified. | See drawable-resources.md and tests/draw/drawable_resources/README.md. No CMake edits. Standard two-UV/tangent mesh; M01-07 now provides reference serialization and draw-aware loading; complete resource export remains M06/M07. |
| [M01-07](milestone_01_deferred_renderer/step_07_db-levels.md) | `accepted` | Existing uncommitted work/lead fixes preserved. Raw scene document IO, graphics-free component/graph reference IO, strict schema factories, deferred draw resource hooks, draw scene/BG2/OBJ overloads and manual DB probe. Shared pure parsers retain production overloads; OBJ index/missing-attribute handling and allocation cleanup added. Uncommitted. | Declarations/call sites, production delegation, schema errors, lexical source-path rebasing, component ownership, native boundaries, local includes and delimiter balance, built-in parser/serializer definitions, Markdown links and git diff --check. No configuration/compilation/runtime execution. | Accepted by project lead continuation instruction; coverage unspecified. | See db-levels.md and tests/draw/db_levels/README.md. No CMake or GPU changes. Full draw resource export/glTF/MTL remain later work; environment source realization is M01-08. M01-08 linked pending acceptance. |
| [M01-08](milestone_01_deferred_renderer/step_08_environment.md) | `accepted` | Baseline b955833f314d plus preserved uncommitted work/lead fixes. Public environment resources/pass services, sphere/procedural sky/irradiance/GGX prefilter/BRDF generation, configurable paired shaders, safe snapshot publication, depth-aware SkyboxRenderer, scene blur/visibility and example 06 reuse. Uncommitted. | Declarations/call sites, resource/source/target lifetime, failure publication, shader bindings/packing/pairs/entries, face formulas and roughness/PI/BRDF conventions, local includes/delimiters, dependency boundaries, documentation links and git diff --check. No configuration/compilation/runtime execution. | Accepted by project lead continuation instruction; coverage unspecified. | See environment-tools.md and tests/draw/environment/README.md. No CMake, GPU or production render implementation changes. Deferred renderer environment synchronization remains M01-10; M01-09 linked pending acceptance. |
| [M01-09](milestone_01_deferred_renderer/step_09_bindings-visitors.md) | `accepted` | Baseline b955833f314d plus preserved earlier uncommitted work. Public ShaderData and frame/object/light/environment bindings, acquired-slot mutable data and retained resource owners; draw queue/extraction/draw/query visitors; paired shared ABI headers and GPU diagnostic stages; structural ResizeViewportVisitor extent adapter and manual fixture. Uncommitted. | Declarations/call sites, extraction reset/ownership/IDs/world matrices, classification/sort/visibility, ray buffer/submesh equivalence, slot guards/resource retention, exact C++/GLSL/Metal packing and native bindings, local includes/delimiters, shader pairs/entries, documentation links and git diff --check. No configuration/compilation/runtime execution. | Accepted by project lead continuation instruction; coverage unspecified. | See bindings-visitors.md and tests/draw/bindings/README.md. No CMake/GPU or legacy draw/queue/query binding/visitor implementation changes. Actual PBR/deferred passes remain M01-10; alpha-aware hit tables remain M01-11. M01-10 linked pending acceptance. |
| [M01-10](milestone_01_deferred_renderer/step_10_raster-layers.md) | `accepted` | Preserved earlier uncommitted work/lead fixes. RendererDeferred, public GBuffer/DeferredLayer/FinalLayer/RenderTarget/FullscreenPass/RasterFrame, native raster shader pairs, pre-acquire prepare hook, configurable environment native indices, sampled retained output and D16/D32 exact GPU copies; manual deferred fixture and downstream handoffs. Uncommitted. | Source declarations/call sites/lifecycle, acquired-slot retention, exact MRT/push/resource bindings/native stage partners, copied depth/transparent ordering, shader formulas/coverage, local includes/delimiters/whitespace, native dependency boundary, documentation links and git diff --check. No configuration/compilation/shader compilation/runtime execution. | Lead reports successful compilation after correcting Metal name collision and missing include; shader rename reviewed as behavior-preserving. Continuation accepted; runtime/backend coverage unspecified. | See raster-layers.md and tests/draw/deferred/README.md. No CMake edits or production render implementation changes. Both transparent classes use deferred per-item composites; no RT/effect placeholders. M01-11 linked pending acceptance. |
| [M01-11](milestone_01_deferred_renderer/step_11_query-soft-shadows.md) | `accepted` | Preserved lead corrections and earlier uncommitted work. Generic BLAS/instance opacity on Vulkan/Metal; public compute SoftShadowPass/settings/status and minimal dense UV/alpha table; acquired-slot TLAS/buffers/invocation sets; query-aware deferred variants/default fallback, explicit native image map and Vulkan mapped-write flushing; manual fixtures and downstream handoffs. Uncommitted. | Lead shader rename/include review; native SDK query/depth/opacity API spellings, C++/GLSL/Metal field layout/bindings, dense ID/submesh ranges, ownership/slot/invocation lifetime, fallback branches, declarations/call sites, includes/delimiters/whitespace, native dependency boundary, local links and git diff --check. No configuration/compilation/shader compilation/runtime execution. | Accepted by project lead continuation instruction; backend/runtime coverage unspecified. | See query-shadows.md and tests/draw/soft_shadows/README.md. Eight distinct cutout image/sampler pairs, explicit capacity error, no arbitrary RT material architecture. No CMake or production render implementation changes. M01-12 linked pending acceptance. |
| [M01-12](milestone_01_deferred_renderer/step_12_reflected-settings.md) | `accepted` | Live SettingsCatalog/SettingsCodec/SettingsImport and stateless RenderSettingsPreferences, typed callable reflection, built-in final/sky/shadow/environment/light/generator targets, unique generations and weak lifecycle queue integration; exact typed JSON/hex persistence; manual CPU fixture and downstream contracts. Uncommitted. | Local includes/delimiters, all codec type tags, exact any/enum handling, complete validation before setters, rollback paths, ordinary accessor definitions, dynamic availability/instance metadata, pre-acquire queue/generation/ownership, dependency boundaries, Markdown links and git diff --check. No configuration/compilation/shader/runtime execution. | Accepted by project lead continuation instruction, including the type-validation correction; backend coverage unspecified. | See reflected-settings.md and tests/draw/settings_codec/README.md. Existing production JSON parser/writer behavior preserved; const readers added. No CMake or production render implementation edits in this step. M01-13 linked pending acceptance. |

| [M01-13](milestone_01_deferred_renderer/step_13_upscaling-fsr.md) | `accepted` | Baseline b955833f314d plus preserved earlier uncommitted work/lead fixes. Open Upscaler/ResamplingUpscaler/FsrUpscaler, live ResolutionSettings, narrow GPU Vulkan SDK bridge, paired reconstruction/camera-motion shaders, renderer output-domain HDR/final contract, delegate safe policy/rebuild/rebind and automatic live catalog targets; manual application injection fixture and downstream handoffs. Uncommitted. | Local includes/delimiters, shader pairs/entries/push layout/Y sign, native SDK function/capability/state restoration signatures, partial context cleanup, slot/shared-resource ownership, lifecycle/reset/fallback/call sites, GPU dependency boundary, documentation links and git diff --check. No configuration/compilation/shader/runtime execution. | Accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend coverage unspecified. | See upscaling.md and tests/draw/upscaling/README.md. Camera-only finite perspective FSR, four SDK queued frames, no engine history/denoise/reactive mask. No CMake or production render changes. M01-14 linked pending acceptance. |

| [M01-14](milestone_01_deferred_renderer/step_14_settings-ui.md) | `accepted` | Baseline b955833f314d plus preserved earlier uncommitted work/lead fixes. New GraphicsSettingsWindow, native-free transient ReflectionWidget::editValue, facade hasPersistence/queueRestore, ui/all include, manual UI integration/checks and downstream handoffs. Uncommitted. | Local includes/delimiters, public native boundary, exact UInt/Double/enum/path storage, transient getter/setter proxies, weak completion lifetime, safe queue/generation/availability refresh, call sites/signatures, documentation links and git diff --check. No configuration/compilation/shader/runtime execution. | Accepted by project lead continuation after their virtual Window destructor correction; compilation/checks assumed successful under the agreed convention, backend coverage unspecified. | See settings-ui.md and tests/draw/settings_ui/README.md. Reference settings only need ResourcePicker HDR file intent; no legacy TextureWidgets preview migration. CMake and production RenderSettingsWindow untouched. M01-15 linked pending acceptance. |

| [M01-15](milestone_01_deferred_renderer/step_15_working-example.md) | `accepted` | Baseline b955833f314d plus preserved previous uncommitted work/lead Window fix. New draw_deferred_renderer main/target/checklist, examples registration/README; FinalLayer reconstructedColor constructor argument for correct pre-initialization injection; working-example.md and affected successor handoffs. Uncommitted. | Local includes/signatures/virtuals/delimiters, referenced asset existence, constructor/pass contracts, shared/weak ownership/cleanup, live scalar versus texture edits, scene-time/queue/reset/generation paths, CLI/target/resources, native boundary, documentation links and git diff --check. No configure/build/shader/runtime execution. | Accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. | See working-example.md and examples/draw/10_deferred_renderer/README.md. CMake edits limited to the explicit M01-15 target registration/deliverable. No production app changes. Runtime assertions/counters are implemented probes, not observed passing results. HDR correction explicitly accepted; separate transparency issue deferred. M01 closure authorized by the lead. |

| [M01-16](milestone_01_deferred_renderer/step_16_review.md) | `accepted` | Source revision b955833f314d unchanged; inherited uncommitted M01 code/plan/probes preserved. New review.md, nine pending summaries and concrete owning steps, root index/architecture/progress updates. Documentation-only review; no source/build mutations. | Actual public APIs/owners/ABI/native transfers/product callers inspected; pending M02–M10 implementation work compared; review/whole-plan relative path links and git diff --check. No configuration/compilation/shader/runtime execution. | Explicit lead validation on 2026-10-15. M01-15 accepted by project lead continuation; backend/scenario details unspecified. | Review distinguishes reusable primitives, deliberate later requirements and verification gaps. Selectable mode/schema, independent CPU/live modifier seam, depth/array/public-profile/windowless tasks explicit in owning steps. M03 architectural checkpoint stays lead-owned. Followed by M01-17 documentation under revised ordering; earlier delivery history retained. |
| [M03-04](milestone_03_ray_tracing/step_04_rtao.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved M03-01–03 edits; public tracing snapshots/RTAO, paired kernels/lighting variants, renderer integration, manual fixture and documentation. Uncommitted. | Source/ABI/native bindings/ownership, licenses/delimiters, 24 paired profile wrappers and local links; git diff --check. No compilation/runtime execution. | Accepted by project lead continuation; backend/scenario coverage unspecified. | See rtao.md; scalar history/filtering/UI remain later steps; legacy soft shadows retain separate eight-cutout-pair limit. |
| [M03-05](milestone_03_ray_tracing/step_05_rtgi.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved prior uncommitted work; public RTGlobalIllumination, native transport profiles, mode/lighting integration, manual fixture and docs. Uncommitted. | 76 new source files checked for licenses/delimiters/includes/preprocessor; 60 native wrappers, native lighting binding maps, API definitions, ownership/ABI and documentation links; git diff --check. No compilation/runtime execution. | Accepted by project lead continuation; backend/scenario coverage unspecified. | See rtgi.md; HDR alpha validity, diffuse IBL replacement and retained settings; histories/filter/UI pending; legacy soft-shadow table limit unchanged. |
| [M03-06](milestone_03_ray_tracing/step_06_rt-reflections.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved prior uncommitted work; public RTReflections, paired native profiles, display-relative targets, confidence/IBL composition, manual fixture and docs. Uncommitted. | 81 new source files checked for licenses/delimiters/includes/preprocessor; 60 native wrappers, eight paired lighting variants, descriptor completeness/Metal sampler bounds, API definitions, ABI/ownership and local links; git diff --check. No compilation/runtime execution. | Accepted by project lead continuation; backend/scenario coverage unspecified. | See rt-reflections.md; unpremultiplied hit radiance/fractional confidence, output-relative quality; separate histories/filter/UI remain pending. |
| [M03-07](milestone_03_ray_tracing/step_07_temporal.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved prior uncommitted work; public TemporalAccumulator/MotionVectorGenerator, paired compute, renderer invalidation/signatures, manual fixture and docs. Uncommitted. | Fifteen new source files, eight compute wrappers, ABI/native bindings, ownership/sequence/invalidation review, licenses/delimiters/preprocessor/includes, API definitions and local links; git diff --check. No compilation/runtime execution. | Accepted by project lead continuation; backend/scenario coverage unspecified. | See temporal.md; camera-only reprojection, conservative moving-object reset, same ordered queue and provisional recording chronology. Spatial filtering/UI remain pending. |
| [M03-08](milestone_03_ray_tracing/step_08_denoise.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved prior uncommitted work; public DenoiseFilter, paired profiles, after-temporal renderer integration, virtual accessor metadata, manual fixture and docs. Uncommitted. | Thirteen new source files, eight native wrappers, logical/native bindings/ABI, ownership/ordering, live metadata definitions, licenses/delimiters/preprocessor/includes and local links; git diff --check. No compilation/runtime execution. | Accepted by project lead continuation; backend/scenario coverage unspecified. | See denoise.md; exact scalar/HDR profiles, explicit surface guides, confidence/validity, per-call scratch; visible controls/preferences remain M03-09. |
| [M03-09](milestone_03_ray_tracing/step_09_existing-example-integration.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved earlier uncommitted work; live effect settings/codec, two existing examples, final processor/SMAA native kernels, shared noise, debug signals, query-capacity fallback, manual fixtures and handoff docs. Uncommitted. | 59 source licenses/includes/lexical delimiters; eight SMAA wrappers and binding/ABI review; graph ordering, live target registration, local links; git diff --check. No compilation/runtime. | Accepted by project lead continuation instruction, including corrected visible UI and requested manipulation light controls/defaults; backend/scenario coverage not specified. | See integration.md and advanced_integration manual matrix. Next M03-10, then M03-11 cumulative docs. |
| [M03-10](milestone_03_ray_tracing/step_10_review.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved uncommitted baseline; review.md, rt-design reconciliation, 19 pending step files/seven downstream summaries, cosmetic separators in three pipeline implementations. | Opening GPL scan across 338 scoped files; native/source/signal/settings/ownership review; focused static checks, local Markdown links, non-whitespace-token equality for cosmetic edits and git diff --check. No compilation/runtime. | Accepted by project lead continuation instruction; backend/scenario coverage not specified. | Next M03-11 cumulative docs; M04 not yet authorized. |
| [M03-11](milestone_03_ray_tracing/step_11_documentation.md) | `accepted` | HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved prior work; five new/twenty-four updated maintained pages, documentation.md coverage and review handoff. Documentation only. | 29 maintained pages, 344 local links/anchors, 73 header/source references, code fences, source-backed tutorial wiring/defaults, git diff --check. No compile/runtime. | Accepted by project lead continuation instruction; M03 closed, backend/scenario coverage not specified. | Last M03 step; next M04-01 after acceptance. |
| [M04-01](milestone_04_lightmaps/step_01_bake-contracts.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377`; new bake Types/LightmapSettings/LightmapPixels/BakerContext/LightmapBaker/all headers, three CPU implementations, manual contract fixture and source/mapping docs. Uncommitted. | Ten source GPL/includes/dependencies/delimiters; eleven accessor metadata properties and declarations/definitions/export checks; local links and git diff --check. No compile/runtime. | Accepted by project lead continuation instruction; backend/scenario coverage not specified. | Abstract execution only; no UV/GPU baker or preview promised. Next M04-02. |

| [M04-08](milestone_04_lightmaps/step_08_documentation.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`; cumulative docs and M04 coverage ledger, with pre-existing canonical tutorial retained. | 34 pages/505 links and anchors/fences; tutorial block equality; API/source/deployment inspection; git diff --check. No compile/runtime. | Accepted by project lead continuation instruction; backend/scenario coverage unspecified. | M04 closed. M05-01 selected. |

| [M05-01](milestone_05_offscreen/step_01_execution-selection.md) | `accepted` | Starting HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`, with preserved M04-08 uncommitted documentation; new public OffscreenExecutionMode, DrawOffscreenApplicationConfig/Delegate, overloaded OffscreenApplication init, mode query, overload validation, and downstream handoffs. | New headers inspected for complete GPL and no native/Vulkan/render includes; signature/caller cross-reference, declaration/definition inspection, 312 local links across changed Markdown and git diff --check. No configure/build/runtime. | M04-08 accepted by continuation. M05-01 accepted by project lead continuation instruction; backend/scenario coverage unspecified. | Draw init validates positive extent and selects mode; actual windowless engine/surface/frame execution is M05-02. Production delegate API retained. |

| [M05-02](milestone_05_offscreen/step_02_draw-offscreen-context.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved M04/M05-01 uncommitted work. Shared window/headless draw Engine initialization, generic surface/slots and dependent resources, actual typed app frame driver/cleanup, public DefaultOffscreenApplicationDelegate, fixed scene time, window/UI guards, manual fixture and source-backed API/plan handoff. | 18 owned sources: GPL, includes, lexical delimiters, function separation and interface/lifecycle review; 132 engine include paths; local Markdown links/anchors and git diff --check. No configure/build/shader/fixture/app/GPU execution. | M05-01 accepted by project lead continuation instruction; M05-02 accepted by project lead continuation, backend/scenario coverage not specified. | Synchronous immediateSubmit confirms completion before counters/callback; no failed frame counts. Default one requested frame, overlays excluded, frozen time by default. Exact linear averaging/config import remains M05-03. See headless-execution.md and tests/draw/offscreen/README.md. |
| [M05-03](milestone_05_offscreen/step_03_accumulation-settings.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved M04/M05 uncommitted work. Config/direct live import and resolution precedence, optional Renderer linear/final seam, Deferred split/frozen identity, CPU double FrameAccumulator over completed RGBA16F samples, exact default N driver/final-only completion hooks, manual reference/native probes and API/downstream docs. Uncommitted. | 24 owned sources: GPL, 160 engine includes, lexical delimiters/function separation; source signature/call/transfer/ownership/completion inspection, local documentation links and git diff --check. No configuration/build/shader/C++ fixture/GPU execution. | M05-02 accepted by project lead continuation; coverage unspecified. M05-03 accepted by project lead continuation after GLM packing include correction; backend/runtime coverage unspecified. | Synchronous per-sample readback/CPU mean and final RGBA16F upload are the declared reference cost. Final color/optional SMAA run once, result publishes after GPU completion. See accumulation-settings.md and both manual fixture READMEs. Next M05-04 conditional on acceptance. |
| [M05-04](milestone_05_offscreen/step_04_working-example.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved M04/M05 work and accepted M05-03 correction. New registered `draw_offscreen_application` CLI with scene/material/assets/environment/lights, strict CLI/config routing, frozen default, final PNG callback, cleanup and usage docs; examples/CMakeLists registration. Uncommitted. | GPL/style and lexical inspection for new app source, CLI/API/call/ownership and output callback inspection, CMake registration/resource-helper inspection, local Markdown links and git diff --check. No configure/build/run/image inspection. | M05-03 accepted by project lead continuation instruction after the GLM include correction; coverage unspecified. M05-04 accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. | Output defaults to offscreen_output/scene.png, PNG RGBA8. Explicit unknown skip reports target warnings; random scene node IDs (light, environment, lightPose) have no name remap. M05-05 adds verified temporary-PNG publication. |
| [M05-05](milestone_05_offscreen/step_05_readback-export.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved M04/M05 uncommitted work and accepted M05-04. The CLI validates tight RGBA8 length, writes a PNG-suffixed temporary file, decodes it, verifies dimensions and exact RGBA bytes, then publishes it as scene.png without replacing an existing path; failure removes the temporary. README and step handoff specify row orientation, alpha/color handling, UI/animation/accumulation comparison. Uncommitted. | GPL/style and source signature/lifecycle inspection, DB writer/decoder call verification, native row-order/viewport inspection, local Markdown links and `git diff --check`. No build, executable, fixture or image inspection. | M05-04 accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. M05-05 accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. | Project lead visual orientation/content and cross-backend inspection are still not specified. Review now follows. |
| [M05-06](milestone_05_offscreen/step_06_review.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved uncommitted M04/M05 implementation. New `milestone_05_offscreen/review.md` records actual execution/configuration/accumulation/readback/output contracts and revises pending M06–M10 requirements. No source or CMake changes. | API/source/call-site/lifecycle review, current source versus pending steps, revised M06–M10 links/contracts, Markdown relative links and `git diff --check`. No build/runtime/image checks. | M05-05 accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention; backend/artifact coverage not specified. M05-06 accepted by project lead continuation instruction; backend/scenario coverage unspecified. | Lead’s platform/runtime/image evidence remains unspecified. Review hands off to M05-07 cumulative maintained documentation after acceptance/continuation; M05 is not closed. |
| [M05-07](milestone_05_offscreen/step_07_documentation.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved inherited work. Five new maintained references/concept/tutorial/app pages and 23 updated affected pages, including packaging; new M05 documentation coverage ledger, review/index/summary handoff and explicit M06-01 closure prerequisite. Markdown only. | 28 maintained pages, 338 local links/anchors, balanced fences, three exact tutorial source excerpts, source/API/config/native-lifecycle/codec/target/asset inspection and git diff --check. No builds/runtime/visual checks. | M05-06 accepted by project lead continuation instruction; backend/scenario coverage unspecified. M05-07 accepted by project lead continuation instruction; supplemental build-from-scratch tutorial rewrite requested in M06-01; backend/scenario evidence unspecified. | See documentation.md. Complete scene/resource export remains M06/M07, UV standalone/batch M08, canonical promotion M09. Next M06-01 only after documentation acceptance/continuation. |
| [M06-01](milestone_06_model_edit/step_01_application-copy.md) | `accepted` | HEAD `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` plus preserved inherited work and resumed partial files. New apps/draw/model_edit target/source/README, minimal apps CMake registration, application-inventory.md mapping workflows/API calls, M06-02–06/M07-01 handoffs. Supplemental tutorial 13 now constructs the entire shipped example through 22 ordered C++ blocks. | Seven owned files checked for GPL/includes/native boundary/lexical structure and style, source/API/ownership/cleanup/resource-helper inspection, complete tutorial code assembly, Markdown links/anchors and git diff --check (working tree and index). No configure/build/shader/application execution. | M05-07 accepted by project lead continuation; coverage unspecified. Lead explicitly requested the full tutorial rewrite with M06-01. Accepted by project lead continuation; backend/scenario coverage unspecified. | Startup fixture is not complete editor parity. M06-02 implements Document and complete IO; M06-03 tools/widgets; M06-04 framing; M06-05 bake; M06-06 parity. Lead should build/run draw_model_edit, test settings/orbit/resize/cleanup and production coexistence, and review the tutorial assembly. |

| [M06-02](milestone_06_model_edit/step_02_complete-db-io.md) | `accepted` | Same HEAD plus preserved inherited/staged work. Shared MTL/glTF draw import, BG2 joints/submesh/material metadata, native GPU texture archive, complete resource publication, material/JSON round-trip fixes and editor Document/Open/Save/drop/unsaved flows. New API reference, resource-io.md and manual probe. No CMake edits. | 27 owned C++ files checked for GPL, block-opening style and lexical delimiter balance; manual API/ownership/metadata/rollback/dependency inspection; maintained Markdown link paths/fences; working-tree and index diff whitespace checks. No compilation, runtime, shader, packaging or probe execution. | M06-01 accepted by project lead continuation instruction; backend/scenario coverage unspecified. M06-02 accepted by project lead continuation instruction; backend/scenario coverage unspecified. | See resource-io.md and tests/draw/resource_io/README.md. Native .bg2tex requires draw loaders and little-endian texels; static import subsets/custom resource contracts/filesystem publication limits are explicit. Next M06-03 after acceptance. |

| [M06-03](milestone_06_model_edit/step_03_editor-tools.md) | `accepted` | Same HEAD with inherited/staged work preserved. EditorUi now uses production Workspace composition: docked Model Properties/Environment Options, full-width Toolbar, StatusBar, File/View/Window menus with panel checkmarks, menu-launched settings and maximized app startup. Prior document/material/environment editing workflows retained. No CMake edits. | Static GPL/style/lexical checks on touched app files, Workspace/Window/Menu API and callbacks inspected, manual layout matrix updated, maintained Markdown links/fences and working-tree/index whitespace checks. No configure/build/shader/application/probe execution. | M06-02 accepted under continuation convention. UI parity correction accepted by latest project-lead continuation; backend/runtime coverage unspecified. | See step_03_editor-tools.md, editor-tools.md, doc/apps/draw_model_edit.md and tests/draw/editor_tools/README.md. |
| [M06-04](milestone_06_model_edit/step_04_viewpoint-camera.md) | `accepted` | Same HEAD with inherited/staged work preserved. Bounds framing and explicit draw ViewpointAnalyzer integration in apps/draw/model_edit; queued weak generation/member-checked actions; candidate/visible-submesh stats and status UI; documentation and lead matrix. No CMake edits. | Static source/API/lifecycle/identifier mapping inspection, Markdown path/fence checks, and working-tree/index whitespace checks. No configure/build/shader/application/probe execution. | Accepted by latest project-lead continuation; backend/runtime coverage unspecified. | See viewpoint-camera.md. Bake integration proceeds in M06-05. |
| [M06-05](milestone_06_model_edit/step_05_editor-baking.md) | `accepted` | Same HEAD with inherited/staged work preserved. New ModelLightmapWindow and EditorBaking source; Window menu integration; UV1/UV2 and result previews; safe UV2 regeneration; integrated AO/GI context/frame lifecycle, cancel, source invalidation, apply-to-material and native sidecar export path. No CMake changes. | GPL/style/lexical checks on new and touched app files; bake/UV/image APIs, frame completion ordering, weak target/document guards, material metadata, cleanup and complete export paths inspected; docs links/fences and working-tree/index whitespace checks. No configure/build/shader/application/probe execution. | M06-03 UI correction and M06-04 accepted by project-lead continuation; coverage unspecified. M06-05 accepted by project-lead continuation; backend/scenario coverage unspecified. | See step_05_editor-baking.md, application-inventory.md, doc/apps/draw_model_edit.md and tests/draw/editor_tools/README.md. Lead should run UV, AO/GI, cancellation/invalidation, apply and save/reload matrix on available backends. |
| [M06-06](milestone_06_model_edit/step_06_product-validation.md) | `accepted` | Static package and parity review recorded in product-validation.md. draw_model_edit is registered via bundle_app_sdl; shared package collection installs registered apps, engine runtime dependencies, shaders and assets. No CMake changes. | Read-only inspection of app target helper, package target collection, install/resource rules and platform script configuration; inventory/manual scenarios reconciled into project-lead evidence matrix; working-tree/index whitespace checks. No configure/build/shader/package/application/probe execution. | M06-05 accepted by project-lead continuation; backend/scenario coverage unspecified. M06-06 accepted by latest project-lead continuation; runtime and package evidence remain unspecified. | See product-validation.md. Static package routing is confirmed; native launch, parity and clean-checkout package execution remain an explicit verification gap for M09/M10 gates. |
| [M06-07](milestone_06_model_edit/step_07_review.md) | `accepted` | New review.md compares actual model editor DB/UI/bake/viewpoint/application contracts with M07–M10 requirements and updates M07–M10 handoffs. No source or CMake changes. | Read actual public API declarations and implementation records, checked affected pending summaries/steps, package-source handoff, review and downstream Markdown paths/fences, `git diff --check` and index equivalent. No compile/runtime/shader/package execution. | M06-06 accepted by project-lead continuation; runtime/package coverage remains unspecified and explicitly documented. M06-07 accepted by project lead continuation after editor corrections; zero-light rendering explicitly reported correct. | See review.md. Next M06-08 cumulative documentation; M06 remains open and M07 entry is gated on M06-08 acceptance. |
| [M06-08](milestone_06_model_edit/step_08_documentation.md) | `accepted` | Same HEAD and inherited working tree preserved. Maintained app/tutorial/UI/settings/IO/architecture documentation reconciled through M06; new model editor tutorial and documentation.md inventory; review/summary and manual evidence reconciled. Documentation-only; no CMake changes. | Source/CLI/header/resource review; local links/anchors and fenced blocks; working-tree/index whitespace checks. No build/runtime/shader/package execution. | M06-07 and editor corrections accepted by latest project lead continuation. Lead explicitly reports correct zero-light rendering; other backend/package coverage unspecified. | M06-08 accepted by project lead continuation instruction; backend/package coverage unspecified. Next M07-01. |

| [M07-01](milestone_07_bg2e_composer/step_01_application-copy.md) | `accepted` | HEAD bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d plus inherited changes preserved; new apps/draw/bg2e_composer app sources/README/target, apps registration and M07 inventory/handoff. | GPL/private includes/identity/draw boundary, resource helper, ownership and whitespace inspected; no configure/build/shader/runtime/package execution. | M06-08 accepted by project lead continuation instruction; backend/package coverage unspecified. | M07-01 accepted by project lead continuation instruction; backend/package coverage unspecified. Next M07-02; Composer-specific gaps mapped in application-inventory.md. |

| [M07-02](milestone_07_bg2e_composer/step_02_scene-io-cloning.md) | `accepted` | HEAD bd6a95076141335550f415f47ea2a824703b119c; initially clean checkout. Composer DocumentIo/stage/menu/camera IO, shared Scene root transfer, Node subclass lookup/clone guards, factory/projection/camera/controller fixes, dependent Model Edit projection clone, source probes and documentation. | Static declarations/call sites/GPL/style/delimiters/ownership/IO/mode/include/link/whitespace checks; no configure/build/shader/runtime/probe/package execution. | M07-01 accepted by project lead continuation instruction; backend/package/scenario coverage unspecified. | Accepted by project lead continuation instruction; backend coverage unspecified. Actual contracts in scene-io-cloning.md; lead checks in tests/draw/composer_io. Next M07-03, including full component/controller UI and fixture/light policy. |
| [M07-03](milestone_07_bg2e_composer/step_03_scene-editing.md) | `accepted` | HEAD f7c2498cb4c29e27921c91809043dcf5fe4d963c; initially clean. Composer SceneEditor/actions/tools/camera/input, shared SceneTree intents, explicit node-only selection and Drawable helper separation. | Static API/include/ownership/rollback/camera/light/GPL/style/Markdown/whitespace checks; no build, shaders, runtime or packaging. | M07-02 accepted by project lead continuation instruction; backend/scenario coverage unspecified. | Accepted including bool-reference correction by project lead continuation instruction; backend coverage unspecified. scene-editing.md records actual contracts and lead checklist. Next M07-04. |
| [M07-04](milestone_07_bg2e_composer/step_04_async-import-service.md) | `accepted` | HEAD f7c2498cb4c29e27921c91809043dcf5fe4d963c plus preserved M07-03 work. New app-local importer/server/settings/queue UI and stopped publication/rollback; reusable CPU glTF/OBJ/BG2 realization boundaries. | Static GPL/style/delimiter/include/API/thread/ownership/rollback/protocol/Markdown/whitespace review; no configuration/build, shader, parser, HTTP, app, package or probe execution. | Accepted by project lead continuation instruction; backend/package/HTTP coverage unspecified. | Actual contracts in async-import-service.md; lead matrix in tests/draw/composer_import. M07-05 follows. |
| [M07-05](milestone_07_bg2e_composer/step_05_scene-baking.md) | `accepted` | Same HEAD plus preserved inherited work; app-local serial selected-subtree/scene batches, retained per-target outputs, staged texture preparation/adoption, previews and cleanup. No CMake/core/shader edits. | Static API/call-site/source/ownership/UV/output/GPL/style/link/whitespace checks; no configuration/build/shader/app/GPU/package/probe execution. | Accepted by project lead continuation instruction; backend/package coverage unspecified. | Actual contracts in scene-baking.md; manual checks in tests/draw/composer_baking. M07-06 follows. |
| [M07-06](milestone_07_bg2e_composer/step_06_product-validation.md) | `accepted` | Same HEAD plus inherited work preserved; product-validation.md, workflow/distribution matrix, restored Chain controls and selection-crash correction: missing Composer list scope plus bool beginList handling in every caller. No CMake/vendor edits. | Static source/API/Chain clone/schema/ownership/style, package routing, table scopes, GPL/delimiters, Markdown paths/fences and whitespace checks; no build/shader/app/HTTP/GPU/package execution. | Lead explicitly confirms the selection correction is checked and requests continuation; M07-06 including corrections accepted. Backend/package detail unspecified. | Lead matrices retained; M07-07 follows. |
| [M07-07](milestone_07_bg2e_composer/step_07_review.md) | `accepted` | Original review f7c2498cb4c29e27921c91809043dcf5fe4d963c; ae0de5bbe015 plus inherited UI/IO/shadow/native-table work. Latest Environment correction: explicit Scene source, disabled-branch filtering, in-place inspector edit, HDR aliases/clear, fresh-document selection/rebuild, visible error. | GPL/style/API/ABI, weak owner/selection/destruction, renderer/queue/settings agreement, rollback, reflection Clear/empty semantics, docs/local links/whitespace. No build/shader/app/HTTP/GPU execution. | Accepted by project lead continuation instruction on 2026-10-21; compilation/checks assumed successful under the agreed convention, coverage unspecified. | Historical correction reports retained; M07-08 documents current source contracts. |
| [M07-08](milestone_07_bg2e_composer/step_08_documentation.md) | `accepted` | HEAD 78943d5820e69363c942d71136f479a0df87128e, clean entry; maintained Composer app/tutorial/scene/import docs, joined MainLoop workers, UI intent/Clear contracts, current tracing v3/shadows v5; documentation.md coverage and pending-owner reconciliation. Markdown only. | Static source/signature/lifecycle/ABI/path/local-link/fence and diff whitespace checks. No compilation/shader/app/HTTP/GPU/package execution. | Accepted by project lead continuation instruction before redirection to plan audit; backend/measurement detail unspecified. | M08-01 remains next; latest instruction scopes work to source/plan audit only. |
| [M08-01](milestone_08_lightmap_generator/step_01_standalone-context.md) | `accepted` | HEAD 78943d5820e69363c942d71136f479a0df87128e with inherited Markdown preserved. New bake StandaloneBakerContext/StandaloneLightmapBaker, explicit scene generations, one live target/private serial slot, required-profile/TLAS preflight, synchronous validated samples, failure/cancel/stale guards. Public primitive reuse; no render/gpu/CMake/shader edits. | Static API/dependency/native completion/source revision/ownership/lifecycle, GPL/style/include/link/whitespace checks; manual standalone fixture added but not run. No build/shader/app/GPU execution. | Accepted by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. | standalone-context.md and tests/bake/standalone/README.md; M08-02 follows after acceptance. |
| [M08-02](milestone_08_lightmap_generator/step_02_scene-assembly.md) | `accepted` | Same HEAD with inherited work retained. New bake StandaloneBakeSceneAssembler, shared schema/CPU BG2/OBJ preparation, camera-less single/multi-root prefab ownership, enabled stable targets, helpers excluded, staged independent UV2, optional target realization and known canonical dependencies. | Static signatures/includes/shared schema/hook/root/resource/UV/source ownership, GPL/style/local paths and whitespace. Manual assembly probe added, not run. No build/shader/app/GPU/package execution. | Accepted by project lead continuation instruction; compilation/checks assumed successful under the convention, backend/scenario coverage unspecified. | scene-assembly.md and tests/bake/assembly/README.md; M08-03 must supplement MTL/custom dependency discovery before protected output. |
| [M08-03](milestone_08_lightmap_generator/step_03_batch-output.md) | `accepted` | HEAD 78943d5820e69363c942d71136f479a0df87128e with inherited work retained. New bake batch/writer, complete dependency/preflight/overwrite checks, staged generation/final manifest publication, exact typed maps and independent native model copies; generic DB CPU archive/publication helpers and OBJ MTL inventory. | Static API/dependency/license/include/style, source/lifecycle/archive/publication ordering, local links and git diff --check. Opt-in probes added, not run; no compile/shader/app/GPU/CMake execution. | Accepted by project lead continuation; compilation/checks assumed successful, backend/scenario coverage unspecified. | batch-output.md and tests/bake/batch/README.md; next M08-04 after acceptance. |
| [M08-04](milestone_08_lightmap_generator/step_04_cli-application.md) | `accepted` | Same HEAD with inherited work retained. New draw_lightmap_generator parser/delegate/main, typed live bake config/CLI-last overrides, backend/mode/UV2/copy/format controls, signal/exit policy, CMake target/registration and guide/example/probes. | GPL/includes/lexical/style/whitespace, legacy option coverage, typed example/codec ordering, no-camera owner lifecycle, shared shader/package rule inspection and local links. No configure/build/shader/app/probe/GPU execution. | Accepted by lead continuation; compilation/checks assumed successful, backend/scenario coverage unspecified. | cli-application.md, app README and tests/bake/cli; M08-05 follows after acceptance. |
| [M08-05](milestone_08_lightmap_generator/step_05_product-validation.md) | `accepted` | Same HEAD with inherited work retained. Source parity/product/deployment/regression record, independent read-only PNG/native/BG2-material artifact inspector, synthetic fixture tests and preserved production documentation/package handoffs. | Ten synthetic CPU inspector tests passed; source/protocol/format/deployment-rule/local-link/whitespace checks. No C++/shader build, engine/native probe/package execution or measured native evidence. | Accepted by lead continuation; compilation/checks assumed successful, backend/scenario details unspecified; P01–P11 provenance remains explicit. | product-validation.md and tests/bake/product; M08-06 next after acceptance. |
| [M08-06](milestone_08_lightmap_generator/step_06_review.md) | `accepted` | HEAD 78943d5820e69363c942d71136f479a0df87128e plus preserved inherited work. Actual technical review, R01–R14/product/contract/evidence tables, 1338-file candidate retirement inventory/generator, concrete pending M08-07/M09/M10/root revisions. | 25 scoped C/C++ GPL/include/delimiter/control-brace checks and structural style review; source/consumer/lifecycle/publication/resource/protocol comparison, inventory existence/uniqueness/count/hash, Python syntax/local links/whitespace. No build/app/native probe/test/package execution. | Accepted by lead continuation; compilation/checks assumed successful, native backend/scenario detail unspecified. | review.md/removal-impact.md; next M08-07 after acceptance. No M09 deletion performed. |
| [M08-07](milestone_08_lightmap_generator/step_07_documentation.md) | `awaiting_project_lead` | Same HEAD and preserved inherited work. Nine new/twenty-two updated maintained pages; standalone API/IO/tutorial/CLI/architecture, corrected bool-noexcept save/parent/copy-fallback guidance, coverage/source ledger and review closure note. | 31 affected pages/409 local targets and anchors, fences/includes/JSON; all 25 CLI flags; static signature/default/example/source ordering, current precision/save/resource contracts and whitespace. No configure/build/app/native probe/test/package execution. | M08-06 accepted by continuation; documentation acceptance pending, backend/scenario detail unspecified. | documentation.md; next M09-01 after acceptance. Production remains intact and no final draw promotion is claimed. |


## Open issues

GPU 13 presentation correction is delivered for the reported cross-format copy
abort; native confirmation remains pending. See
[correction](milestone_10_documentation/rt-display-correction.md). No new native
backend/driver validation result is inferred.


No open source finding remains from M10-03/04. The requested corrections are
delivered in [source-corrections.md](milestone_10_documentation/source-corrections.md)
and await lead validation. Older issue entries below retain historical status.

M07 correction awaiting validation: the lead reports macOS exiting with tracing tables at
images=129, samplers=129, capacity=128. This is an engine table/profile overflow,
not established native Metal pipeline failure. The loading correction and
[source-backed options](milestone_07_bg2e_composer/loading-and-tracing-investigation.md)
are delivered. The lead subsequently authorized warning/no-texture behavior for
overflowed pairs, now implemented in shared snapshot construction and both native
sampling helpers. Native budgets/profiles and raster materials are unchanged.
The application/shader correction is not runtime-validated; M07-08 remains
conditional on lead acceptance of this correction and earlier review/UI work.

Subsequent lead direction raises the practical table ceiling to the exact minimum
of 16384 and device budgets after reservations. [Implemented policy](milestone_07_bg2e_composer/tracing-capacity-16384.md)
supersedes historical small profiles/Metal 256 limits, keeps the neutral fallback
and object-local materials, and requires rebuilt native v2 shaders. Validation
remains pending; native sampler budgets may constrain capacity well below 16384.

The project lead reported a Metal deprecation warning in M01-03. The texture usage check now calls `isFramebufferOnly()` instead of deprecated `framebufferOnly()`; the corrected step was accepted by project lead continuation. M01-02 was accepted by continuation with unspecified backend coverage. M01-03 checks are assumed successful under the continuation convention; the expanded manual probe has not been run or registered in CMake. M01-04 checks are assumed successful under continuation, with unspecified coverage; M01-05 accepted by continuation with unspecified coverage. M01-06 accepted after lead compilation fixes; backend/scenario coverage unspecified. M01-07 accepted by project lead continuation, compilation/checks assumed successful and coverage unspecified. M01-08 accepted by continuation with unspecified coverage. M01-09 accepted by continuation with unspecified coverage. M01-10 accepted after lead compilation corrections and reviewed shader rename; runtime coverage unspecified. M01-11 accepted by continuation; compilation/checks assumed successful, backend coverage unspecified. M01-12 accepted by project lead continuation instruction, including its type-validation correction. M01-13 accepted by project lead continuation instruction; backend coverage unspecified. M01-14 accepted after the lead virtual Window destructor correction. M01-15 accepted by project lead continuation; backend/scenario coverage unspecified. M01-16 review acceptance is held for the reported M01-15 overflow correction. The M01-11 explicit alpha table supports eight distinct image/sampler pairs and throws on a ninth; M03 reconsiders arbitrary addressing. No known static failures remain; M01-13 upscaling and M01-12 CPU/persistence/lifecycle fixtures have not been compiled or run by the agent. RendererDeferred did not exist at M01-05: the manual minimal-renderer fixture covers the intermediate lifecycle, with the actual deferred/settings replacement required in M01-15.

## Decisions and handoff notes

- Implement one selected step per request, then stop for project lead validation.
- Compilation, shader compilation, runtime tests and build-producing scripts are delegated to the project lead by default; the agent performs relevant static checks.
- A project lead instruction to continue accepts the previously delivered step as compiled/checked without reported failures. Record that basis without inventing test details or asking for reconfirmation.
- A reported failure requires correction before advancing; deliver the corrected step for lead validation again.
- Maintain the explicit next-step link/action/condition at each handoff. The project lead can reference index.md and progress.md and simply request the next step. The last milestone review step is required and updates all affected pending milestones.

Future entries should record actual interfaces/paths/decisions and pending-plan changes needed by the next chat. Do not use this section to keep disconnected configuration values or replace architecture-decisions.md.

| [M09-01](milestone_09_cleanup/step_01_retirement-manifest.md) | `accepted` | HEAD 1889bdb plus inventory/manifest artifacts and plan handoff. | Source/path/hash/include/reference/Markdown checks; no build/native run. | Accepted by project lead continuation instruction; backend/scenario details unspecified. | Historical snapshot retained; M09-02 supplies explicit retirement delta. |
| [M09-02](milestone_09_cleanup/step_02_canonical-entrypoints.md) | `accepted` | Canonical app/window/headless contracts, retired app/native adapter files and registrations, ported input fixture, maintained entrypoint docs and entrypoint-changes.tsv. Uncommitted over inherited M09-01 work. | Retained offscreen lifecycle equivalence; unchanged draw frame/cleanup and app identities; 14 source GPL/include/delimiter reviews; supported reference/registration, docs/hash/whitespace checks. No compile/runtime. | Accepted by project lead continuation instruction; backend/scenario details unspecified. | M09-03 next; shared/native cleanup, full templates/packages and namespace retirement remain pending. |

| [M09-03](milestone_09_cleanup/step_03_shared-api-cleanup.md) | `accepted` | Inherited M09-01/02 preserved; shared schema/scene/DB/utils/tools/UI cleanup, public bake export, dependent native renderer retirements, fixtures/docs and shared-api-changes.tsv. Uncommitted. | 109-source GPL/include/delimiter/style review; 273 protected files byte-identical; 40 parser/clone/save functions unchanged modulo accessor/whitespace; shared API/member/include and docs/hash checks. No build/runtime. | Accepted by project lead continuation instruction; backend/scenario details unspecified. | M09-04 next: 23 remaining render files, noise relocation and native UI/platform confinement; full test/distribution validation remains lead-owned. |

| [M09-04](milestone_09_cleanup/step_04_native-render-deletion.md) | `accepted` | Inherited work preserved; final render retirement, exact noise relocation, native GUI/window GPU bridges, source ledger/audit and docs. Uncommitted. | 704 supported-source checks, 41 delta hashes, 18 GPL/delimiter reviews, 16 native methods preserved, 308 protected files unchanged; graph/data/docs/whitespace checks. No build/runtime. | Accepted by project lead continuation instruction; backend/scenario details unspecified. | M09-05 next; see native-retirement.md for contracts, lifecycle checks and remaining distribution/test work. |

| [M09-05](milestone_09_cleanup/step_05_targets-tests-packages.md) | `accepted` | Inherited work preserved; 173 retired files, five CMake repairs, canonical standalone template/scripts, extended registered probes and product ledger/audit/docs. Uncommitted. | 190 delta hashes, 1,205 runtime/build/script files, 33 resolved registrations, 292 paired outputs/290 include edges, 707 native-boundary files, protected source hashes, GPL/includes/style, shell parse and whitespace/docs checks. No build/native/product script execution. | Accepted by project lead continuation instruction; backend/scenario details unspecified. | M09-06 next; see products-distribution.md and probe-dispositions.tsv for retained coverage, contract differences and validation limits. |

| [M09-06](milestone_09_cleanup/step_06_final-product-validation.md) | `accepted` | Final source/umbrella/API/product catalogs, audit tooling, 33-scenario matrix, compatibility/ownership report and pending-plan handoff; inherited runtime work untouched. Uncommitted. | 1,210 source files, 317 public headers/11 roots/255 reachable, 33 targets, 592 reconciled delta paths; existing product/native audits, unchanged 1,327-file snapshot, source paths/docs/whitespace checks. No build/native/product/artifact execution. | Corrected delivery accepted by project lead continuation instruction, including scene counter, Metal declarations and VMA linking fixes; backend/scenario coverage unspecified. | M09-07 next; final-product-validation.md distinguishes source checks, source declaration/export clues and actual native/binary/round-trip evidence. |

## Chronological history

| Event | Step | Evidence / result |
|---|---|---|
| Workflow record initialized | — | Cross-chat/project lead validation protocol added. No plan implementation step started or accepted. |
| Implementation started | M01-01 | Selected from the initialized next-step pointer; pre-existing user planning changes preserved. |
| Implementation delivered | M01-01 | Public contracts and dispatch added; static checks complete, compilation/runtime delegated to project lead. M01-02 linked, conditional on acceptance. |
| Project lead continuation | M01-01 / M01-02 | M01-01 accepted by continuation instruction; compilation/checks assumed successful under the agreed convention, coverage unspecified. M01-02 started. |
| Implementation delivered | M01-02 | MRT/state/formats/capabilities and paired probe fixtures delivered. Static checks complete; build/runtime delegated. M01-03 linked pending acceptance. |
| Project lead continuation | M01-02 / M01-03 | M01-02 accepted under the agreed convention; compilation/checks assumed successful, backend/scenario coverage unspecified. M01-03 started. |
| Implementation delivered | M01-03 | Ranges/views, 2D mip allocation/targeting, packed/HDR transfers, per-subresource tracking and native mip generation delivered. Example 06/probe updated; static checks complete. M01-04 linked pending acceptance. |
| Warning correction delivered | M01-03 | Lead reported deprecated Metal `framebufferOnly()` usage. Replaced with `isFramebufferOnly()`; vendor declaration and maintained GPU call sites checked, with no compilation by the agent. M01-03 remains awaiting lead validation. |
| Project lead continuation | M01-03 / M01-04 | Corrected M01-03 accepted by continuation; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. M01-04 started. |
| Implementation delivered | M01-04 | Paired shader tree, native shared helpers/probe stages, path/entry API, recursive compilation/deployment and manual integration probe delivered. Static checks only; M01-05 linked pending lead acceptance. |
| Project lead continuation | M01-04 / M01-05 | M01-04 accepted by continuation; compilation/checks assumed successful under the agreed convention, backend/deployment coverage unspecified. M01-05 started. |
| Implementation delivered | M01-05 | Non-template exclusive renderer ownership, retained scene, queued synchronized replacement, target generations, scene submission/timing counters and manual lifecycle fixture delivered. Static checks complete; M01-06 linked pending acceptance. |
| Project lead continuation | M01-05 / M01-06 | M01-05 accepted by continuation; compilation/checks assumed successful under the agreed convention, coverage unspecified. M01-06 started. |
| Implementation delivered | M01-06 | Independent CPU/GPU draw connector, materials/textures and tracing clones, explicit component modes, bounds/camera/cache integration and manual resource fixture delivered. Narrow decoder ownership and sampler settings support included. Static checks complete; M01-07 linked pending acceptance. |
| Naming correction delivered | M01-06 | Renamed the new draw Drawable backend resource API/internals to `DrawMesh`, `drawMesh()`, `drawMaterial()`, `buildDrawMesh()` and `_drawMesh`; updated its fixture, handoff docs and downstream plan references. Legacy production accessors remain unchanged. M01-06 remains awaiting lead validation. |
| Project lead continuation | M01-06 / M01-07 | Lead corrected compilation issues and reports the code is correct. M01-06 accepted; M01-07 started. Backend/scenario coverage unspecified. |
| Implementation delivered | M01-07 | Graphics-free document/reference IO, built-in schema parsers, strict factories and draw-only scene/BG2/OBJ resource loading delivered. Full draw resource export remains M06/M07; static checks only, M01-08 linked pending acceptance. |
| Project lead continuation | M01-07 / M01-08 | M01-07 accepted under the agreed continuation convention; compilation/checks assumed successful, coverage unspecified. M01-08 started. |
| Implementation delivered | M01-08 | Public environment/IBL/BRDF/skybox services, paired shaders, immutable replacement snapshots, scene sky intent and example 06 integration delivered. Static checks only; M01-09 linked pending acceptance. |
| Project lead continuation | M01-08 / M01-09 | M01-08 accepted by continuation; compilation/checks assumed successful, coverage unspecified. M01-09 started and resumed after an interrupted inspection turn. |
| Implementation delivered | M01-09 | Shared raster/query extraction, stable node/submesh and frame IDs, slot-safe bindings/six-texture shader ABI, paired diagnostic and structural viewport adapter delivered. Static checks only; M01-10 linked pending acceptance. |
| Project lead continuation | M01-09 / M01-10 | M01-09 accepted by continuation instruction; compilation/checks assumed successful, backend/scenario coverage unspecified. M01-10 started. |
| Implementation delivered | M01-10 | Concrete injectable deferred raster graph, sorted transparent composites, single HDR final correction/reconstruction, exact D16/D32 copies and manual fixture delivered. Static checks only; M01-11 linked pending acceptance. |
| Lead corrections reviewed / continuation | M01-10 / M01-11 | Lead reports successful compilation after renaming Metal irradianceImage and adding CommandBuffer.hpp. Reviewed identical native indices/sample math and preserved both behavior-preserving fixes. M01-10 accepted, M01-11 started; runtime coverage unspecified. |
| Implementation delivered | M01-11 | Compute cutout-aware visibility, generic opacity overrides, dense submesh UV/alpha lookup, per-light softness, acquired-slot lifetimes, unshadowed fallback and manual checks delivered. Static checks only; M01-12 linked pending acceptance. |

| [M01-17](milestone_01_deferred_renderer/step_17_documentation.md) | `accepted` | Baseline 4d58ce60f568; maintained doc/ API/tutorial/app/conceptual/navigation updates, documentation.md and review handoff. | Actual declarations/callers, resource/status/ownership/codec/FSR limits, local paths and scoped whitespace checked. No build/runtime. | Lead explicitly waived separate documentation validation and authorized immediate M02-01 after completion. | Cumulative M01 closure; transparency deferral remains visible. Future authoring owners retained in documentation.md. |
| [M02-01](milestone_02_manipulation/step_01_dual-manipulation.md) | `accepted` | Baseline 4d58ce60f568 plus this turn's M01 docs; manipulation tool modes/draw overloads, typed weak selection, Selectable schema/identity updates, instance-owned draw gizmos/weak safe callbacks, overlay mode, CPU manual fixture and actual contract report. | Includes/signatures/callers/schema/lifetimes/identity/backend boundaries, delimiters, local links and scoped whitespace. No compilation/shaders/runtime. | Accepted by lead continuation; lead reports checks through this state on macOS Metal and Linux Vulkan. | See dual-manipulation.md and tests/manipulation/dual_mode. Draw picking/overlay recording remain M02-02/M02-03; M02-02 is next after acceptance. No CMake edits. Concurrent unrelated UI edits preserved. |
| [M02-02](milestone_02_manipulation/step_02_picking-readback.md) | `accepted` | Baseline 7b217bcb6d9a; SelectionManager draw picking/input, DrawPicking.cpp, paired picking shaders and manual checklist. | Signatures/identity/lifetimes, RGBA8/push ABI, pass load/store, completion/orientation/discovery, GPL/style and whitespace inspected. No builds/runtime. | Accepted by project lead continuation; backend coverage unspecified. | See picking.md; M02-03 follows acceptance. No GPU or CMake changes. |
| [M02-03](milestone_02_manipulation/step_03_gizmos-overlays.md) | `accepted` | Baseline 7b217bcb6d9a plus preserved M02-02; public OverlayPass/MeshOverlayPass, RendererConfiguration/RendererDeferred lifecycle, draw GizmoAndSelectionRenderer snapshots/edge cache, paired shaders and lead checklist. | Signatures/includes/callers, acquired-slot ownership/partial cleanup, shader ABI, depth/color ordering, topology, dependency/GPL/style/whitespace and links inspected. No compilation/runtime. | Accepted by project lead continuation; backend coverage unspecified. | See overlays.md; optional output-sized unjittered extension, geometric scene depth and separate transform depth. M02-04 follows acceptance. No CMake changes. |
| [M02-04](milestone_02_manipulation/step_04_material-modifier.md) | `accepted` | Baseline 7b217bcb6d9a with preserved M02-02/03; draw MaterialModifier overloads, Drawable materialDefinition seam, shared alphaCutoff decoder, CPU fixture/checklist. | Include/signature/caller/setter mapping, independent targets, texture/cache/completion/callback policy, source GPL/style/whitespace and local links inspected. No builds/tests executed. | Accepted by project lead continuation; backend coverage unspecified. | See material-modifier.md; application queues redraw/history reset even on partial failure. M02-05 follows acceptance; no CMake changes. |
| [M02-05](milestone_02_manipulation/step_05_widget-adaptation.md) | `accepted` | Baseline 7b217bcb6d9a plus preserved M02-02–04; separate DrawMaterialEditor/DrawTextureWidgets, TexturePreview/session registry, shared Texture GPU snapshots, backend generation/registration helpers in GPU and decision/checklist docs. | Includes/callers/API/setter mapping, queued weak/generation identities and error invalidation, retained preview resources/retirement/backend generations, new native-free public headers/GPU dependencies, GPL/style/whitespace/links inspected. No build/runtime. | Accepted by project lead continuation instruction; backend coverage not specified. | See widgets.md and tests/ui/draw_material. Scene inspector/bake widgets retain later owners. M02-06 follows; no CMake changes. |
| [M02-06](milestone_02_manipulation/step_06_viewpoint-utils.md) | `accepted` | Baseline 7b217bcb6d9a plus preserved M02-02–05; new utils analyzer/types, camera overload, GPU typed D32 readback in both native backends, actual API/checklists and M06/M07 integration guidance. Production render analyzer and camera overload remain unchanged. | Compared analyzer algorithms/shader ABI and render queue/bounds contracts; inspected D32 format, usage, layout, completion and Vulkan/Metal readback implementations; checked dependencies, candidate/selection map separation and source formatting. `git diff --check`. No build/shader/runtime execution. | Accepted by project lead continuation instruction; backend coverage not specified. | See viewpoint-analysis.md and tests/manipulation/viewpoint. M02-07 follows; no CMake changes in M02-06. |
| [M02-07](milestone_02_manipulation/step_07_working-example.md) | `accepted` | Preserved existing uncommitted M01/M02 work and delivered `examples/draw/11_manipulation`, registered as `draw_manipulation`. Includes draw selection/gizmo overlay, input routing, draw material editing and viewpoint action; later corrections removed CLI options, use engine wrappers, fix 70% render/pick scale, keep camera components on one node, use editor-style mouse mapping, preserve camera projection and remove temporary debug traces. | Static source review and documentation mapping; no build/runtime run recorded by this agent. | Project lead requested M02-09 after reporting the camera/manipulation issue solved. Vulkan runtime validation remains outstanding; no other backend result is inferred. | See example, [M02 review](milestone_02_manipulation/review.md) and tutorial. SDL remains behind engine wrappers. |
| [M02-08](milestone_02_manipulation/step_08_review.md) | `accepted` | Added [review.md](milestone_02_manipulation/review.md) and updated affected M03–M10 pending summaries/steps with actual draw manipulation, edit notification, preview, viewpoint, identity and lifecycle contracts. Subsequent corrections are recorded chronologically in review.md. | Static source/plan inspection; no build/shader/runtime execution. | Accepted by project-lead continuation to proceed with M02-09. This does not certify Vulkan runtime behavior. | See [M02 review](milestone_02_manipulation/review.md). |
| [M02-09](milestone_02_manipulation/step_09_documentation.md) | `accepted` | Added cumulative references for manipulation and utils, draw UI wrappers, event forwarding, tutorial #11, app guidance and navigation; removed camera trace code from the example. | Static link/anchor/header/source/target checks and `git diff --check`; no build, shader compilation or runtime/backend check. | Accepted by lead continuation on 2026-10-16; lead explicitly reports manipulation working on macOS Metal and Linux Vulkan. | See [documentation handoff](milestone_02_manipulation/documentation.md). M02 gate closed; M03-01 authorized by the same continuation. |
| [M03-01](milestone_03_ray_tracing/step_01_rt-design-checkpoint.md) | `accepted` | Baseline `15f67dff9efad51c878740bc1ddbce94f880ed46`, clean entry; uncommitted design and pending-plan updates only. | GPU native pipeline/resource/query and draw material/identity comparison; native primary docs, scoped links and `git diff --check`; no build/runtime execution. | M02 accepted on explicit macOS Metal/Linux Vulkan report and continuation; M03-01 accepted by project lead continuation. | See rt-design.md. Design accepted by continuation to M03-02; source baseline findings retained in rt-design.md. |
| [M03-02](milestone_03_ray_tracing/step_02_rt-gpu-data.md) | `accepted` | Baseline `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved uncommitted M03-01 work. GPU Common/Device/ResourceSet/ShaderLib/RT headers and native implementations; primitive paired shader profiles and manual fixture; API and plan handoffs. Uncommitted. | Signatures/callers/native API names, bindings/packing/profile discovery, optional SBT layout/flush/cleanup, array ownership/limits and acceleration ordering, licenses/delimiters/GPU dependencies, scoped links and `git diff --check`. No compile/shader/runtime execution. | M03-01 accepted by lead continuation; M03-02 accepted by project lead continuation; compilation/checks assumed successful, backend/scenario coverage unspecified. | See rt-gpu-data.md and tests/gpu/advanced_tracing/README.md. No CMake/production rendering edits. M03-03 authorized by project-lead continuation. |
| [M03-03](milestone_03_ray_tracing/step_03_blue-noise-utils.md) | `accepted` | Baseline `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved M03-01/02 edits; new utils/BlueNoise header/source and umbrella export, paired noise helpers/probe kernels, manual fixture and API/plan documentation. Uncommitted. | CPU/native metadata/sequence/bindings, upload/readback/retained owners and replacement paths, embedded asset shape/histograms, source licenses/includes/delimiters, scoped links and git diff --check. No compilation or CPU/GPU test execution. | M03-02 accepted by continuation; M03-03 lead-reported GLSL sampler-constructor failure corrected in source, awaiting recompile/validation. | See blue-noise.md and tests/utils/blue_noise/README.md. Logical-layer atlas, no physical image-array extension; production noise and CMake untouched. Next M03-04 pending acceptance. |
| [M04-02](milestone_04_lightmaps/step_02_uv2-surface.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved M04-01 work; public UV mesh/pass/reload, safe draw app callback, six native shaders, manual fixture and API/plan handoffs. Uncommitted. | Seventeen source licenses/includes/delimiters, declarations/definitions, 64/128-byte layouts, native bindings, depthless MRT/coverage ordering, reload ownership, documentation links and git diff --check. No compilation/runtime. | Accepted by project lead continuation instruction; backend/scenario coverage not specified. | See uv-surfaces.md; four MRT outputs plus derived R8 coverage. Scheduling/accumulation/preview remain subsequent steps. |
| [M04-03](milestone_04_lightmaps/step_03_integrated-context.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved M04-01/02 work; IntegratedBakerContext/IntegratedLightmapBaker and LightmapBakeExecutor, manual lifecycle fixture, maintained API and pending-step handoffs. Uncommitted. | Five new source GPL/include/style/delimiter checks; public definitions, exact completion counter, frame/source/target guards, retained partial-record owners, selected native pass/profile and copied environment ownership; seven pages/62 local links; earlier bake checks and git diff --check. No compile/runtime. | Accepted by project lead continuation instruction; backend/scenario coverage not specified. | See integrated-context.md. One live target/in-flight bake frame per context; raw samples only. M04-04 implements accumulation/composition/dilation/results. |
| [M04-04](milestone_04_lightmaps/step_04_accumulation-output.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved earlier M04 work; public accumulator/composition, ten native shader files, integrated commit/publication/typed cache, output fixtures and maintained/pending API handoffs. Uncommitted. | Twenty-two source license/include/control/delimiter checks; five native binding profiles/16-byte uniform layout; valid counts/commit/publication and typed codec checks; eleven pages/93 links; 6080 script-level nearest-seed/reference comparisons; previous bake checks and git diff --check. No compilation/native fixture/runtime. | Accepted by project lead continuation instruction; backend/scenario coverage unspecified. | See accumulation-output.md. Full-float output and conservative half-float GI saturation rejection; final synchronous validation/readback. Next M04-05 preview/UI. |
| [M04-05](milestone_04_lightmaps/step_05_uv-preview.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved earlier M04 work; bake UV diagnostic renderer, BakeLightmapPreview UI bridge and API handoff. Uncommitted. | GPL/header, API/ownership, resolution/rebind and mesh nonmutation source inspection; `git diff --check`; no build/runtime/UI/GPU execution. | Accepted by project lead continuation instruction; backend/scenario coverage unspecified. | CPU-rasterized UV1/UV2 diagnostic (submesh fill/wireframe and atlas boundary) uploaded as sampled RGBA8 via gpu::Image; completed lightmap images bind directly into draw UI. Next M04-06 working example. |
| [M04-06](milestone_04_lightmaps/step_06_working-example.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved earlier M04 work; draw_lightmaps example/target, explicit indirect material metadata and JSON, HDR G-buffer and 16 native shader pairs, UI filtering fallback and API/manual guide. Uncommitted. | 41 source/license/delimiter checks; 16 shader-pair and flag/composition checks; 9000 scalar algebra comparisons; four pages/27 local links; git diff --check. No compilation/shader tools/app/GPU execution. | Accepted including shader deployment correction by project lead continuation; backend/scenario coverage unspecified. | Existing reserved baked output now samples linear RGB UV2. Synchronous per-sample submission in prepareFrame proves completion before completeFrame; no render-loop API extension. All targets finish before adoption. Correction: missing bake shader compilation/bundle/install tree fixed in lib/CMakeLists.txt and cmake/utils.cmake; static path/dependency checks only, lead rebuild required. Next M04-07 technical review. |
| [M04-07](milestone_04_lightmaps/step_07_review.md) | `accepted` | HEAD `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved M04 working tree; review.md, root decisions/index, twenty pending-step requirements, six future summaries and M04-08 handoff. Markdown only. | Actual source/application/shader/deployment contracts, 91 owned source style inventory, local Markdown links and git diff --check; no build/runtime execution. | Accepted by project lead continuation; backend/scenario coverage unspecified. | Records intentional CPU UV diagnostic and AO-slot material interpretation, HDR ranges, serial completion and future export/headless obligations. Next M04-08 documentation; milestone remains open. |

## M01-01 handoff

- Actual contracts and remaining device/shader validation are documented in [contracts.md](milestone_01_deferred_renderer/contracts.md). Renderer and pass configuration is live object state; no preferences model, concrete deferred implementation or production migration was added.
- Initialization modes are specified for subsequent shared-object steps; no mode fields were added to unrelated namespaces.
- `skills.md` was not found in the repository or its nearby parent directories. Architecture decisions, source anchors and current draw/GPU headers were inspected instead.
- Lead checks: compile existing library/draw targets and public headers on available backends; verify compatible custom-layer dispatch and rejection of incompatible bindings. No new executable at this step.


## M01-02 handoff

- Actual API and compatibility rules: [GPU rendering primitives](milestone_01_deferred_renderer/gpu-rendering-primitives.md). Primitive descriptions use no scene/render naming and introduce no higher-level dependencies.
- `RenderingDescription` supplies ordered colors and optional depth, typed clears and load/store control; `GraphicsPipelineDescription` supplies per-target state and optional independent depth state. Vulkan/Metal implement these while retaining prior overloads.
- Physical format/limit queries and logical enabled ray-query/tracing capabilities are distinct. Metal aggregate tile costs and new-family image limits are conservative; native creation is the final validator.
- [Manual MRT probe](../../../tests/gpu/mrt_probe/README.md) includes a standalone runner and paired shader sources. The lead compiles/links it with GPU-example settings and checks available Vulkan/Metal, existing triangle/cubemap behavior and unsupported configurations. No CMake edits or execution by the agent.
- M01-03 must extend the new attachment path to views/subresources. Its step file now records that dependency. M01-11 also records query-only versus raster-stage capability selection. New deferred rendering, FSR and M03 RT design remain scheduled.

## M01-03 handoff

- Actual API/lifetimes/synchronization: [GPU subresources](milestone_01_deferred_renderer/gpu-subresources.md). `ImageSubresourceRange` and borrowed `ImageView` are shared by attachment targeting, resource bindings and transfers. Native views belong to their image.
- Whole-image layout queries reject mixed state; ranged queries and transitions preserve independent mip/face contents. Command recording follows the existing ordered-submission model; callers coordinate producers and recover resources if partially recorded work is abandoned.
- `Image::upload` and packed `readPixels` complete synchronously; command copies and generation can be batched. Filtered multi-level generation is capability-checked; one-level images only transition. Integer/depth generation and depth transfers remain explicitly bounded as documented.
- Ordinary 2D mip counts are honored, sampler LOD reaches the chain, and example 06 generates the HDR source chain where supported, with nearest/single-level fallbacks. Authored specular mip levels remain independent.
- The [manual probe](../../../tests/gpu/subresources/README.md) is called by the existing MRT runner and needs no new shaders. Lead checks: compile the library (including new gpu/Image.cpp), existing draw examples and manual runner; run available Vulkan/Metal checks, inspect example 06 LOD behavior and report failures. No agent build or runtime execution.
- M01-06/M01-08 now reference the actual upload/view/mip APIs and independent-clone/authored-IBL requirements. M05 readback/export records the existing packed color readback baseline. Future region/depth/asynchronous readback and array textures extend these same contracts.

## M01-04 handoff

- Started after project lead continuation accepted corrected M01-03. Existing uncommitted work is preserved.

- Actual loading/build/deployment contracts and lead checks: [shader-libraries.md](milestone_01_deferred_renderer/shader-libraries.md). The selected step explicitly includes CMake integration; no build/configuration/install/packaging command was executed.
- Shared native math/PBR/color helpers and distinct same-basename `probe` libraries establish deployment only. M01-08/10/11/13 now reference these contracts and still supply concrete renderer stages.
- `ShaderLib::stagePath` and `entryPoint` expose the actual naming without GPU creation. Existing production paths and historical example compile helpers remain separate. Standalone creation copies nested native shader libraries.
- Manual probe: optional engine shader directory in the MRT runner; creates graphics/compute pipelines, draws both libraries and checks odd-sized readback. Lead checks include development/installed/bundled/standalone roots and shared-include-only rebuilds. No agent execution.

## M01-05 handoff

- Actual lifecycle, failure and ownership contracts: [runtime-renderer.md](milestone_01_deferred_renderer/runtime-renderer.md). Non-template DefaultRenderLoopDelegate accepts a unique renderer, keeps the shared scene, initializes at the first valid resolved surface and queues renderer/scene changes for prepareFrame before recording. Application/MainLoop selection is preserved.
- Settings attach/detach hooks and rendererGeneration/withRenderer invalidate delayed actions before old resources are released. Callers use weak delegate ownership; MainLoop safe-update cancellation alone does not validate raw old property targets. Resize and scene changes also rebind targets. No preferences implementation yet.
- FrameContext appends sceneFrameNumber and sceneDeltaSeconds; existing frameNumber/deltaSeconds retain presentation semantics. Scene numbering advances after scene submission, not UI-only presentation. Continuous rendering requests are explicit; dirty state survives pause and callbacks during recording.
- [Manual lifecycle probe](../../../tests/draw/renderer_lifecycle/README.md) requires a visible initialized window Engine before any other loop/UI initialization. It checks deferred replacement from UI preparation, retained scene identity, stale generation rejection, pause/resize/resume, animation and cleanup ordering. It has not been compiled/run or registered in CMake.
- M01-12/13/15, M03-07 and M05-02 now reference the actual hooks/timing/resize policy. M01-05's exit check was refined for its real prerequisites: RendererDeferred and the actual settings facade arrive later, so M01-15 retains their full replacement demonstration.
- Lead checks: compile bg2e and ordinary draw examples, run the fixture on available Vulkan/Metal, check real UI/minimize/resize/format changes, background throttling, initialize/attach/resize failures and shutdown. No agent build/runtime execution or CMake changes.

## M01-06 handoff

- Actual resource ownership/access/edit/clone contracts: [drawable-resources.md](milestone_01_deferred_renderer/drawable-resources.md). `draw::Drawable` uses the standard geo::MeshPNUUT/gpu::MeshPNUUT layout and holds live MaterialBase attributes after loading. It owns CPU metadata, raster buffers and optional tracing geometry; per-frame bindings remain scheduled in M01-09.
- Backend-realized resources in the new draw API use the `draw` prefix: `DrawMesh`, `drawMesh()` and `drawMaterial()`. Legacy production names remain unchanged while both APIs coexist.
- Clones duplicate CPU data/settings and actual GPU raster/material/texture/sampler realizations, copying every authored mip/layer while restoring source layouts. Built tracing geometry rebuilds against cloned buffers even if future tracing was disabled. Explicit clone bypasses mutable cache sharing. Procedural settings snapshot pixels because arbitrary generators have no clone API.
- DrawableComponent has typed draw construction/setters/drawDrawable/executionMode/clearDrawable. Wrong-mode or loaded cross-context replacement is explicit. Shared bounds/camera/clone paths handle both modes. M01-07 now adds explicit reference serialization and draw-aware loading; full resource export remains M06/M07 and must not silently omit GPU-only data.
- TextureCache adds separate weak draw entries with device identity, normalized paths and exact sampler/color settings. GPU samplers append LOD bias and clamp-to-border support; Metal rejects unsupported nonzero bias on older runtimes. Existing db::loadImage now adapts STB buffers to base::Image's new[] ownership before freeing decoded storage.
- [Manual resource probe](../../../tests/draw/drawable_resources/README.md) checks two-submesh clone/edit independence, all material images/samplers, authored mips, built/no-RT paths, component mode/clone and transformed/visible bounds. It has not been compiled/run or registered in CMake.
- M01-07/09/10, M02-04 and M04-02 reference actual resource/clone/edit APIs. Project lead checks: compile bg2e and existing production/draw examples, run the probe on available Vulkan/Metal and check file/HDR/procedural reloads, cache/context differences, cubemap/mixed-layout copies and old/new scene behavior. No agent build/runtime execution or CMake changes.


## M01-07 handoff

- Accepted M01-06 after the lead reports correcting compilation issues and confirms the code is correct. Exact fixes/backend coverage are unspecified; current source and earlier uncommitted work are preserved.
- Actual contracts and lead checks: [db-levels.md](milestone_01_deferred_renderer/db-levels.md). readSceneDocument/writeSceneDocument preserve raw unknown JSON; engine-free loadScene and Node/Component parsers construct known CPU schema with unresolved references. saveSceneReferences never exports or copies resource contents.
- Scene/Node draw Engine reference overloads and Node/Component::loadResources realize draw BG2/OBJ mesh/material resources. Production factories/loaders remain distinct; legacy name and explicit path references are supported. Source references clone/rebase without opening assets; same-context repeated realization preserves live resources and wrong-mode/context errors are explicit.
- New factory routes call DefaultCreator then the graphics-free virtual parser, plus loadResources for draw. Unsupported custom schemas fail rather than falling back to production or dropping components. All registered scene built-ins have pure schema/reference hooks; SelectableComponent's hook and draw update remain explicitly scheduled in M02-01.
- EnvironmentComponent retains its HDR reference without draw upload until M01-08. Complete draw save, glTF/MTL import, GPU-only export and full joint/resource round-trip remain M06/M07; current high-level production save rejects unresolved/draw resources. This does not relabel reference IO as complete save.
- [Manual DB probe](../../../tests/draw/db_levels/README.md) checks absent-resource graph/reference round trips, unknown raw fields/components, lexical path rebasing, reference clones, draw-only realization and repeated same-context loading. Not compiled or run. Lead checks: library/public overloads, production/draw examples, real textured BG2 and geometry OBJ on available Vulkan/Metal, custom factories and IO/context failure paths.
- Updated M01-08/15, M02-01, M06-02, M07-02 and M08-02 with actual APIs/remaining responsibilities. No CMake or GPU implementation changes, configuration, compilation or runtime execution in this step.


## M01-08 handoff

- Actual APIs/ownership/shader ABI and lead checks: [environment-tools.md](milestone_01_deferred_renderer/environment-tools.md). Public generation classes derive from CubemapRenderer/EnvironmentPass, with settings snapshotted at initialization and synchronous generate returning shared images. EnvironmentGenerator publishes complete validated snapshots after successful generation; previous snapshots survive errors and cleanup of service references.
- Empty file source intentionally generates procedural sky; missing/SDR HDR sources fail explicitly. File upload/source mips and sampling are capability-checked; filtering can use nearest, source mip chain can use one level. No RT requirement. Authored specular levels are rendered independently and never regenerated.
- Linear HDR sky/specular radiance, PI-inclusive diffuse irradiance, GGX perceptual roughness m/(levels-1), and split-sum BRDF RG=A/B at (NdotV, roughness) are the shared shader contract. Skybox preserves foreground depth, pushes camera/exposure per draw and applies blur through the specular chain. All generated sampled subresources finish ShaderReadOnly.
- Scene EnvironmentComponent exposes normalized blur/visibility intent and pure reference serialization. EnvironmentGenerator::synchronize must run before active frame/command recording; M01-10 wires source changes/defaults and rebinding into RendererDeferred. Blur/visibility-only changes share GPU images. Explicit build reloads a changed file/configuration at the same path.
- Example 06 now uses the public services and draws a skybox while retaining its reflection-map/LOD controls. Its old app generation shaders remain historical assets. Existing recursive paired shader integration discovers the new engine shaders; no CMake edits were made.
- [Manual environment probe](../../../tests/draw/environment/README.md) checks all constant-color faces/mips, energy/PI, finite LUT, failed HDR replacement, default sky, blur snapshots and retained ownership. Not compiled or run. Lead checks include library/example 06, paired native shaders/deployment, asymmetric HDR axes/seams, skybox depth/rotation/resize, invalid contexts and no-RT/unsupported-format cases on available Vulkan/Metal.
- Updated M01-09/10/12/15, M03 reflections, M04 bake contracts and M08 standalone context to consume actual public services and color/lifetime contracts. No GPU backend or production render implementation migration. No configuration, compilation, shader compilation or runtime execution by the agent.


## M01-09 handoff

- Actual APIs, binding table, native shader packing, lifetimes and lead checks: [bindings-visitors.md](milestone_01_deferred_renderer/bindings-visitors.md). RenderQueueVisitor publishes complete snapshots with node/drawable/local transforms, visibility, packed materials, lights and first enabled environment intent; failure resets traversal and leaves the previous queue intact.
- RenderQueue keeps canonical one-based 24-bit object IDs separately from opaque/transparent/solid-transparent order. Transparent categories stable-sort back to front. Stable nodeId/submesh keys support delayed selection/history; IDs may change next frame. New draw visitors reject production/unresolved/unsupported layouts explicitly and never mutate Drawable transforms or invoke scene native drawing callbacks.
- CollectRayTracingInstancesVisitor consumes the same snapshot/world matrices and retains mesh/BLAS/textures with exact submesh ranges. Default excludes both transparent categories but keeps opaque cutouts for M01-11. No-RT yields no instances. populate only replaces same-device CPU instance lists; callers retain collections/TLAS resources through submission completion and implement alpha-aware GPU tables later.
- ShaderData sizes: Frame 240, Material 192, Object 336, Light 80, LightUniforms 656, Environment 16 bytes. Six slots retain all actual scale/UV/channel/inversion/cutoff/color fields. Manual sRGB decode avoids native double decoding; AO presence is distinct from baked-lightmap semantics. See the shared paired draw/lib/bindings headers and exact four-set native index table.
- Frame/Object/Light/EnvironmentDataBinding prepare once per acquired presentation slot and retain the matching resources. Mutable UBOs/sets are slot-local; geometry/textures are shared. Direct Texture reload/updateSampler invalidates captured realization pointers and requires fresh extraction/rebinding. No per-frame waitIdle/upload/submission in preparation. Explicit cleanup/lifecycle is a stopped boundary and Engine/device outlive all resource owners.
- ResizeViewportVisitor now uses pure math::Viewport plus a constrained structural width/height template; native/generic extent source calls remain supported without a render/native header dependency. Legacy production visitors/bindings and scene Update/Input/Transform remain intact. Existing binaries require rebuilding the changed overload.
- [Manual draw/binding probes](../../../tests/draw/bindings/README.md) cover nested extraction, identity/classification/sort, hidden/disabled branches, recovery and optional ray agreement, plus paired shader consumption of all four sets/material/environment slots. Not compiled or executed. Lead validates library and existing production/draw examples, available Vulkan/Metal, acquired slot isolation, texture/UV/channel/color readback, viewport callers and error/replacement paths.
- Updated M01-10/11, M02 picking, M03 RT GPU data and M04 integrated bake instructions to the actual identity/binding/color conventions. No CMake or GPU backend changes; no compilation/configuration/shader compilation/runtime execution by the agent.


## M01-10 handoff

- Accepted M01-09 by project lead continuation; coverage unspecified. Existing uncommitted planning/code and lead fixes remain preserved. Actual API, image/native binding tables, ownership and checks: [raster-layers.md](milestone_01_deferred_renderer/raster-layers.md).
- RendererDeferred supplies ordered public GBuffer/DeferredLayer/FinalLayer/SkyboxPass stages. Configuration replaces exact compatible stage contracts; unknown/duplicate/incompatible selections fail. The final stage can be an unrelated compatible RenderLayer; only the optional finalSettings accessor requires FinalLayer. Explicit camera or first enabled scene camera is supported; shared scene update/viewport behavior remains intact.
- Six G-buffer colors plus working/preserved opaque depth and two linear HDR colors are acquired-slot owned. Both transparent categories merge far to near and each item records its own G-buffer plus deferred fullscreen composite. Copied opaque depth rejects hidden glass; cleared normals distinguish absent coverage from copied depth. Fullscreen descriptors are invocation-local and slot-retained. Shared geometry/textures are not rings.
- Native paired shaders consume all material UV/channel/scale/inversion/color fields, normal maps, alpha cutoff, unlit/emission, PBR lights, sheen, Fresnel tint, refraction and actual IBL conventions. The baked RGB channel stays white. No query/AO/GI/reflection/history/denoise objects exist as placeholders. FinalLayer applies exponential exposure (stops), gamma, brightness/contrast only once after HDR composition and explicitly reconstructs bilinear output at a separate extent. Implemented debug views0..7 show the last processed G-buffer layer.
- Renderer::prepareFrame defaults to no-op and the non-template delegate invokes it before ordinary acquisition for environment synchronization. Initialization/scene attachment also synchronize while stopped before recording; failed source replacement retains previous resources and exposes environmentError. Animation-driven source edits are seen next prepare. Fixed configuration.environment bypasses generation. SkyboxPass now explicitly exposes camera matrices. EnvironmentDataBinding optionally selects Metal texture base8 for fullscreen (default6 preserves M01-09).
- Required GPU extension: exact matching D16/D32 depth-only subresource copies now pass generic validation and Vulkan chooses DEPTH aspect; Metal uses its existing exact texture copy encoder. Stencil/scaled depth copy/upload/readback are not added. RenderLoop retained scene color gains Sampled usage for validated final output transitions. No new native implementation outside gpu, no CMake or production render implementation changes.
- [Manual deferred fixture/checks](../../../tests/draw/deferred/README.md) include an application-owned counting GBuffer, red opaque/green occluded glass/blue front glass pixel expectation, texture/cutout/PBR/IBL/refraction, native shaders, exact depth copies, slots/resize/replacement/scene/environment and UNORM/sRGB/reduced output cases on available Vulkan/Metal/no-RT. Not compiled or executed by the agent.
- Pending M01-11/12/13/15, M02 picking and M04 integrated bake now reference the actual graph, native indices, copied depth, setting owners and final processing. Project lead validates compilation/native shaders and runtime behavior before continuation. M01-11 is the next step; no further implementation step started.


## M01-11 handoff

- Reviewed the lead's M01-10 fixes: irradianceImage retains texture9/sample9 and unchanged sample/lighting math; explicit CommandBuffer include supplies RenderTarget's type. Both corrections are preserved, including the new shadow shader variant. The lead reports compilation success; M01-10 accepted with runtime/backend coverage unspecified.
- Actual API/layout/limits/lifetime/lead checks: [query-shadows.md](milestone_01_deferred_renderer/query-shadows.md). Public SoftShadowPass is a real compute RenderLayer with live SoftShadowSettings and available/status. RendererConfiguration.softShadows injects it; RendererDeferred exposes softShadows/usesQueryShadows and chooses compatible deferred.shadows.v1 only after successful capability/kernel creation. Unsupported/default paths keep unshadowed PBR; explicit plain deferred.v1 injection opts that graph out.
- Generic RayTracingMeshDescription.opaque and RayTracingOpacity Geometry/ForceOpaque/ForceNonOpaque map to both native backends. Cutout instances override already-built opaque BLAS to expose candidates; ordinary instances keep opaque fast paths. GLSL/Metal accept only triangles passing actual albedo alpha/cutoff/UV/scale/sampler lookup and continue through holes. No tracing pipeline/SBT/intersection table or render-stage ray-query capability is required.
- Minimal table is dense TLAS order with original canonical objectId retained, exact submesh firstIndex-resolved UV0/UV1 triples and material alpha fields. Eight distinct image/sampler pairs are statically bound; a ninth throws rather than truncating or silently casting solid shadows. Explicit LOD0 samples keep alpha linear and native samplers intact. This UV/alpha-only mechanism deliberately does not settle M03 full material/geometry addressing.
- Per-light castShadows/sourceSize/shadowSamples drive deterministic disk-sampled visibility, capped by an explicit1..64 sample budget (default16); zero light samples means one hard ray. Two RGBA16-float masks map eight direct lights. Each opaque/transparent receiver G-buffer computes visibility before its deferred composite; IBL/emission/unlit/final processing remain separate. Both transparent classes remain excluded as default occluders.
- Acquired slots retain collection/TLAS/mapped query buffers and every invocation's set. TLAS builds once per active slot frame via GPU commands and is reused across transparent invocations. No normal global idle/immediate submission; disabled/no-light/empty scenes need no TLAS or Drawable tracing geometry. Vulkan mapped uniform/storage writes now flush non-coherent allocations. Native API changes stay in gpu; no CMake or production render implementation changes.
- [Manual fixtures/checks](../../../tests/draw/soft_shadows/README.md) cover sparse canonical IDs versus dense lookup, submesh firstIndex/UV1/scales/cutoff, actual alpha holes/continued traversal, source-size/sample changes, transparent receivers, no-RT/forced-disabled fallback, fixed table limit, native opacity/shader indices and slots/resize/replacement. The earlier deferred pixel probe remains applicable. Not compiled or executed by the agent.
- Pending M01-12/15, M03-01/02 and M04 integrated bake now reference the actual settings/status/query table/native opacity and limits. Project lead validates native compilation and runtime before continuation. M01-12 is next; no following step started.


Project lead handoff: M01-12 delivered; awaiting compilation and applicable checks. The linked successor is M01-13.


## M01-12 delivery notes

- Accepted M01-11 by the lead continuation instruction, with compilation/checks assumed successful and backend coverage unspecified; all previous uncommitted changes and lead fixes preserved.
- Actual settings contract: [reflected-settings.md](milestone_01_deferred_renderer/reflected-settings.md). Catalogs hold only live targets/copied metadata/unique generations. Selected object accessors/settings remain authoritative; custom renderer/layer/upscaler targets append through ExtendCatalog.
- Exact codec schema/version/component/type/property identities; numeric strings preserve uint32/double/int64 enum values through JsonNode's float storage, strict parsing handles escaping/Unicode and rejects malformed data. Defaults reject unsupported properties; explicit skip reports diagnostics. Whole-batch validation precedes writes; setter rollback failures are reported.
- Optional borrowed Preferences stores ASCII-hex codec text. Explicit restore documents bypass persisted defaults, missing properties retain live values, and nothing loads implicitly on bind. CPU codec needs no window/UI/device.
- Weak lifecycle callbacks clear before cleanup/rebind after attachment. Queued edits validate renderer/catalog generations at the pre-acquire boundary after GPU drain and request scene frames. Environment generation edits mark rebuild intent; subsequent resource failure retains the old published environment and reports environmentError.
- Manual CPU fixture plus persistence/deferred replacement/resize/availability checks: [README](../../../tests/draw/settings_codec/README.md). Text-only source checks and git diff --check pass; compilation/execution is delegated to the lead.
- Pending M01-13/14/15/16, M05 settings and architecture notes now reference actual extension, queue, wire and persistence contracts. M01-13 is next after acceptance; no following step started.


## M01-12 type-validation warning correction

The lead reported ignored-result warnings for type-only any_cast expressions in SettingsCatalog::validateValue. These expressions checked exact types by throwing bad_any_cast, but discarded values and unnecessarily copied strings/paths. They now use an explicit requireType helper with the pointer any_cast overload and throw bad_any_cast on null, preserving the validation contract without copies or ignored-result warnings. False and empty string/path values are valid. The manual CPU fixture now covers Bool/String/Path and both Resource storage types, wrong exact types and empty any. Text-only source/whitespace checks passed; no compilation or fixture execution. Corrected M01-12 remains awaiting project lead validation; M01-13 is still the conditional successor.


## M01-13 handoff

- Accepted M01-12, including its type-validation warning correction, by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend coverage unspecified. Baseline b955833f314d and all earlier uncommitted work/lead fixes preserved.
- Actual APIs/ownership/native ABI/color/state/reset/fallback limits and lead checks: [upscaling.md](milestone_01_deferred_renderer/upscaling.md). Public open Upscaler injection through RendererConfiguration, implementation-owned live reflection metadata and stable identifiers; no closed engine enum. Manual ApplicationUpscaler composes the public resampler and exposes live bypass/record count without engine registration.
- ResolutionSettings enabled/scale is authoritative on both Metal/Vulkan and independent of FSR availability. Default retains original fixed/native behavior; enabled floor(output*scale) supports [0.25,1]. The delegate preserves original fixed extent, resolves policy before acquisition, detaches/rebinds settings generations around extent/needsRebuild changes and preserves live final color settings.
- Reconstructed RGBA16F HDR is acquired-slot/output-sized before the single final tone/color operation. RendererDeferred final injection uses finalContract(output, depth, true), shader identity draw.raster.final.reconstructed.v1; seven debug sources stay internal-sized. Standalone final.v1 remains supported. GLSL debug sampling now chooses the actual source extent.
- FsrUpscaler owns enabled/sharpness plus SDK/backend/hardware/status reporting. All SDK headers/native casts/handlers are in gpu::vk::FidelityFXBridge. Context/shared resources use the ordered graphics stream, with four SDK queued frames maximum; motion/output/descriptors are acquired-slot owned. Native SDK unregister restores registered states before generic output sampling. Partial initialization cleanup and bilinear failure paths are explicit. No normal per-frame idle/readback/immediate submission.
- SceneFrameNumber drives jitter; motion maps jittered clip to previous/current unjittered clip and writes previous-minus-current framebuffer pixels, using preserved opaque depth and the engine negative-height Vulkan viewport. Attach/resize/replacement/projection/discontinuity/explicit camera-cut resets are supported. Finite conventional RH perspective/zero-to-one depth only; identity/orthographic use unjittered fallback. Object motion/reactive masks remain M03 and glass/moving objects can ghost under FSR.
- [Manual fixture/checks](../../../tests/draw/upscaling/README.md) cover native/reduced/odd/sRGB/debug output, scale fixed-extent restoration, FSR transitions/status/reset/fallback, slots/native states, external injection/reflection/generations and earlier deferred/production regressions. Static include/delimiter/shader-entry/dependency checks and git diff --check passed. No C++/shader compilation, executable/runtime checks or CMake edits by the agent; lead owns validation.
- Updated M01-14/15/16, M03 temporal, M05 settings, M06/M07 application integration, M08 standalone settings and architecture decisions to the actual contracts. M01-14 is next after acceptance; no successor implementation started.


## M01-14 handoff

- Accepted M01-13 by project lead continuation instruction; compilation/checks assumed successful under the agreed convention, backend/platform coverage unspecified. Existing uncommitted changes and lead fixes are preserved.
- Actual window/editor/facade APIs, ownership, file/persistence behavior and scope: [settings-ui.md](milestone_01_deferred_renderer/settings-ui.md). GraphicsSettingsWindow binds a shared live facade, enumerates fresh descriptors and reads getters every frame; no renderer pointers or copied configuration. Generation-scoped native editor IDs refresh on rebind; availability refreshes without requiring generation changes.
- ReflectionWidget::editValue replaces original getter/setter accessors with transient proxies, never follows raw object pointers, preserves exact uint32/double/sparse int64 enum and String/Path/resource types. Existing reflection object/action UI and production RenderSettingsWindow remain unchanged.
- UI/programmatic queueEdit/importFile use facade queueSet/queueImport. Strict SettingsCodec file IO and app FileDialog wrappers preserve exact wire values. Export/persist read live getters and are blocked while this window has pending edits. New facade hasPersistence/queueRestore exposes optional safe queued restoration; no implicit save/restore occurs. Preferences durability reporting remains the production service's log-only behavior.
- Pending edits disable controls; completion diagnostics use weak window state, surviving renderer replacement safely and ignoring destroyed/rebound windows. No graph/resource changes occur directly during UI draw. Unsupported controls can be hidden/shown disabled while real capability/status properties remain visible.
- The reference example 17 has no TextureWidgets preview. Minimal HDR source intent uses existing ResourcePicker; legacy image/material/picking widgets remain separately scheduled in M02/M06. New public interfaces have no native UI/Vulkan types, and no native texture registration/upload path was added.
- [Manual integration/checks](../../../tests/draw/settings_ui/README.md) cover live getter agreement, queued edits/imports, numeric/path extremes, generations/availability/expired callbacks, optional persistence/strict files, HDR failure retention and production coexistence. Static includes/delimiters/native-boundary/link checks and git diff --check pass. No configuration, compilation, shader compilation, executable/runtime execution or CMake edits by the agent.
- Updated M01-15/16, M02 widget adaptation, M03 reflected effect integration, M06/M07 editor integration and milestone summary to actual APIs/scope. M01-15 is next after acceptance; no successor implementation started.


## M01-14 lead correction accepted

The lead reported that Window lacked a virtual destructor while GraphicsSettingsWindow declared an override, and added `virtual ~Window() = default;`. The declaration is verified in ui/Window.hpp and preserved. This correctly supports deletion through the polymorphic base. M01-14 accepted by project lead continuation instruction after that correction; runtime/backend coverage unspecified. M01-15 is in progress.


## M01-15 handoff

- Reviewed/preserved the lead's virtual ~Window() = default correction. M01-14 accepted by project lead continuation after that fix; other backend/runtime coverage unspecified. Starting revision b955833f314d and all earlier uncommitted work/lead changes preserved.
- Actual product/API/ownership/CLI/validation: [working-example.md](milestone_01_deferred_renderer/working-example.md). New target draw_deferred_renderer at examples/draw/10_deferred_renderer uses native-free MainLoop/EngineConfig, the non-template delegate, RendererDeferred and existing engine paired shaders/assets. Explicit step target registration is the only new CMake scope; helper/production targets are unchanged.
- Textured two-submesh OBJ, nested opaque original/independent tinted clone, ordinary and solid glass, floor/HDR, scene perspective camera and four rotating lights retain the reference's supported behavior within the eight-light limit. Alpha-checker canopy and exact packed-channel/UV1 cube supply controllable query/material probes. Live scalar material edits queue at safe boundaries and reset SDK history. Clone initialization checks independent CPU/GPU mesh/material/texture/image/sampler owners.
- Default .67 resolution, bilinear/optional FSR/application resampler and injected CountingGBuffer demonstrate actual extensions without engine registration. GraphicsSettingsWindow reads the connected facade. Startup explicit settings file bypasses requested saved defaults; import/restore issues are reported. No-RT fixture disables geometry/shadow intent without falsifying device capability. Pause/refresh/scene/UI counters use retained scene timing.
- Same-type renderer replacement retains scene/final/shadow/resolution owners and live final settings. Stale-edit probe queues an old exposure command and replacement, expecting generation rejection. Resource release occurs before engine cleanup; callbacks use weak state. Existing production example17 remains unchanged.
- Integration correction: FinalLayer accepts reconstructedColor=false as an optional final constructor argument; pass true for output-domain HDR injection before structural configuration validation. Standalone default usage remains source-compatible; lead rebuilds library/callers.
- [Lead checks](../../../examples/draw/10_deferred_renderer/README.md) cover available native backends/shaders/assets, actual textures/glass/IBL, clone independence, packed channels/UV/scales/inversion, cutout query continuation/no-RT, scale/FSR/status/reset, external pass/upscaler, strict file/persistence, generation replacement, pause/resize/cleanup and production regressions. Includes/delimiters/assets/native boundary/target/docs/whitespace checks pass statically. No configuration, compilation, shader compilation, launch/runtime/image verification by agent.
- Updated M01-16, M02 product derivation, M03 existing-target integration, M04/M05 scene baseline, milestone summary and actual upscaling constructor handoff. M01-16 is the next required separate code-versus-remaining-plan review, conditional on lead acceptance. No review or next milestone implementation started.


## M01-16 review handoff

- Accepted M01-15 by project lead continuation instruction; compilation/checks assumed successful under the agreed convention. Backend/device/scenario coverage is not specified and no new evidence is invented. Current source revision remains b955833f314d; M01 implementation and inherited modifications remain uncommitted.
- Delivered [review.md](milestone_01_deferred_renderer/review.md) with actual public contracts/ownership, CPU/native ABI and resource states, planned-versus-actual classifications, reference-product scope, verification ledger and impact table for every pending milestone.
- Reviewed actual delegate/setRenderer/target generations, typed scene accessors/cloning/cache, raw/reference versus full DB IO, rendering/subresources/typed color transfer, query opacity/dense mapping/eight-pair capacity, environment snapshots, live settings/numeric-string codec, FSR/UI and accepted product callers. Preserved all lead corrections, including virtual Window destructor.
- Updated all nine M02–M10 summaries and concrete owning steps. M02-01 must fix production-only Selectable update and pure schema hooks; M02-04 needs explicit CPU-definition/live patch access because loaded material() aliases live state. M02 overlays/M03 effects need minimal public borrowed surface/camera contracts. M03 arrays are absent; arbitrary hit-data design stays at its explicit lead-owned checkpoint. M04 UV2 is texCoord1/selector1, not a third channel. M05 Engine is still window-only; raw color readback is reusable but depth/conversion/exact averaging are distinct work.
- Complete glTF/MTL/high-level resource save/GPU export/editor/async/batch work remains in its assigned M06–M08 steps. M09 names actual coexistence/native/widget/template/probe/shader artifacts; M10 distinguishes maintained doc/ from historical plan records. No plan requirement was silently marked implemented or retired.
- Static source assertions, all relative path links under the plan (including report/source references), and git diff --check pass. No configuration, compilation, shader compilation, executable/runtime/image/package checks, CMake edits or source mutations by this review. Manual probes/counters are not relabeled as agent-observed results.
- M01-16 awaits lead review/acceptance. Next required step is M02-01; a continuation accepts this review/milestone under the agreed convention. No manipulation implementation or M03 design selection was started.


## M01-15 runtime failure reported

The lead reports draw_deferred_renderer exits with "Query cutouts support eight distinct albedo image/sampler pairs; no occluders are truncated". Reopened M01-15 despite prior continuation acceptance; M01-16 review remains pending and M02 is blocked by this correction. Source cause: geo::createCube has six face submeshes; example canopy/channel setup configured only material(0), leaving ten other faces with default positive alpha cutoff and independent fallback albedo realizations. The table correctly rejects excess distinct pairs. Fix the example material domains rather than increasing shader capacity or dropping occluders.


## M01-15 cutout overflow correction delivered

- Cause verified in geo/cube.cpp: six contiguous face submeshes, while example canopy/channel setup applied only to slot0. Positive default alphaCutoff made remaining fallback albedo realizations consume query alpha slots. Floor/channel slot0 also kept the default cutoff.
- Example-local singleMaterialCube preserves every vertex/index and covers the whole buffer with one submesh for each uniform-material fixture. Canopy checker/cutoff now covers every face; floor and packed cube explicitly use cutoff0. Original/clone two-submesh OBJ materials retain cutoff0, transparent models remain excluded from default occluders, and the default scene needs one cutout pair.
- Eight-pair GPU/shader capacity and explicit overflow rejection are unchanged. No occluder, alpha test, shadow feature or cloning independence is removed. No CMake, engine, shader or general geo primitive edits were made.
- Updated example checklist, actual product handoff and review with reported failure, correction and acceptance hold. Static range/material/identity reasoning, source include/delimiter/whitespace checks performed; no compilation or execution. Lead rebuilds draw_deferred_renderer and checks default query shadows, every canopy/channel cube face, cutoff/visibility/renderer replacements and no-RT fallback.
- M01-15 correction awaits lead validation. M01-16 is the reconciled successor to resume after acceptance; no M02 work is authorized by this failure report. Prior acceptance history is retained.


## M01-15 black Metal geometry investigation

The lead reports black geometry with a visible HDR sky; disabling cast shadows does not change the result. No runtime reproduction was performed by the agent. Static inspection does not yet establish the complete cause. [Investigation and diagnostic procedure](milestone_01_deferred_renderer/lighting-investigation.md) records the evidence, bounded numerical correction, and new opaque/lighting inspection controls. This failure remains open; neither M01-16 acceptance nor M02 implementation advances. The earlier overflow correction and acceptance history are preserved.


### Lead clarification: transparency isolates the failure

With Transparent models disabled and Final selected, the lead reports that the image is no longer black. Debugging only the last G-buffer layer is intentional behavior and is not classified as a bug. Investigation is now focused on transparent passes/background preservation; the numerical PBR correction is not a confirmed explanation. M01-15 remains open.


### Lead correction: opaque Final remains black

The lead clarified that Transparent models disabled plus Final still produces black geometry. The previous opposite report concerned albedo debug and is superseded. All lights' casting and global shadow generation have also been disabled without resolving it. Opaque lighting, HDR reconstruction and final processing remain candidates. Added Final Raw HDR debug to bypass color processing; no complete root cause or runtime fix is claimed. No compilation/execution performed.


### Direct contribution survives; IBL investigation

Lead images show correct direct-only output through the same reconstruction/final path, black IBL and black emission. Static review confirms the combination adds indirect and emission to direct; no final multiplication erases it. Zero emission is expected for non-emissive material intensity and is not evidence of a bad texture binding. Invalid/negative indirect data remains a hypothesis requiring Nonfinite diagnostics. Added raw IBL irradiance/prefilter/LUT and AO/baked inspection views in both lighting variants. Native index and generation/push inspection did not establish a binding mismatch; no root cause claimed. Static brace/mode/sum/whitespace checks pass; no compilation/execution. M01-15 remains open.


### HDR radiance overflow correction delivered

Lead confirmed magenta nonfinite output and valid AO/baked/BRDF LUT, with black sampled irradiance/prefilter cubemaps. Read-only RGBE inspection of autumn_field_4k.hdr found maximum163840 and31 pixels exceeding float16 maximum65504. The loader forced RGBA16 and the uploader converts overflow to infinity: a concrete invalid-radiance source before convolution. HDR file loading now selects float32 when maximum RGB magnitude times PI exceeds65000, rejects nonfinite file pixels, and propagates float32 through realized sky/irradiance/specular generation while restoring requested settings. No clamping or final-light fallback added. Details: lighting-investigation.md. Static source/whitespace checks only; no compilation/runtime execution. Lead must regenerate environment and validate finite IBL/final lighting. Transparency explicitly deferred by lead and untouched. M01-15 remains open pending validation and deferred transparency work; M01-16 held.


## HDR correction accepted; documentation scheduling revised

The lead explicitly confirms the HDR precision correction works. This validates that correction; no extra backend/scenario coverage or transparency fix is inferred. The separate transparency issue stays explicitly deferred. M01-16 remains the held technical review to reconcile; its successor is now M01-17 documentation, not M02-01.

At the lead's request, added final documentation steps M01-17, M02-09, M03-11, M04-08, M05-07, M06-08, M07-08, M08-07 and M09-08. They update maintained doc/ cumulatively through actual delivered functionality, after each technical review and before next-milestone entry. M10 retains six stable step IDs as lighter audits of existing inventory/API/tutorial/app/build/consistency and final review. The documentation-policy.md coverage matrix preserves every former M10 authoring requirement and assigns its incremental owner. Updated all summaries/review handoffs, root index, requirements and execution/review protocols. Historical delivery/acceptance statements above remain chronological evidence; revised ordering supersedes their next-step claims.

This turn changes planning Markdown only. No maintained documentation step has been implemented or accepted by this revision; no source, shaders or CMake changed. No build/runtime execution.

Planning revision checks: all milestone ordered tables match their step files, IDs are unique, M01–M09 end in their new documentation step, M10 ends in its retained final audit review, links in the new documentation steps/policy and rewritten M10 files resolve, and git diff --check passes. Documentation work itself remains not_started.


## Draw example numbering correction

The project lead requested numbered directory names for every example directly under examples/draw, excluding applications under apps/. Renamed the delivered folder to examples/draw/10_deferred_renderer and updated its CMake subdirectory registration, README links and all plan/source path references. Reconciled planned example folders: M02 manipulation is 11_manipulation, M04 lightmaps is 12_lightmaps and M05 offscreen_application is 13_offscreen_application. M03 adds features to existing numbered examples and no new example; later app milestones remain under apps/ and are not renamed. All current examples/draw directories now begin with a number. Target and executable remain draw_deferred_renderer. Static directory/reference/whitespace checks pass. No build or runtime launch under project instructions.

## M01 closure and authorized M02 entry

Lead explicitly validated M01-16, requested M01-17 and waived a separate
documentation acceptance before immediate M02-01. Delivered maintained doc/
updates and [coverage](milestone_01_deferred_renderer/documentation.md); M01-17
is accepted under that explicit direction. Source baseline 4d58ce60f568. No
compilation/runtime execution. M02-01 is now in progress. Transparency stays deferred.

## M02-01 handoff

Delivered [actual contracts](milestone_02_manipulation/dual-manipulation.md) and
[manual validation](../../../tests/manipulation/dual_mode/README.md): initialized
ToolExecutionMode, typed weak scene/resource selection, resource/count ID reset,
graphics-free Selectable schema, per-instance draw gizmo assets and safe weak
callbacks. Production signatures remain. Optional tooling attaches independently
of Renderer; draw picking and overlay recording are explicit next-step boundaries.

Static include/signature/caller/lifetime/schema/asset/delimiter/link checks and
scoped git diff --check pass. No builds, shader compilation or executable execution;
lead validates library/callers, CPU fixture and applicable Vulkan/Metal/production
interaction. Concurrent draw/UI edits were preserved and excluded from scoped checks;
whole-tree whitespace checks reported trailing blanks in those separate changes.
No CMake changes. M02-01 awaits acceptance; M02-02 is linked as successor.

## M02-02 started

Lead reports all changes checked through M02-01 on macOS Metal and Linux Vulkan and authorizes the next step. M02-01 accepted; M02-02 in progress. No agent compilation or runtime execution.

## M02-02 handoff

Delivered [picking contracts](milestone_02_manipulation/picking.md) and
[lead checks](../../../tests/manipulation/picking/README.md). Synchronous RGBA8
geometry/type/transform picking uses retained canonical IDs and weak selection
identity, unjittered top-left viewport coordinates and explicit DPI scale.
Metal-compatible transform depth restart preserves color. Typed handle input
and draw empty clearing are integrated. No GPU/CMake/native changes, compilation,
shader compilation or runtime execution. Static checks only; lead validates both
backends and production regressions. M02-03 is the successor after acceptance.

## M02-03 started

M02-02 accepted by project lead continuation. No additional backend coverage
reported. M02-03 active; existing uncommitted M02-02 changes preserved.

## M02-03 handoff

Delivered [actual overlay contracts](milestone_02_manipulation/overlays.md) and
[lead checklist](../../../tests/manipulation/overlays/README.md). Public optional
RendererConfiguration.overlays and OverlayContext provide acquired-slot final
output and original camera/queue access. GizmoAndSelectionRenderer implements
the lifecycle using an injected public MeshOverlayPass; explicit edges, live
selection/material colors, type icons and last/self-occluding transform handles
are retained per slot. Output-resolution geometric scene depth is opt-in;
line/fill/blending/shaders are configurable before initialization. No new native
implementation, GPU API, CMake edit or executable was introduced.

M02-02 accepted by continuation with no extra backend evidence. Static checks
cover includes/callers/ownership, shader push/vertex ABI, rendering transitions,
GPL/source style/whitespace and local links. No build, shader compilation or
runtime execution. Existing uncommitted M02-02 changes preserved. Updated M02-07,
M05 offscreen and M06/M07 integration steps with actual extension lifecycle and
safe preparation/refresh rules. M02-03 awaits lead validation; M02-04 is next.

## M02-04 started

M02-03 accepted by lead continuation. M02-04 active; earlier uncommitted work
preserved. Agent compilation/runtime execution remains prohibited by default.

## M02-04 handoff

Delivered [draw material patch contracts](milestone_02_manipulation/material-modifier.md)
and [CPU fixture/live checklist](../../../tests/utils/material_modifier_draw/README.md).
Drawable::materialDefinition exposes stored CPU attributes independently of the
loaded material() alias. Indexed/named/group/regex draw overloads patch CPU/live
fields independently, reusing target texture/cache/callback policy. Shared decoder
now accepts alphaCutoff. No whole-material synchronization, target retention, new
renderer ownership or MaterialBase implementation change. Existing cleanup/clone
snapshot semantics and nontransactional failure/callback behavior are documented.

M02-03 accepted by continuation; no extra backend evidence supplied. Static source,
setter/ABI/caller/dependency/lifetime/whitespace/link checks only. CPU fixture is
supplied but not compiled/run; no shaders/build/runtime execution or CMake edits.
Updated M02-05/07, M03 history and M06/M07 tool integration with real CPU/live and
queued redraw/reset boundaries. Lead validates M02-04; M02-05 is the successor.

## M02-05 handoff

M02-04 accepted by project lead continuation; no extra backend results supplied.
Delivered [widget contracts/decision table](milestone_02_manipulation/widgets.md)
and [lead checklist](../../../tests/ui/draw_material/README.md). Separate
DrawMaterialEditor reads live values, queues field-only MaterialModifier patches
or explicit Clear, validates weak scene/resource identities and renderer/binding
generations, and handles error/reset/redraw at the safe boundary. Multi-selection
shows primary values and edits specified fields on valid selected submeshes.

DrawTextureWidgets uses native-free TexturePreview handles from draw UserInterface.
Retained image/sampler snapshots survive reload; orphan registrations retire after
a following-frame drain, Vulkan backend reinit advances generations and frees
tracked IDs, and UI cleanup invalidates handles/releases snapshots before backend
shutdown. Native preview helpers live in GPU with no higher-level engine dependency.
Metal uses the standard ImGui sampler for previews. File history lists names without
on-demand thumbnail uploads in UI draw. Existing production widgets remain intact.

Static source/include/caller/setter/ownership/generation/dependency/GPL/style/link
and whitespace checks pass; no compiler, shader compiler, executable or runtime
check was run. No CMake edits or new executable. M02-07/M06/M07 instructions now
use actual widgets and explicitly defer raw scene/property/UV/bake workflows.
M02-05 awaits lead validation; M02-06 viewpoint tools is next.

## M02-06 started

M02-05 accepted by project lead continuation; no additional backend coverage
reported. M02-06 active. Preserved all existing uncommitted source and plan work.

## M02-06 handoff

Delivered [draw viewpoint analysis contracts](milestone_02_manipulation/viewpoint-analysis.md)
and [manual lead checks](../../../tests/manipulation/viewpoint/README.md).
`utils::ViewpointAnalyzer` is a separate draw/GPU analyzer with candidate,
per-submesh ID and coverage/border/depth statistics. It reuses the draw picking
shaders, synchronous RenderQueueVisitor snapshots and D32 images; it never uses
ray tracing. Added typed `gpu::Image::readPixelsDepth32` for initialized D32
TransferSrc images in Vulkan and Metal. The new camera helper overload applies
orbit target/rotation/distance and perspective/optical FOV/clipping planes.
Production `render::ViewpointAnalyzer` and its camera helper overload are intact.

Static inspection checked candidate sampling/scoring and projection fitting,
draw queue/bounds/submesh mapping, 80-byte shader push ABI, D32 format/usage,
backend copy/layout/completion paths, utility dependencies, docs links and source
style; `git diff --check` passes. No build, shader compilation, test or runtime
execution was performed. Lead validates Vulkan and Metal D32 readback, expected
statistics and production regression as available. M02-06 awaits acceptance;
M02-07 is the linked successor. No CMake changes.


## M02-07 handoff

- Accepted M02-06 by project lead continuation instruction; compilation/checks
  are assumed successful under the agreed convention, with backend/scenario
  coverage unspecified.
- Added [the registered manipulation example](../../../examples/draw/11_manipulation/src/main.cpp)
  and its local executable target; `examples/CMakeLists.txt` includes the new
  subdirectory. The M01 deferred renderer example was left intact as the
  baseline.
- The scene reuses the M01 OBJ clones, glass, cutout canopy and packed UV/channel
  fixtures. Drawable objects receive draw `SelectableComponent` and
  `GizmoComponent`; a shared `GizmoAndSelectionRenderer` overlay is configured
  before renderer initialization. `SelectionManager` routes mouse gestures and
  captures active transform drags before unconsumed events reach
  `scene::InputVisitor` and the orbit camera. Ctrl state is also read from SDL's
  modifier mask at mouse boundaries so additive picking remains stable if key-up
  arrives before mouse-up.
- `DrawMaterialEditor` is bound to the draw selection and delegate. Scalar and
  texture/alpha/UV presets apply selected submesh patches; additional
  MaterialModifier buttons exercise exact group and regex matching across the
  selected drawable. The editor's safe queue updates live GPU material state,
  including texture refresh; clone sliders and side-by-side selection support
  independence checks.
- The optional best-view action queues synchronous analysis at a stopped
  renderer boundary, applies `ViewSample.camera` through the draw camera helper,
  and resets deferred upscaling history. Logical camera viewport is restored
  after renderer resize; pick scale follows internal render extent versus
  logical window size. Delete and deselect controls use queued/safe selection
  changes. `--no-rt` disables drawable tracing geometry and shadow requests.
- Static review inspected actual declarations and call sites for overlays,
  selection capture, event routing, material editor queue semantics, modifier
  group/regex behavior, camera helper, DPI scale and example resource bundling.
  Gizmo GPU resources and editor texture sessions are released while the draw
  engine/UI session is alive; clone edits update both stored and live material
  state, and discontinuous edits reset deferred history. License/style, balanced
  delimiters and scoped `git diff --check` pass.
  No CMake configure, build, shader compilation, test or runtime check was run by
  the agent. The project lead owns Vulkan/Metal compilation/runtime verification.
- M02-07 accepted by the project lead continuation instruction; compilation/checks
  are assumed successful, backend/scenario coverage unspecified. M02-08 review is
  recorded below.

## M02-08 handoff

- Accepted M02-07 by project lead continuation; compilation/checks are assumed
  successful under protocol, with backend/scenario coverage unspecified.
- Delivered the [M02 code-versus-remaining-plan review](milestone_02_manipulation/review.md).
  It records actual selection/picking identities, overlay/gizmo ownership,
  MaterialModifier/widget boundaries, viewpoint readback, clone/reload limits and
  the lead/agent verification split.
- Updated affected M03–M10 summaries and owning steps for edit/history resets,
  bake/source generations, offscreen overlay/sample policy, safe editor mutations,
  async document generations, batch identity/export, retirement inventory and
  final API/tutorial audits. M03-01 remains the project-lead-owned advanced RT
  architecture checkpoint; no M03 implementation or decision was started.
- Static checks: local Markdown links across the review and affected pages
  resolve; scoped whitespace/diff checks pass. No build, shader compilation,
  executable, test or backend runtime was run by the agent. M02-08 awaits project
  lead review acceptance; M02-09 cumulative documentation is the successor.

## M02-07 failure report after M02-08 delivery

On 2026-10-16 the project lead reported two clang compilation failures in the manipulation example: `DefaultRenderLoopDelegate` was constructed with only a configuration instead of a renderer owner, and `UiDelegate` called `weak_from_this()` without a shared-ownership base. The lead also requires example input functionality to use engine wrappers instead of direct SDL includes/calls. Correction takes precedence over M02-09; the previously delivered M02-08 review remains pending acceptance. No build was requested.

## M02-07 correction handoff — 2026-10-16

- Replaced every example SDL call/include with engine APIs. Existing
  `app::Keyboard::controlPressed()` handles Ctrl. New
  `app::MainLoop::windowSize()` reports logical dimensions before scene/UI
  initialization and after resize, resetting on normal/exceptional run exit.
  Camera viewport and internal-to-logical picking scale use this accessor.
- Fixed the base constructor by passing an owned `RendererDeferred` and the
  existing overlay configuration. Made `UiDelegate` inherit
  `enable_shared_from_this<UiDelegate>`; application `make_shared` ownership
  precedes UI initialization and the cleanup callback remains weak.
- Static checks covered actual signatures, shared/weak ownership, window-size
  initialization/resize/exception cleanup, source license/style, absence of
  direct SDL/native-window references in the example and whitespace. No CMake
  edits, configuration, compilation, shader work or runtime execution performed.
- Lead validation: compile `draw_manipulation`, exercise Ctrl-click additive
  selection, resize/DPI camera and picking alignment, and close the application
  with the material editor active. The correction and M02-08 review/addendum
  await acceptance together; M02-09 remains the successor after validation.

## M02-07 fixed-scale correction handoff — 2026-10-16

The lead clarified that the observed internal extents were zero, not the
previously reported large integers. The example queried renderer configuration
before the first surface notification and passed zero to `setPickPixelScale`.
This is an example lifecycle error; `Size2D` already initializes to zero.

- Added one fixed `RenderScale = 0.7f` constant. `makeRendererConfiguration()`
  enables resolution scaling at 70% of output dimensions; no user control changes
  it. `init()` sets the selection pixel scale once using the same constant.
- Removed `refreshPickScale()` and its per-frame configuration query. English
  comments cross-reference both initialization functions and distinguish physical
  render output from the independent logical-coordinate picking image on Retina.
- Static inspection checks the shared constant, enabled resolution policy, single
  positive picking-scale assignment, absence of the old refresh/configuration
  query and source whitespace. No compilation/runtime checks or CMake edits.
- Lead checks: start `draw_manipulation` without the zero-scale exception, then
  select/drag and resize on normal/Retina displays. Rendering stays at 70%; picking
  stays at 0.7 pixels per logical unit. M02-08 review and corrections await
  acceptance; M02-09 remains the successor.

## M02-07 single-node camera correction — 2026-10-16

The lead clarified that Transform, OrbitCameraController and CameraComponent
belong on the same node. The example now creates exactly one camera node with
all three components and its projection; the separate child camera and look-at
transform were removed. SceneState names this owner `cameraNode`, and the best
viewpoint action obtains both camera and orbit controller directly from it.
Comments explain the shared transform and application of analyzed candidates.

Static checks verified one camera component, one orbit controller and no child
camera/pivot references. No compilation/runtime checks or CMake changes. The lead
checks orbit/pan/zoom, selection and best-view framing; the correction remains
part of the M02-07/M02-08 pending validation before M02-09.

## M02-07 editor mouse controls correction — 2026-10-16

The lead clarified that the apparent mouse issue was the example's interaction
mapping and requested the editor controls: left drag rotates, right drag pans,
middle drag zooms. No shared camera/input API changes were requested or made.

Compared model_edit's direct click-pick/scene forwarding with Composer's
SelectionManager gizmo capture routing. The example now combines those patterns:
left down/move reaches the scene unless a gizmo captures it; left release picks
only when down/up differ by at most two logical pixels on each axis. Noncaptured
releases always reach the scene, including after picking, to end camera gestures.
The example performs its own tolerant pick and calls SelectionManager mouse-up
only to finish captured gizmo gestures, avoiding duplicate exact-position picks.
Ctrl additive selection is retained. Comments and visible controls describe the
mapping and tolerance. The shared SelectionManager and both apps are unchanged.

Static inspection checked button mapping, inclusive tolerance boundaries,
single pick dispatch, balanced scene release, gizmo capture and whitespace.
No compilation/runtime checks or CMake edits. Lead checks each drag mapping,
click offsets of 0/2/3 pixels, Ctrl-click and gizmo drags. Corrections remain
pending lead validation with M02-08; M02-09 is still the successor.

## M02-07 camera diagnostics — 2026-10-16

At the lead's request, added example-only `[camera-debug]` output after mouse
movement/wheel and best-viewpoint application. Logs are emitted after the normal
renderer scene update, once per scene update with pending input, so they report
the applied world transform rather than parameters waiting to be applied.
Output includes world position, normalized viewing direction (local -Z for this
example's RH projection), orbit center/pitch/yaw/distance and near/far planes.
No camera/controller/analyzer behavior was changed. Static placement and
whitespace checked; no compilation or runtime execution. The lead will perform
the diagnostic run and report conclusions before further fixes.

## M02-07 / M02-06 projection preservation correction — 2026-10-16

The lead's diagnostic trace shows that mouse rotation changes world position and
front, but best-viewpoint application changed near/far from 0.1/100 to
51.2542/104.465 while orbit distance was limited to 24. The lead clarified the
contract: applying a viewpoint must never change the scene camera's projection.

Removed perspective FOV/near/far and optical focal-length/near/far writes from
the utils::CameraCandidate overload of applyViewpointToOrbitCamera. It now changes
only orbit center, distance and rotation, using the camera node only for the
existing child-distance compensation. The production overload already preserves
projection and is unchanged. Analyzer candidate projection parameters remain
internal to analysis rendering. Updated the public helper contract, M02 viewpoint
documentation and manual verification checklist; this supersedes M02-06's earlier
projection-copy handoff description.

Static inspection verified that the draw helper contains no projection mutation
and retains its pose setters; whitespace checks passed. No compilation, shader
compilation, runtime checks or CMake changes. Lead checks Best viewpoint and the
debug log: camera near/far and FOV must remain unchanged before/after application
and mouse movement. The independent orbit-distance limit is unchanged by this
projection-only correction. M02-07 corrections/M02-08 review remain pending
validation before M02-09.

## M02-09 documentation handoff — 2026-10-16

- The project lead reports the camera/manipulation behavior is solved and
  requested the documentation step; Vulkan validation remains outstanding.
- Removed temporary `[camera-debug]` output, pending-reason state and its input
  and best-view triggers from `draw_manipulation`; interaction behavior is intact.
- Added maintained `manipulation` and `utils` API references, draw widget and
  input-routing guidance, tutorial 11, app guidance and navigation updates. The
  page/source disposition is recorded in
  [M02-09 documentation.md](milestone_02_manipulation/documentation.md).
- Static local link/anchor, public header, example source/target path and
  whitespace checks passed. No build, shader compilation or runtime test was
  run. M02-09 awaits lead acceptance; complete the Vulkan check before entering
  M03-01.

### M02-09 tutorial correction

The lead rejected the initial overview and the later complete-source listing.
Rewrote tutorial 11 as 26 explanatory steps for building the application from
an empty file. Focused snippets and concrete implementation recipes cover all
example behavior without a full-file dump. Updated incoming anchor/navigation
and the documentation handoff. Removed the example's remaining `--no-rt`
configuration per the earlier instruction; default tracing/shadow behavior is
unchanged. Static review only; no build/runtime checks. M02-09 acceptance and
the final Vulkan check remain outstanding.

## 2026-10-16 — M02 accepted; M03-01 delivered

The project lead reports completing the M02 Linux check and states that manipulation APIs work on macOS Metal and Linux Vulkan. This closes the outstanding M02 Vulkan runtime gate and accepts M02-09 by the instruction to proceed. Device models and individual runtime scenarios remain unspecified; earlier handoffs above are historical.

M03-01 started at `15f67dff9efad51c878740bc1ddbce94f880ed46`, with a clean working tree. Delivered uncommitted planning changes: `milestone_03_ray_tracing/rt-design.md`, the M03 summary/checkpoint and M03-02–10 handoffs, affected M04/M08 summaries/steps, architecture decisions and this ledger. The design compares actual GPU pipeline/resource/query code and maintained RT documentation, and proposes native Vulkan stages plus Metal compute queries, packed indexed geometry, bounded image/sampler tables, shared cutout evaluation, explicit surface contracts and tracing/history invalidation. No source, public API, shader or CMake changes.

The recommendation deliberately remains pending project-lead acceptance, including its native feature requirements, capacity errors and exclusion of blended secondary transport. Continuation to M03-02 selects it; a different choice revises the checkpoint first. No unresolved failure is reported. Static checks: inspected source/declarations/callers and production GI behavior references, native primary documentation, scoped Markdown links and `git diff --check`. No build/configuration/shader compilation/runtime check was run. Design review is the only validation required for this step.

## M03-02 implementation entry

Project lead continuation accepts the M03-01 recommendation, including native stages, bounded native resource arrays, packed geometry and deferred blended secondary transport. Entry revision remains `15f67dff9efad51c878740bc1ddbce94f880ed46`, with the uncommitted M03-01 planning changes preserved. `skills.md` is absent from the repository; current architecture.md, operative decisions and actual GPU sources are the available architecture references. No build is authorized.

## M03-02 delivery — primitive GPU tracing/data access

The lead's continuation accepted the explicit M03-01 recommendation. Delivered AnyHit and optional Vulkan hit-group stages, additional native descriptor visibility, fixed whole-array sampled image/sampler setters, explicit Metal argument-buffer grouping and native residency, enabled array capability/limit profiles, scalar-compatible validation and storage-buffer range checks. Hardened Vulkan optional SBT indexing, handle/base alignment, mapping flush/failure cleanup and dispatch limits; added BLAS→TLAS and TLAS→RT read ordering plus scene mesh/device/index checks. All new native work stays inside gpu.

The primitive fixture/shader profiles test indexed packed geometry, nonzero index/vertex bases, dense identity, UV1, divergent resources/samplers, cutout continuation and exact expected pixels, plus explicit invalid-resource/capacity cases. It is delivered for lead execution, not claimed as run. Shader discovery already recognizes all new stages; no CMake edits or new executable target. Full draw material snapshots, normal transforms, six-slot shading and scene mutation integration remain M03-04 onward; array graphics-stage support on Metal is not provided.

Static source/license/dependency/delimiter, API/caller/native binding, profile discovery and scoped Markdown link review plus `git diff --check` completed. The agent did not configure/build, compile shaders, launch tests/examples or run packaging. Lead checks: build engine/paired shaders, run manual advanced-tracing probe on Vulkan query/native pipeline and Metal compute, existing GPU RT example and draw examples 10/11/shadow fallbacks, unavailable indexing/no-RT, and native validation/slot retirement cases. Next pointer is M03-03; stop for acceptance.

## 2026-10-17 — M03-03 entry

M03-02 accepted by project lead continuation; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. M03-03 starts at `15f67dff9efad51c878740bc1ddbce94f880ed46` with inherited uncommitted M03-01/02 work preserved. No build/runtime execution authorized. `skills.md` is absent; architecture.md, operative plan and actual sources remain the architecture references.

## 2026-10-17 — M03-03 delivery

Delivered utils::BlueNoiseData (borrowed CPU ranks, validation/wrapped texels/sample2D), sampleNoise2D with explicit null-data hash alternative, and Device-backed utils::BlueNoise with synchronous build, strong failed-replacement behavior, copied shared image/sampler owners and tile/channel/layer metadata. Default ranks reuse the existing private production byte header unchanged; the utility has no render Engine/native graphics dependency. M09's manifest now explicitly relocates this pure data header before render directory deletion.

Selected layout: logical tiles stacked in a sampled/readback/upload RGBA8 UNORM 2D atlas (default 128x512), with exact channel expansion and nearest/zero-LOD sampler. No physical image-array primitives or implicit tracing-table entries are introduced. Paired draw/lib helpers use explicit tile-local texel reads, byte-rank reconstruction, bounded uint32 sequence rotations and declared hash fallback. CPU/GPU fixtures verify raw upload, samples, wrapping/layers, overflow keys, invalid metadata and retained old snapshots across replacement/cleanup when the lead runs them. GPU sampling of disabled noise binds no noise image/sampler. No advanced effect is introduced yet.

Agent static inspection confirmed original data shape/131072 bytes and balanced byte-rank histograms, SHA256 recorded in blue-noise.md; source was not regenerated and spatial diagnostics were not run. Reviewed signatures/includes/lifetimes/native binding/sequence packing, licenses/delimiters and scoped Markdown links; git diff --check passed. No configure/build, shader compiler, test executable, CPU/GPU fixture, renderer or packaging invocation. Production BlueNoise/data/shaders/scripts and CMake are untouched.

Lead validation: compile engine and paired shaders, run the manual CPU/Vulkan/Metal utility fixture with native diagnostics, test shared-owner/replacement and no-RT utility reuse, and preserve existing production compilation. Coverage for the accepted M03-02 remains unspecified. Next is M03-04 RTAO, conditional on this delivery's acceptance.

## 2026-10-17 — M03-03 compiler failure report

The lead reports glslang compilation failure for draw/blue_noise/probe.comp.glsl:36 (`sampler constructor must appear at point of use`) with Vulkan SDK 1.4.313.1, target Vulkan 1.2 on the macOS build. The probe passed a constructed combined sampler2D as a user-function argument. Reopen M03-03; fix the helper/caller contract and downstream instructions before advancing. No passing correction build is claimed.

## 2026-10-17 — M03-03 GLSL correction delivered

Changed bg2eBlueNoise2 GLSL arguments from combined sampler2D to separate texture2D/sampler. Construction now occurs directly in texelFetch, and the probe passes the separate noiseImage/noiseSampler bindings. Updated API documentation, implementation handoff and M03-04 caller guidance. Existing descriptor indices, CPU/Metal sampling and disabled-noise variant remain unchanged. Static review found the single GLSL caller and verified signature/argument consistency and point-of-use construction; git diff --check passed. No compilation or runtime execution. M03-04 remains next, conditional on the lead accepting this correction.

## M03-04 entry

The lead continuation accepts M03-03 including its GLSL sampler-constructor correction; compilation/checks assumed successful under the agreed convention, backend/scenario coverage unspecified. Entry HEAD `15f67dff9efad51c878740bc1ddbce94f880ed46` plus preserved uncommitted M03-01–03 changes. No compile/runtime execution is authorized. Public tracing snapshot/surface primitives belong in draw::pipeline; production render remains unchanged.

## 2026-10-17 — M03-04 delivery

Delivered public TracingSceneResources/TracingSnapshot with retained immutable geometry, transformed dense instances, complete MaterialData/UV0/UV1 and independent bounded native image/sampler tables. Shared cutout predicates use actual interpolated submesh UVs and nearest accepted hits. Public replaceable RTAmbientOcclusion supports camera-depth and explicit position/normal/coverage receivers, independent quality resolution, configurable samples/bounces/radius/bias/falloff/attenuation and shared BlueNoise/hash variants. Paired Vulkan/Metal kernels cover all 32/128/256 profiles. RendererDeferred integrates optional scalar AO and compatible lighting variants, retaining old shader contracts and capability fallback. Neutral output is one; direct PBR/emission/unlit and existing material AO arithmetic remain intact.

Per-slot snapshots and per-invocation resources remain retained through completion; reset/cleanup drain at stopped boundaries. No per-frame waitIdle, CMake edits or production render migration. Added a manual fixture for cutout UV changes/rear occluders, dense IDs/submesh offsets, geometry reuse, radius, disabled/noise paths and camera/explicit receiver equivalence. Documentation records exact ABI/bindings, ownership, validation requirements and downstream limitations, including the existing separate soft-shadow cutout limit.

Agent performed only nonexecuting source inspection, license/delimiter/local include checks, all 24 native profile wrapper checks, new documentation link checks and git diff --check. No configure/build, shader compiler, fixture or renderer execution. Project lead must compile engine and paired shaders, run the fixture on Vulkan/Metal with diagnostics and validate deferred/manipulation regression plus unsupported-device fallback. Next M03-05 RTGI is conditional on M03-04 acceptance.

## M03-05 entry

Lead continuation accepts M03-04 under the execution convention; backend/scenario coverage unspecified. Existing uncommitted changes are preserved. No build/runtime execution authorized.

## 2026-10-17 — M03-05 delivery

Delivered RTGlobalIllumination with public settings/accessors, compatible shader/subclass replacement, camera/explicit receivers and retained matching-capacity tracing snapshots. Vulkan uses iterative raygen plus miss/closest-hit/any-hit at recursion depth one; Metal uses compute nearest-intersection transport. Sixty native wrappers cover camera/world, hash/blue and 32/128/256 capacity. Hit transport evaluates albedo/cutout UV and scalar emission semantics, geometric normals, participating point/spot/directional lights with cutout-aware shadow samples, and environment escape/terminal ambient once per path.

HDR RGBA16 output is incoming diffuse radiance/pi before receiver albedo, with explicit alpha validity; disabled/empty/unavailable output is invalid zero. RendererDeferred integrates scaled GI targets and diffuse IBL replacement while preserving specular IBL, sheen, material AO, baked multiplier, direct PBR and emission. IBL/AO/GI selection retains component settings, suppresses inactive scene snapshots and preserves ordinary composition when GI is unsupported or disabled. Existing lighting variants remain available for explicit replacement. The shared scene tables and production render implementation are unchanged by M03-05.

Added a manual fixture for dense IDs/submesh bases/geometry reuse, emissive HDR and cutout continuation, distance versus excluded geometry, environment escape, blue noise/bounces, participating light contribution, virtual dispatch, camera/explicit agreement and disabled/empty alpha. Agent performed nonexecuting source/license/delimiter/preprocessor/include/profile/binding/API/link inspection and git diff --check only. No configure/build, shader compiler, fixture, renderer or packaging execution; no CMake edits. Lead must compile engine/native shaders and validate Vulkan/Metal, unsupported fallback and existing examples with diagnostics. Coverage for accepted M03-04 remains unspecified. M03-06 reflections is next, conditional on acceptance of this delivery.

## M03-06 entry

Lead continuation accepts M03-05 under the execution convention; backend/scenario coverage unspecified. Prior uncommitted changes are preserved. Architecture reference skills.md remains absent; architecture.md, actual sources and accepted native contracts are used. No build/runtime execution authorized.

## 2026-10-17 — M03-06 delivery

Delivered RTReflections with replaceable public configuration/lifecycle/recording, quality/sampling/roughness/bias/distance/shadow/noise accessors, camera or explicit receivers, receiver material and camera position. It reuses existing packed scene/cutout/native hit helpers. Vulkan raygen/miss/closest-hit/any-hit and Metal compute query wrappers cover twelve surface/noise/capacity profiles. Hit shading includes slot-correct albedo/emission/normal maps, inverse-transpose normals, model-transformed tangents and mirrored determinant sign, environment diffuse and participating lights with cutout-aware shadows.

Output RGBA16 RGB is unpremultiplied mean finite geometry-hit radiance before receiver BRDF; alpha is accepted hits/actualSamples. Misses/roughness rejection/empty/disabled/unavailable results preserve prefiltered specular IBL through zero confidence. RendererDeferred integrates optional independent reflections with display-relative quality, scaled lighting inputs and confidence-weighted split-sum composition. Diffuse AO/GI/IBL selection and material/direct/emission behavior remain separate. Eight new paired lighting variants retain old contracts. FullscreenPass adds an optional independent native sampler map so reflection texture16 shares nearest sampler0 within Metal fragment sampler bounds.

The manual fixture covers shared geometry/dense IDs/submesh offsets/UV cutouts, deterministic mirror emission, rear emitters through holes, distance misses, glossy partial confidence with unpremultiplied radiance, roughness rejection, shared noise, camera/explicit equivalence and a 2x2 display-relative target over a 1x1 receiver, plus disabled/empty/virtual dispatch. Documentation defines exact ABI, source tangent convention, lifetime/fallback and downstream history/confidence requirements.

Agent performed only nonexecuting source checks: 81 new source files, 60 native wrappers, eight paired lighting descriptor/native sampler maps, licenses/delimiters/preprocessor/includes, API definitions/ABI/ownership and local documentation links; git diff --check passed. No configure/build, shader compilation, fixture, renderer or packaging execution. No CMake or production render edits in this step. Lead must compile and validate Vulkan/Metal diagnostics, unsupported fallback, normal-map/mirrored/shadow/resize scenarios and existing deferred/manipulation regressions. Coverage for accepted M03-05 remains unspecified. Next M03-07 histories/temporal accumulation is conditional on acceptance of this delivery.

## M03-07 entry

Lead continuation accepts M03-06; backend/scenario coverage unspecified. Preserve existing uncommitted work. No build/runtime execution authorized. Architecture references remain architecture.md and operative plan; skills.md is absent.

## 2026-10-17 — M03-07 delivery

Delivered public TemporalAccumulator with borrowed TemporalInput/SurfaceInputs/TemporalView, separate signal/view/layer keys, actual rendered-scene sequence independent of presentation slots, reset generations and scalar/HDR native kernels. Immutable per-invocation signal/position/normal-count images and retained previous/next versions keep submitted history safe; disabled/unavailable paths return the borrowed raw signal. Histories prune stale keys and enforce a bounded capacity. Position/normal/coverage/roughness rejection preserves AO neutral visibility, GI validity and confidence-weighted unpremultiplied reflection radiance. Arithmetic means before the frame bound, rolling or frozen behavior at the bound, and nominal versus per-pixel counts are documented.

RendererDeferred injects/exposes the public component, accumulates each active effect for opaque and stable transparent submesh layers, and coordinates generic Renderer::invalidateHistories with existing resetUpscaler adapters and FSR state. Exact initialized extraction/settings signatures detect identity/material/transform/light/environment/quality/mode/noise changes before jitter preparation; explicit application notification remains necessary for in-place content/custom shaders and same-camera teleports. Duplicate persistent node/submesh correspondence is diagnosed when active. Existing tracing snapshots are freshly prepared from the same extracted queue after reset.

MotionVectorGenerator provides the public CPU camera-motion reference with previous/current unjittered transforms from current jittered clip and correct framebuffer Y. Existing FSR GPU motion and SDK jitter ownership remain unchanged. Moving-object vectors are not introduced: geometry/transform changes conservatively reset effect and SDK histories; previous-object reprojection is explicitly deferred. Same ordered queue submission is required, and abandoned recording must invalidate provisional histories. Fresh history images avoid mutable overwrite at the cost of allocations; a reuse pool is a later optimization, not an asset frame ring.

Added a manual numerical fixture for simulated presentation gaps/two retired slots, separate scalar/GI/reflection arithmetic, fractional confidence, retained outputs, rolling/frozen limits, generation and position/normal rejection, view/layer separation, camera/explicit profiles, invalid/black GI, roughness rejection and disabled pass-through, plus CPU jitter/motion reference assertions. These fixtures were not executed. Agent performed nonexecuting inspection of fifteen new source files, eight native wrappers, descriptor/ABI/ownership/sequence/reset paths, license/delimiter/preprocessor/include/API/link checks and git diff --check. No configure/build/shader compiler/runtime/packaging execution; no CMake or production render changes.

Lead must compile native shaders/engine and validate Vulkan/Metal, asynchronous ordered slots, pause/UI-only/pick/overlay chronology, edit/cut/resize/FSR resets, reprojection and confidence edges, unsupported fallback and existing examples. Coverage for accepted M03-06 remains unspecified. M03-08 spatial denoise is next, conditional on this delivery's acceptance.

## M03-08 entry

Lead continuation accepts M03-07; backend/scenario coverage unspecified. Preserve existing uncommitted work. No build/runtime execution authorized. Architecture reference remains architecture.md and operative plan; skills.md is absent.

## 2026-10-17 — M03-08 delivery

Delivered public DenoiseFilter with compatible shader/configuration/lifecycle/resource replacement, per-effect/view/layer keys and borrowed SurfaceInputs. Native camera/world x scalar/HDR compute profiles filter R16 visibility or RGBA16 radiance at the signal extent, independently of the surface extent. Bilateral weights combine spatial distance, geometry distance and normals, with hard rejection and normalized borders. Camera guides compare linear view Z; explicit guides compare world separation without a fake camera. GI validity, black valid radiance, reflection fractional confidence, unpremultiplied RGB and roughness eligibility remain distinct. Missing profiles return the borrowed input without wrong-format allocation.

RendererDeferred appends shared filter injection/exposure and resolves tracing -> temporal -> spatial -> lighting per active effect/layer. Filter outputs remain separate from temporal history and bake averages. Fresh invocation outputs/descriptors/uniforms are retained per completed slot; external result owners remain valid and no per-frame waitIdle is introduced. Virtual control getters/setters, settingsType and matching settingsInstance metadata permit live catalog enumeration of injected filters. Renderer signatures read the actual virtual getters and coordinate history/SDK reset. Built-in visible controls/preferences remain M03-09.

Added a manual fixture for actual metadata read/write and record override, scalar smoothing, constant HDR energy, odd 5x3 extents/borders, coverage, world-distance and normal discontinuities, confidence weighting, invalid GI centers, reflection roughness rejection, camera profiles, independent 3x1 signal extent, retained prior output and no-op/disabled borrowed results. Agent performed only static checks: thirteen new source files, eight paired wrappers, bindings/ABI/ownership/order, metadata definitions, licenses/delimiters/preprocessor/includes, documentation links and git diff --check. No configure/build, shader compiler, fixture, renderer or packaging execution. No CMake or production render edits in this step.

Lead must compile engine/native shaders and validate Vulkan/Metal diagnostics, partial/unsupported capability fallback, extreme controls, asynchronous completed-slot ownership, shader/subclass replacements, HDR/confidence edges and existing deferred/manipulation examples. Coverage for accepted M03-07 remains unspecified. M03-09 existing-example controls/integration is next, conditional on acceptance of this delivery.

## M03-09 entry

Lead continuation accepts M03-08; backend/scenario coverage unspecified. Preserve prior uncommitted work. No build/runtime execution authorized. Architecture references are architecture.md and operative plan; skills.md is absent.

M03-09 delivered live metadata/settingsInstance hooks for AO, GI, reflections and
temporal alongside existing denoise metadata. RenderSettingsPreferences binds all
six optional component targets, indirect selection, graph/capability/noise status,
light reflection participation and appended resolved signal debug views. Requested
values remain editable and strict-codec serializable without hardware availability.
Distance/bias pairs are validated before mutations. Queued settings reset engine/
SDK histories even after reported partial application; explicit scene/material
operations invalidate before potentially partial edits. Existing editor callback,
clone/resource identity and application loop behavior are retained.

Both existing examples inject disabled optional FSR and SMAA; ordinary resampling
remains default. The basic example exposes resolution settings without changing its
default dimensions. RendererDeferred allocates a shared BlueNoise atlas only when
needed by an available effect lacking injected noise. Legacy eight-cutout-pair
query-shadow capacity is explicitly reported: table construction still throws a
specific length_error subtype; the pass catches only this case, renders neutral
whole-scene shadow visibility and retries on the next scene frame. Advanced tables
are independent and never truncated.

Public FinalProcessor/SMAAProcessor provide optional after-color/before-overlay
processing and a device-only reusable process API. Paired edge/weights/blend/copy
shaders port the existing orthogonal SMAA 1x profile with official vendored lookup
bytes and unchanged original licenses. Fresh RG8/RGBA8/RGBA16 scratch, a 32-byte UBO,
explicit bindings/transitions and completed-slot retained ownership support odd
extents and held outputs. Missing native formats/profiles leave original color
usable. SMAA has no RT/FSR gate; disabled is a borrowed pass-through. No diagonal/
corner detection, temporal SMAA or MSAA is claimed.

Added manual metadata/no-device strict round-trip and numerical SMAA fixtures plus
an integration matrix for both examples, unsupported/partial capabilities, SDK
fallbacks, import/stale handles, edits/clone/picking regression and resource lifetime.
Agent performed only static checks: 59 touched source licenses, local includes and
lexical balance; eight native SMAA wrappers, logical/Metal bindings and ABI review;
signal diagnostics, final/overlay ordering, six live targets and authored local
links; git diff --check. No configure/build, shader compiler, fixture, executable or
packaging execution. No CMake or production render edits in this step.

Lead must compile engine/native shaders and validate Vulkan/Metal/no-RT/partial
capability behavior and the manual matrix. Source inspection is not backend proof.
M03-08 acceptance rests on continuation; exact backend/scenario coverage remains
unspecified. M03-10 review is the next required step after acceptance, followed by
M03-11 cumulative maintained documentation. Pending M04/M05 and retirement notes
now name actual final-processing/settings/ownership contracts and retained lookup
data. The complete M03 review and cumulative docs are not claimed by this step.

Final static review also checked every deferred Metal sample against declared samplers, preserving reflection texture16 sharing sampler0. No runtime claim is added.

## M03-09 correction — missing visible graphics controls

Lead screenshots and report showed only scene/manipulation windows, with no effect
controls. Source inspection confirms the prior delivery omitted example-owned
RenderSettingsPreferences::connect, GraphicsSettingsWindow::bind and draw calls.
The earlier claim that the examples already displayed shared settings was wrong.
The request to continue implements this correction, not acceptance/advancement to
M03-10. Both examples now own/connect live facades before the first renderer bind,
show/draw the settings window at startup and expose a reopen button. Separate
borrowed PreferencesStore scopes enable explicit save/restore as well as JSON.
Manipulation clears the settings facade/window at its existing cleanup boundary.

Also removed duplicate finalProcessor recording from RendererDeferred::update;
only record after final color invokes the optional processor. This corrects stale
input and duplicate-serial failure when enabling SMAA. No CMake or production
render changes. Agent checks call-site/lifecycle wiring, single final-processor
ordering, licenses/includes/delimiters and diff whitespace only. No compilation,
shader compiler or runtime execution. Lead must validate startup, reopening,
actual effect toggles/status, preferences/rebind and SMAA; M03-09 remains awaiting
project lead. Next required step after acceptance remains M03-10.

## M03-09 requested addition — manipulation lights and active GI defaults

Lead requests light property/position/orientation editing in draw_manipulation and
GI plus reflections active initially. This extends the pending integration work;
no M03-10 review or new backend validation is implied. The manipulation factory
now explicitly selects GlobalIllumination and injects enabled GI/reflection objects.
Other renderer/example defaults, FSR/SMAA defaults and capability fallbacks remain.

The Manipulation window adds per-light editors using actual catalog descriptors
and GraphicsSettingsWindow::queueEdit, preserving validation, availability, queued
renderer generation checks and SDK/engine invalidation. An example-owned facade
extension adds live local position and Euler-degrees targets per light. Node owners
retain component lifetime; rebind creates current handles. Pose changes stop parent
animation and invalidate histories; dedicated property edits queue the same pause
before the catalog mutation. Transforms preserve other TRS components. Both JSON
and preference serialization include pose targets. Omni orientation has no visual
meaning; spot/directional lights follow transformed local +Z. Current parent
rotation is retained on pause, so positions are explicitly local.

Agent checked factory defaults, metadata getter/setter signatures, target owners,
queue/edit/animation/reset ordering, source includes/licenses/delimiters and
whitespace. No build, shader compilation or runtime execution. Lead validation
includes both panels, all light types/properties, pose preservation, animated
parent behavior, import/rebind and supported/unsupported GI/reflections. M03-09
remains awaiting project lead validation; M03-10 is still the next required step.

## M03-10 technical review delivery

Latest continuation accepts M03-09, including visible-settings/SMAA corrections and
manipulation per-light local pose/properties plus active GI/reflection defaults.
Acceptance basis: project lead continuation instruction; backend/scenario coverage
not specified. Preserve earlier failure reports/screenshots and corrections as
history; they do not establish measured GI/reflection performance or parity.

Created milestone_03_ray_tracing/review.md from declarations, implementations,
native kernels and real callers. It records bounded packed material/geometry ABI,
capability profiles, cutouts/transparent exclusions, AO/GI/reflection normalization,
public camera/explicit surfaces, bounded display histories, guide-based denoise,
shared noise, live metadata/codec/edit generations and optional final SMAA.
Classifies actual refinements, legacy-shadow degradation, corrected missing UI,
intentional exclusions and verification gaps separately. No new architecture
migration or invented hardware coverage. rt-design.md links concrete final source
contracts rather than leaving proposed profiles as undocumented assumptions.

Updated nineteen actual pending step files across M03-11 and M04–M10 plus all
seven downstream milestone summaries. They schedule UV world-position/normal/
coverage and chart isolation, exact raw bake/linear HDR scene averaging, completed
slot/snapshot ownership, final-AA ordering, required-job capability errors,
example-owned pose target mapping, explicit app settings-window wiring and safe
async edits. Retirement keeps official SMAA lookup bytes/licenses and actual draw
shader deployment. Full maintained documentation still belongs to M03-11.

Static GPL scan covers 338 pipeline headers/implementations and draw shaders.
Reviewed cosmetic rules at changed source sites; corrected adjacent function
separation in SoftShadowPass.cpp, RenderTarget.cpp and GBuffer.cpp, with identical
non-whitespace tokens before/after. Focused source/includes/delimiters, paired native
bindings/declared samplers/order, Markdown links and diff whitespace checked.
No CMake edits, configure/build, shader compiler, fixture, application or packaging
execution. Native numerical/performance/asynchronous failure-path checks remain
lead-owned and unspecified except historical reports explicitly cited in review.

M03-10 is awaiting project lead. Next required step is M03-11 cumulative maintained
documentation, then its acceptance before entering M04. This review alone does not
close M03 or deliver any bake/headless application API.

## M03-11 final documentation delivery

M03-10 accepted by project lead continuation instruction; no specific backend or
scenario evidence added. Implemented final M03 documentation step with five added
and twenty-four updated maintained pages. New surface/snapshot/camera-motion,
GraphicsSettingsWindow and advanced-lighting architecture references describe
actual APIs, ownership/native profiles and exclusions. Existing effect/settings/
renderer references, two ordered tutorials, app/distribution guidance, navigation
and affected production/conceptual guides are reconciled in place. Full inventory,
retained pages and remaining M04–M10 ownership are in documentation.md; review.md
has the cumulative closure handoff. No extra M03 step is required.

Corrected stale effect/UI/history availability, deferred scale, shadow overflow
and chart-isolation claims. Actual GI mode/enabled split, manipulation light poses
and defaults, typed codec identity, partial-failure invalidation, no-RT fallback
and final SMAA resources/order are documented without future baking/headless APIs
being presented as delivered. Tutorials preserve ordered construction and focused
current constructor/metadata/editor recipes rather than a whole-file listing.

Static checks: 29 maintained pages, 344 real local links and anchors, 73 existing
public/private source header references, balanced fences, source-matching example
constructors/call sites/defaults and git diff --check. No CMake/source edits,
configure/build, native shader compilation, fixture/application, image export or
packaging execution. Actual backend numerical/performance evidence remains as
specified in review.md, not inferred from documentation checks.

M03-11 is the last ordered milestone-3 step, delivered awaiting project lead.
Continuation accepts this documentation closure and selects M04-01; M04 is not
implemented or authorized by this delivery alone. Next pointer names its real file.

## M04-01 bake contract delivery

M03-11 accepted by project lead continuation instruction; M03 closed. Backend and
scenario coverage not specified. Current baseline HEAD is
95190d6d042bb85d6e9d72153b155192dac64377; prior work is preserved, no reset/commit.

Added bg2e::bake types and public LightmapSettings with actual accessor/metadata
storage, checked scalar constraints plus coupled validation, and independent
LightmapPixels RGB8/RGB32F payload/overflow/finite-HDR validation. Added abstract
noncopyable BakerContext/LightmapBaker contracts, Device/scene/environment/pass
ownership configurations, completed-slot/serial and source/target generations,
progress/cancel/reset/result/readback semantics. Compatible public draw AO/GI/
filter owners and replaceable UV surface owner are injectable; concrete executors
are not implemented here and integrated use does not depend on M08 classes.

contracts.md records all legacy public/private responsibilities and exact world
surface, raw signal, normalized RGB multiplier, required-capability, empty GI,
chart isolation, byte exposure, row orientation and output-presence policies.
Production render remains untouched; UV2 preprocessing stays geo/app. Metadata
binds actual settings, while concrete execution must freeze/validate a job copy.
Null denoise is off; any future injected filter requires chart-safe orchestration.
Standalone/assembly/batch/preview names are forward declarations, not features.

Added a manual CPU contract fixture for live metadata/strict codec, coupled and
scalar settings rejection, typed HDR/byte payloads, overflow and missing owners.
Updated actual M04-02/03/04/05 and M08-01 handoffs and a truthful maintained bake
contract reference. No CMake or production render modification, new example target,
shader, concrete GPU bake, configure/build, fixture/app or packaging execution.
Agent static checks pass for ten new source files, eleven live metadata entries,
header/definition/export/dependency/lexical checks, links and diff whitespace.

M04-01 remains awaiting project lead compilation/link/manual validation. Next
M04-02 implements safe UV2 reload and actual explicit UV surfaces/chart coverage;
M04-03/04 implement scheduling/accumulation/results, not claimed by these interfaces.


## M04-02 UV surface and safe reload delivery

M04-01 accepted by project lead continuation; backend/scenario coverage was not
specified. Baseline remains 95190d6d042bb85d6e9d72153b155192dac64377 with earlier
uncommitted M04-01 work preserved.

Added public CPU UV surface packing and Device-backed UvSurfacePass. Position
RGBA32F, signed normal RGBA16F, integer submesh and chart outputs use four
depthless attachments (32 bytes/sample); a compute pass derives R8 coverage.
Charts connect shared source-index edges within each submesh; duplicated seam
vertices remain separate. Vulkan and Metal shader pairs share explicit bindings,
world transforms and top-row/texture-v=0 orientation. Per-invocation retained
owners support multiple targets and completed-slot reuse; diagnostic readback
decodes floats, half floats and integers explicitly. WorldPositionCoverage is
exposed through the existing public surface-input contract.

UV2 generation uses the geo modifier/validator at a stopped waitIdle boundary.
It prepares an independent drawable/mesh/material realization before publication,
preserving other source references and rebuilding available tracing geometry.
The app safe-update adapter guards cancellation, scene/target/renderer identity,
invalidates renderer histories and exposes before-change/completion hooks for
caller-owned bake generations, selection and previews. Concrete orchestration
must supply these hooks and adopt/re-extract the replacement.

Actual API/lifetimes and backend checks are in
[uv-surfaces.md](milestone_04_lightmaps/uv-surfaces.md), the maintained
[API reference](../../../doc/api/bake/UvSurfacePass.md), and the
[manual fixture](../../../tests/bake/uv_surface/README.md). The fixture covers
CPU atlas rejection/chart partitioning and GPU transforms, orientation, typed
clears, independent targets/slot reuse, safe rebuild and generated-UV rasterization.
It has not been compiled or executed.

Static source, ABI/binding, ordering, ownership and local documentation-link
checks pass, together with git diff --check. No build, shader compilation, fixture,
application, CMake, production render or GPU primitive modification. M04-02 awaits
project lead validation; M04-03 implements the concrete integrated context and
is the next linked step. Accumulation, finished lightmaps and preview are not
implemented by this surface primitive.


## M04-03 integrated execution delivery

M04-02 accepted by project lead continuation instruction; backend/scenario coverage
not specified. Baseline remains 95190d6d042bb85d6e9d72153b155192dac64377, preserving
prior uncommitted M04-01/02 work. No reset, commit or new worktree.

Implemented IntegratedBakerContext/IntegratedLightmapBaker and the reusable
LightmapBakeExecutor using the existing draw Engine, explicit environment snapshots,
UV surfaces and public full-resolution AO/GI effects. The selected effect creates
bake-owned matching-capacity tracing snapshots/TLAS/material bindings, independent
of renderer/picking resources. Public native world-space shader variants are reused;
no duplicate bake shader/shading family, GPU extension or production-render change.

Context preparation validates global serial, slot, source generation, target and
scene snapshot. Recording validates exact prepared command and one attempt per
frame. CompleteFrame follows caller-confirmed GPU completion and alone increments
completedSamples; cancellation/stale/failed work cannot advance it. Abandon requires
a discarded/reset unsubmitted command. Sources/targets are rechecked on completion;
opaque in-place changes require explicit stopped invalidation. Reset freezes live
settings and advances target generation. Environment structs retain copied image/
sampler owners, with a stopped replacement API and no second environment renderer.

One live baker and one open/in-flight bake frame per context deliberately serialize
raw sample sequencing; independent presentation remains available. Explicit cleanup
requires retirement/abandonment and drains submitted owners. Unsupported required
effects/profiles/formats and empty traceable GI fail explicitly. Chart-unsafe
denoise is rejected. All requested raw samples leave Running/composition pending,
result() empty and readPixels() unavailable: finished output is M04-04.

Actual API, execution contract, limitations and downstream consumption are in
[integrated-context.md](milestone_04_lightmaps/integrated-context.md) and the
[maintained API](../../../doc/api/bake/IntegratedBakerContext.md). Added the
[manual fixture](../../../tests/bake/integrated/README.md) for lifecycle failures,
completion, cancellation, reset/generations, retained raw samples and absent final
output, plus lead checks for real presentation/selection, edits, aborts, environment
replacement and unavailable profiles. M04-04/M04-06/M08-01 handoffs are updated.

Static checks pass for five new sources, public methods, lifecycle/ownership and
selected pass invariants, seven pages/62 local links; earlier source/UV contracts
and git diff --check also pass. No compilation, shader tools, C++ fixture, app or
GPU execution. No CMake, production render or GPU source changes. M04-03 awaits
project lead validation; next linked step is M04-04.


## M04-04 accumulated lightmap output delivery

M04-03 accepted by project lead continuation instruction; backend/scenario coverage
not specified. Baseline remains 95190d6d042bb85d6e9d72153b155192dac64377 with all
previous uncommitted M04 work preserved. No reset, commit or new worktree.

Added Device-backed UvTemporalAccumulator and LightmapCompositionPass, public
configuration/native interface prefixes and exclusive injectable output owners.
Fresh RGBA32F raw sums plus R32_UINT exact valid-update counts are recorded after
each raw effect; only completed current work commits history. The last command
normalizes AO/GI, divides GI by environment irradiance/pi, then dilates from
original chart texels using a horizontal/vertical exact nearest-seed search.
Covered interiors never change; padding copies one seed, breaks ties deterministically
and never averages charts. Radius zero is valid, outside padding is neutral white.

Integrated completeFrame now validates final coverage/charts/counts/numeric flags,
reads the identical padded RGBA32F GPU texture, caches typed RGB8/RGB32F pixels, and
publishes Complete/LightmapResult only on success. HDR float output remains
unclipped; byte GI alone applies exposure before clamp/round. Invalid/missing
covered GI, saturation, empty coverage, bad reference or float overflow fail with
no output. Public half-float GI currently saturates at 65504; the new accumulator
conservatively rejects that boundary, including exact boundary values. Final
validation/readback is synchronous and can drain other Device submissions.

Five Vulkan/Metal shader pairs implement accumulation, AO/GI composition and the
two dilation scans. No private shading estimator, GPU extension, new target,
CMake or production render change. Context-owned invocation resources retain
partial/discarded/submitted owners; reset/cancel/source generations hide stale
results. Optional chart-unsafe denoise remains explicitly rejected.

Actual contracts and verification limits:
[accumulation-output.md](milestone_04_lightmaps/accumulation-output.md),
[maintained API](../../../doc/api/bake/LightmapOutput.md), and
[manual fixtures](../../../tests/bake/output/README.md). Known-value fixtures
cover AO means/chart ties/neutral padding, GI irradiance normalization/HDR/zero
reference and CPU byte/float conversion. Updated the integrated fixture for final
publication and corrected its callback to Device::immediateSubmit's actual
CommandBuffer* signature. Fixtures were not compiled or executed. M04-05/06, M05
readback/export and M08 batch-output handoffs reflect the actual output API.

Agent static checks pass for 22 source files, public method/16-byte uniform/native
binding/ownership/publication/codec invariants, eleven pages/93 local links and
6080 script-level comparisons of the separable dilation algorithm against an
exhaustive nearest-original-seed reference. Previous source/UV/lifecycle checks
and git diff --check also pass. These are mathematical/source evidence, not native
shader or numerical/backend execution. No build, compiler, C++ fixture or app run.
M04-04 awaits project lead validation; next linked step is M04-05 preview/UI.


## M04-06 integrated working example delivery

M04-05 accepted by project lead continuation; backend/scenario coverage not
specified. The lead explicitly requested resolving the implementation gaps.
Baseline remains 95190d6d042bb85d6e9d72153b155192dac64377 with previous uncommitted
M04 work preserved. No reset, commit or new worktree.

Implemented examples/draw/12_lightmaps and its requested shared-bundle target
registration. Static scene reuses DefaultRenderLoopDelegate/RendererDeferred,
public environment resources, safe UV2 rebuild and IntegratedBakerContext. Floor,
white cube and independent red clone bake sequentially from immutable source
snapshots. One synchronous Device::immediateSubmit per prepareFrame returns after
GPU completion, then completeFrame commits exact progress. No general completion
hook or GPU backend modification was necessary. Cancellation/restart/settings
edits occur between completed samples; failures leave ordinary scene display
usable. All targets' GPU/typed CPU results are captured before bake context
cleanup and application of materials. UV and result previews retain/rebind image
owners and clear before UI/Engine teardown.

The previously reserved bakedMultiplier output existed but always wrote one.
Explicit MaterialAttributes::IndirectTextureMode now marks the AO slot as scalar
AO, baked AO or baked GI, with linear RGB/UV2/opaque validation and serialized
metadata. Both raster shaders write its RGB to an RGBA16F attachment (32 total
color bytes), avoiding UNORM clamping above one. All 16 paired deferred variants
apply marked maps, bypass live AO/GI per marked pixel and preserve direct light,
emission and GI specular IBL/reflection contributions. Scalar AO remains the
default. The example disables live indirect and reflection passes globally,
updates live/stored submesh metadata and supports before/after map toggling.

The half-float display map must remain finite in [0,65504]; the example rejects
overflow instead of clipping it. Bake RGBA32F and CPU RGB32F remain full precision.
CPU RGB8/exposure selection does not quantize the GPU map used for display.
The six-slot material supports a baked map or original scalar AO in that slot;
it does not support both concurrently. GPU-only source settings are not a saved
texture: full export/materialization remains M06/M07/M08 work. The example keeps
source materials and regenerates a fresh unbaked scene before every new job.

[Example/manual verification](../../../examples/draw/12_lightmaps/README.md),
[material API](../../../doc/api/draw/BakedIndirectMaps.md) and the step handoff
record actual contracts and limitations. Agent checks: 41 source/header/shader
license and balanced-delimiter inspections, 16 paired shader profiles and flag
paths, 9000 scalar composition algebra comparisons, four Markdown pages/27 local
links and git diff --check. These are source/mathematical checks only. No build,
CMake configure, shader compilation, C++ fixture, application or GPU execution.
Vulkan/Metal/no-RT and original draw regression checks remain with the lead.
M04-06 awaits validation; next linked step is M04-07 technical review, followed by
M04-08 cumulative documentation. The milestone is not yet closed.


### M04-06 correction — missing bake shader libraries

The lead reported Metal failing to load bake/uv_surface/surface.vert.metallib.
The source prefix is correctly spelled uv_surface, but the engine CMake invoked
compile_paired_shader_tree only for draw. Bake sources therefore produced no
compiled output. Incremental bundle refresh likewise copied only draw, and
RuntimeShaders installation omitted bake. The earlier claim that recursive engine
shader deployment already covered bake was incorrect.

Registered the bake source/output tree, made per-tree shader targets and Metal
intermediate directories distinct, and retained the existing paired-shader
aggregate dependency for engine/application builds. Both native trees now copy
into app bundles on shader-only rebuilds and install through RuntimeShaders.
Static checks cover all eight bake stage pairs, expected surface.vert.metallib
path, distinct output/target/intermediate names, aggregate dependency and copies.
No CMake configure, build, compiler or application execution by the agent. The
lead must reload CMake and rebuild draw_lightmaps, then retry the bake. M04-06
remains awaiting validation; M04-07 remains the next conditional step.


## M04-07 technical review delivery (2026-10-18)

The lead continuation accepts M04-06 including its bake shader compilation,
bundle refresh and RuntimeShaders installation correction; later backend/scenario
coverage remains unspecified. Review starting/finishing HEAD is unchanged at
95190d6d042bb85d6e9d72153b155192dac64377 with prior uncommitted code preserved.

Created [M04 review](milestone_04_lightmaps/review.md) from actual public headers,
implementations, shader packing/signal/output paths, example scheduling/adoption
and corrected native deployment. It records implemented contracts, equivalent
scheduling refinements, intentional CPU preview/single indirect-slot changes,
precision/capability limits and evidence gaps. GPU resource ownership and typed
pixels do not imply exported files. Window-only Engine initialization, camera
image averaging, complete texture/model export and standalone/batch scheduling
remain assigned to M05/M06/M07/M08. Production removal stays gated at M09.

Updated twenty concrete pending M05–M10 steps, all six future summaries, root
index/architecture decisions and the M04 summary/documentation handoff. Source
style inventory inspected 91 current changed/new owned C/C++/shader files with
no reported GPL/control/function-separation findings; private source includes,
namespace layout and API grouping were reviewed with the existing source baseline.
Static links and diff whitespace checked. No compilation, shader tools, CMake
configuration, fixture, app, package or GPU run. No backend-specific numeric,
performance or exported artifact is invented from continuation acceptance.

M04-07 awaits lead acceptance. Next is M04-08 cumulative documentation and its
coverage inventory; M04 is not closed and M05 has not started.


## M04-08 cumulative documentation delivery (2026-10-18)

M04-07 accepted by project lead continuation instruction; backend/scenario coverage
not specified. Current starting HEAD is bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d,
with a clean initial working tree. The existing M04 canonical references and full
ordered tutorial are already present at that revision and are retained honestly.

Added public raw-executor/BakeSample reference and integrated draw lightmap
architecture. Updated context completion recipe, canonical navigation, production
versus draw offscreen/writer boundaries, stored/live material adoption and paired
shader packaging guidance. The coverage inventory in M04 documentation.md maps
added/updated/retained pages to source, examples, checks and later owners. Existing
17 tutorial blocks match shipped main.cpp modulo whitespace. Production guides
remain supported and separate; no standalone/batch/export or camera average is
claimed as implemented. No source/shader/CMake changes or build/runtime execution.

Static checks across 34 pages/505 local links and anchors, fences, public
API/caller/deployment inspection and diff whitespace pass. These are source checks, not native backend/numeric/performance
coverage. M04-08 is delivered awaiting project lead acceptance. Continuation closes
M04 and selects the actual linked M05-01; this turn implements no M05 work.


M04-08 cumulative documentation accepted by project lead continuation instruction
when they requested the next step; backend/scenario coverage unspecified. M04 is
closed. M05-01 is now active; no build/runtime checks are claimed for this step.

M05-01 delivered awaiting project lead validation; backend/scenario coverage is
unspecified. The new draw delegate contract carries no Vulkan types, and the typed
init overload selects experimental draw after positive extent validation. M05-02
implements actual Engine/surface initialization and run callbacks; its current run
path reports that staged dependency. Production offscreen behavior retains its
existing delegate and Vulkan implementation.


## M05-02 headless execution delivery (2026-10-18)

M05-01 accepted by project lead continuation instruction; backend/scenario
coverage not specified. The implementation was resumed after an intentional turn
interruption, preserving its partial work and all earlier uncommitted changes.
Starting HEAD remains bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d. No commit/reset,
new worktree or CMake/GPU/production-render/shader modifications.

Engine now initializes an actual offscreen Instance/surface through the same
context/cleanup machinery. It retains the window accessor, adds generic/typed
headless access and nonzero frameSlotCount, and adapts deferred/data-binding/query
resources to generic slots. A second use of a shared active Instance is rejected
without cleaning its owner. Window RenderLoop/UI reject headless initialization.

The typed OffscreenApplication overload now creates and initializes draw Engine
and the delegate. It performs serial acquired-slot recording/immediateSubmit,
then endFrame/counter/completion callback only after confirmed GPU completion.
Color is TransferSrc for completed readback; it never uses Present layout.
DefaultOffscreenApplicationDelegate bridges the public existing renderer owner,
scene/replacement generations, stopped preparation and one renderer update/record
per scene frame. Default continuation requests one frame; subclasses can request
more. Overlays are excluded; first delta is zero and later frames use the fixed
finite nonnegative timeStepSeconds (zero freezes time). No hidden SDL/UI/input.

Initialization/run failures drain retained owners before delegate cleanup and
Engine teardown, retain the first exception, and never report failed completion.
Unrun draw contexts also clean up on destruction; reentrant calls are rejected.
No extra private execution class or duplicated scene traversal was introduced.

Added a manual probe for raw known-value GPU clears/readback, zero frames,
partial init/record/completion/cleanup failure, original-exception preservation,
unrun destruction and shared Instance guards. Optional real deferred empty-scene
mode checks actual native shader/resource integration. None was compiled or run.
The source/style/include/lifecycle and Markdown link checks are static evidence;
Vulkan/Metal/no-RT/SDK, scene numerics and window/production regressions stay
lead-owned. Touched implementation function separators were corrected as required.

Actual APIs/limits and lead matrix are in
[headless-execution.md](milestone_05_offscreen/headless-execution.md) and
[manual validation](../../../tests/draw/offscreen/README.md). Updated maintained
Engine/config/frame/app references and M05-03/04/M08-01 concrete requirements.
The driver still runs ordinary per-frame final processing; exact linear HDR
averaging with final tone/color/AA once is explicitly M05-03 and cannot be inferred
from N callbacks. No scene CLI, encoded image or standalone bake product delivered.

M05-02 awaits project lead validation. The next real linked step is M05-03 under
the normal continuation convention; no M05-03 implementation is started here.


## M05-03 exact scene accumulation delivery (2026-10-18)

M05-02 accepted by project lead continuation instruction; compilation/checks
assumed successful under the agreed convention, backend/scenario coverage not
specified. Starting HEAD remains bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d;
inherited uncommitted M04/M05 files preserved. No commits or CMake changes made.

Implemented [actual settings and accumulation](milestone_05_offscreen/accumulation-settings.md):
strict file parsing before allocation, stopped direct live catalog import before
samples, opt-in persistence bypassed by explicit documents, explicit fixed/scale
overrides, unknown-ID rejection or explicit reported skip/catalog extension.
The default job freezes configuration at initial prepareFrame during init.
Public Renderer linear/final recording is optional for ordinary custom renderers;
RendererDeferred implements it with matching slot/serial and duplicate guards.
The public FrameAccumulator uses synchronous completed RGBA16F readback and CPU
double RGB sums, resolves an equal N-sample mean and uploads RGBA16F once. Alpha
is opaque; nonfinite samples reject without partial contribution. This is a
reference implementation with readback/CPU cost, not a GPU-throughput claim.

DefaultOffscreenApplicationDelegate counts config.frames completed scene samples,
resets effect/FSR histories before zero, includes every requested sample without
hidden warm-up, and records tone/color/optional SMAA only on the resolved average.
The app's extra final-only submission is not a scene update/sample. Internal
completion hooks cannot be bypassed by overriding didRenderFrame. hasResult is
false in intermediate callbacks and true only after final GPU completion; export
must happen in the last callback before surface teardown. Early stopping rejects.
Zero timestep also enforces stable non-jittered scene/camera/history identity;
nonzero timestep explicitly averages an animation sequence with fixed settings.
In-place GPU edits must invalidate histories or be serialized outside the job.
No overlays or undocumented private target access are used.

Authored manual fixtures cover known 1/3 mean 2, N=1, signed HDR, resets,
nonfinite/count/dimension rejection, half transfer round trips and a real GPU
0.25/0.75 mean versus a single 0.5 final output, exact counts and final-once result
publication at reduced internal scale. They have not been compiled or executed.
Agent static checks inspected 24 sources, licenses, 160 includes, lexical structure,
call signatures, native transfer usage, completion/cleanup ordering and local
documentation links; git diff --check passed. No configure/build/shader/runtime.
Lead checks remain the fixture READMEs, actual UI import/unknown lightPose and
resolution precedence, frozen mutation failure, animation timestep, available
Vulkan/Metal, no-RT/FSR-unavailable, and ordinary windowed/production regressions.

Maintained app/renderer docs and actual M05-04/M05-05/M08-01 handoffs updated.
M05-04 and M05-05 were accepted by continuation. M05-06 reviews the actual API,
output and evidence limits and updates affected M06–M10 steps/summaries. No build,
runtime or image inspection was performed. M05-06 awaits project lead validation;
next linked step is M05-07 cumulative documentation after acceptance/continuation.


## M05-03 compilation correction (2026-10-18)

The project lead reported that Apple Clang could not find glm::packHalf2x16 and
glm::unpackHalf2x16 while compiling FrameAccumulator.cpp. Inspection of the actual
Vulkan SDK 1.4.313.1 GLM headers confirms these functions are declared in
<glm/packing.hpp>; <glm/gtc/packing.hpp> only references them in documentation.
Replaced the include in the implementation and the manual frame_accumulation
fixture. No arithmetic, API or CMake changes. Declaration/call inspection and
git diff --check performed; no compilation or fixture execution by the agent.
M05-03 remains awaiting project lead validation of this correction; M05-04 stays
conditional on acceptance/continuation.


## M05-04 headless scene example delivery (2026-10-18)

M05-03 accepted by the project lead's continuation instruction after the reported
GLM packing include correction. Compilation after that correction and backend
coverage remain unspecified; the agent did not compile. Starting source baseline
HEAD is bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d plus preserved inherited
uncommitted work. No commits were made.

Added the `draw_offscreen_application` target at
[examples/draw/13_offscreen_application](../../../examples/draw/13_offscreen_application/README.md)
and registered it in examples/CMakeLists.txt. It reuses the deferred scene's HDR
environment, OBJ/submesh textures/materials, transparent/solid-transparent model
variants, floor and four lights. Camera/light animation and UI are absent. CLI
validates dimensions, paired internal extent, scale, N (1..1048576), backend,
settings path/policy and output destination before rendering; explicit settings
and resolution precedence are delegated to M05-03. It defaults to Vulkan on
non-macOS and Metal on macOS, outputs a single `scene.png`, creates the folder and
refuses an existing destination. The R8G8B8A8 surface is read/saved in the final
completed callback only after `hasResult()`. Errors return nonzero; help returns
zero. Unknown lightPose IDs reject unless the explicit skip option reports each
skipped property; renderer/effect IDs that match the imported catalog still apply.

Agent checks were static only: full GPL header, C++ lexical/style inspection,
CLI and lifecycle call-site inspection, image writer/readback contract, unique
CMake target/resource helper registration, Markdown links and git diff --check.
No configure, compile, run, GPU render or encoded image inspection. Project lead
checks: configure the existing CMake build to discover the newly registered source,
build `draw_offscreen_application`, run `--help` and malformed/conflicting CLI
cases, render N=1 and N>1 at full/reduced internal resolution on available
Vulkan/Metal, inspect `scene.png` dimensions/orientation/content, check no overwrite,
import matching UI settings and exercise strict/skip unknown IDs and unavailable
FSR/RT fallback. See the new example README and M05-05 checklist.

M05-05 uses the actual registered target, explicit RGBA8/PNG path and final
completed callback. This paragraph records the original M05-04 delivery handoff;
its acceptance and the M05-05 delivery are recorded below.


## M05-05 verified image readback and PNG publication (2026-10-18)

M05-04 accepted by the project lead continuation instruction; compilation/checks
assumed successful under the agreed convention, backend/scenario coverage not
specified. Starting HEAD remains bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d, with
inherited uncommitted M04/M05 work preserved. No commits or CMake changes made.

The headless CLI checks the completed RGBA8 byte count, writes the PNG to a
PNG-suffixed temporary path, reads and decodes it through db::decodeImageRGBA8,
then verifies exact width, height and pixel-byte equality before publishing it
as scene.png without replacing an existing path. Encoding/verification failure removes the temporary output; existing
final and temporary paths are refused. Existing synchronous GPU readback is reused.
No BGRA/sRGB/HDR conversion is introduced. Both backend paths copy tight image
rows; current Vulkan and Metal viewport conventions put row zero at the top. The
PNG writer leaves row order and alpha unchanged. The example README describes the
comparison boundary: frozen camera/light, no editor UI/selection/gizmos, and linear
HDR N-sample mean followed by the final color/SMAA pass.

Static checks inspected signatures, callbacks, DB writer/decoder use, backend
viewport/readback order, GPL/style and documentation links; git diff --check passed.
No build, executable, test fixture, GPU render or independent image inspection was
performed. The project lead should build/run draw_offscreen_application, inspect
asymmetric output orientation/content and dimensions on available Vulkan/Metal, and
exercise unwritable destinations plus existing final/temporary paths. M05-05 is
delivered awaiting lead validation. The next linked step is M05-06 review after
acceptance/continuation.


## M05-06 technical review delivery (2026-10-18)

M05-05 accepted by project lead continuation instruction; compilation/checks
assumed successful under the agreed convention, backend/artifact coverage not
specified. Source baseline remains HEAD bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d
plus inherited uncommitted M04/M05 work. Added the M05 code-versus-plan review and
reconciled pending M06–M10 requirements: M05 readback/PNG is a camera-image path,
not complete resource serialization or a bake batch transaction; the headless
frame driver and exact camera accumulation remain distinct from bake scheduling;
the production OffscreenApplication path remains default pending M09 migration.

Static review covers public API ownership, settings/sample/completion lifetimes,
readback/encoding, documented orientation/color/alpha policy, pending plan impacts,
relative links and git diff --check. No compilation, runtime, backend, fixture or
image inspection. The lead should review review.md and provide any applicable
product evidence through the normal continuation/failure workflow. M05-06 is
delivered awaiting lead validation; next linked step is M05-07.


## M05-07 cumulative documentation delivery (2026-10-18)

M05-06 accepted by project lead continuation instruction; backend/scenario evidence
not specified. Starting HEAD remains bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d with
inherited uncommitted M04/M05 work preserved. Resumed after intentional turn
interruption. No source, shader, CMake, build or commit/reset work.

Added canonical DefaultOffscreenApplicationDelegate and FrameAccumulator references,
headless data-flow architecture, the actual scene CLI/build/output guide and
tutorial 13 with exact shipped source excerpts. Updated typed app/config/completion,
GPU surface/transfer, settings and shared navigation. Corrected stale render-only
offscreen statements and the obsolete always-valid/thin-wrapper GPU surface guide.
Production render guides and UV bake semantics remain separate and supported.
The coverage ledger maps 28 affected pages plus retained foundations to source,
checks and future owners. All 338 local links/anchors and three exact C++ snippets
pass static checks; git diff --check passes. No compilation, fixtures, GPU run,
packaging or independent visual checks were performed.

M05-07 is delivered awaiting lead acceptance. The review handoff now links this
coverage and maintained pages. The next step is M06-01 with explicit M05-07
documentation closure prerequisite; no M06 implementation is started.


## M06-01 draw editor startup and supplemental tutorial delivery (2026-10-19)

M05-07 accepted by project lead continuation; backend/scenario details remain
unspecified. The same instruction explicitly requests a complete offscreen tutorial
in the format of the other construction tutorials. Work resumed after intentional
turn interruptions, preserving created/staged files and inherited work. HEAD remains
bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d; no commit/reset/staging or legacy app edit.

Created draw_model_edit with separate app ID org.bg2e.apps.draw.model-edit and
graphics preference scope draw.model_edit.graphics. Native-free AppDelegate owns
a unique RendererDeferred through the existing default delegate; StageScene owns
a procedural fixture and same-node orbit camera; EditorUi binds live settings
before renderer attachment and uses weak delegate ownership. Camera reset is a
stopped generation-checked update. Renderer cleanup precedes all retained stage
drawable releases, even on errors. Only app registration/new target CMake was
modified as explicitly scoped by M06-01. Standard paired engine shaders/resources
use the existing helper. Production model_edit remains intact.

application-inventory.md maps Document, StageScene, ToolBar, SubmeshWindow,
EnvironmentSettings and ModelLightmapWindow workflows plus distinct legacy API/
widget/material calls to their remaining owners. M06-02–06 and M07-01 handoffs
reflect the concrete startup. No document IO, material tools or bake parity is
claimed from the fixture window. The offscreen tutorial now explains every part of
the full application with 22 consecutive C++ source blocks, including setup,
parsing, rendering, resources, completion/PNG and errors for a basic-C++ audience.
Those blocks reconstruct all current example source modulo whitespace.

Static validation checked seven owned C++ files, all 22 tutorial blocks, 335
local links/anchors across 28 maintained pages and 282 local links across 15
app/plan pages. Static source/style/dependency/lifecycle/resource/Markdown checks and both index/
working-tree diff whitespace checks pass. No build/configuration, shader compile,
C++ fixture, application/GPU or packaging run. Lead checks: draw_model_edit startup
on available backends, orbit/pan/zoom/reset, resize/minimize, UI/settings edits and
reopen, pending-update exit, optional-effect fallback and independent production
model_edit preferences; tutorial review and actual execution remain lead-owned.
M06-01 is delivered awaiting acceptance; next linked step is M06-02.


## M06-02 complete resource IO and editor document delivery (2026-10-19)

M06-01 accepted by project lead continuation instruction; no backend/scenario
coverage was supplied. Starting/finishing HEAD remains
bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d with inherited and staged changes
preserved. No commit/reset/staging, CMake edit, configure/build or executable run.

Implemented explicit-context complete scene/model export, native `.bg2tex`
readback/decoding preserving HDR and authored mip/layers, material sampler/color/
UV/inversion/indirect metadata, shared-parser MTL/glTF draw import and BG2 joint/
submesh/duplicate-material handling. JSON strings now escape/decode paths and
names correctly. Publication stages unique resources, replaces the main file
last, restores live references and rolls back new resources on ordinary failure.
Production overloads and graphics-free schema/reference IO remain separate.

The new editor Document records root/source/save paths, dirty state and generation.
Open/drop/import prepare candidates before adoption; renderer reattachment drops
old queues before retired resources are cleaned. Save/Save as materialize actual
live textures and mark clean after publication. Save/Discard/Cancel guards dirty
replacement/close, with cancelled or failed operations retaining the document.
A successful explicitly requested save before a failed open correctly leaves the
old document clean. Model-only BG2 rejects graph state it cannot represent.

[Resource IO handoff](milestone_06_model_edit/resource-io.md) and maintained
[API reference](../../../doc/api/db/DrawResourceIO.md) record signatures, lifecycle,
import subsets, native archive/production compatibility, publication/filesystem
limits and custom component resource contracts. Updated M06-03, M07-02 and M08-03
with actual consumer requirements. Added an unexecuted GPU HDR/mip/metadata/
rollback probe and a lead validation matrix.

Static GPL/style/lexical checks passed for 27 owned C++ files. API/declaration,
namespace/dependency, metadata, ownership/publication and rollback paths were
inspected; maintained Markdown paths/fences and working-tree/index whitespace
checks pass. These are nonexecuting checks, not compilation or backend evidence.
M06-02 is delivered awaiting project lead checks; next continuation selects M06-03.


## M06-03 editor tools and guarded editing delivery (2026-10-19)

M06-02 accepted by project lead continuation instruction; no backend/scenario
coverage supplied. Starting/finishing HEAD remains
bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d with inherited/staged changes preserved.
No commit/reset/staging, CMake edit, configuration, compilation, shader tool,
application execution, packaging or C++ probe execution.

AppDelegate/StageScene now own draw picking/selection, a Scene catalog and optional
renderer overlay, with typed document-only helpers and independent default mesh
resources. EditorActions and EditorTools implement queued weak generation/member
checks, node/submesh metadata and transforms, group/visibility updates, independent
node/material/GPU clones, partial JSON modifiers and geometry reload actions.
Dirty/redraw/history notifications include partial failures. Document replacement
prunes selection/capture and rejects old actions while old nodes may still live.

DrawMaterialEditor adds document guards, authored-image-preserving sampler edits
and explicit AO/bakedAO/bakedGI interpretation. Actual texture previews/history
use the draw UI session. Choosing ordinary AO clears baked interpretation; baked
modes require an assigned map and linear UV2 metadata. Partial sampler changes
preserve other targets' source/settings. Widget bindings detach before renderer/
stage/Engine teardown. Shared input now tracks scene-owned mouse gestures across
UI and prevents UI-owned releases from becoming scene picks.

Environment controls use stopped public generation before adoption, with document
lights, presets/custom images, floor/fill fixtures, camera intrinsics, explicit
independent environment default/preset IO and world-space pose restoration. File
menu/shortcuts, panel reopen and live dynamic Graphics settings are wired. Save
and dirty prompts flush pending edits first. Complete export rejects unresolved
texture requests and omits ignored runtime helpers during model validation.

[Editor tool handoff](milestone_06_model_edit/editor-tools.md),
[maintained guide](../../../doc/apps/draw_model_edit.md) and
[manual probe/matrix](../../../tests/draw/editor_tools/README.md) record actual APIs,
ownership, checks and remaining runtime evidence. Updated M06-04/05 and M07-03
with concrete camera, source-edit/job invalidation and widget reuse contracts.
The inventory now reconciles initial gap labels with M06-02/03 deliveries.

Static GPL/block-style/lexical checks passed for 19 owned C++ files; application
native include boundaries, source APIs, lifecycle, partial updates and IO ordering
were inspected. Maintained Markdown paths/fences and working-tree/index whitespace
checks pass. These are not compilation or backend checks. M06-03 is delivered
awaiting lead validation; next continuation selects M06-04. No automatic framing,
UV/bake UI or cumulative product/runtime/package acceptance is claimed here.


## M06-04 viewpoint and camera delivery (2026-10-19)

M06-03 accepted by project lead continuation instruction; backend/scenario coverage
unspecified. Starting/finishing HEAD remains bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d.
Inherited and staged work preserved; no commit/reset/staging, CMake edit,
configuration/build, shader command, application run or probe execution.

Added camera framing from selected-node/document world bounds and a separate best-view
action using the draw `utils::ViewpointAnalyzer`. Actions run at a stopped renderer
update boundary, wait for device idle, retain analyzer resources in StageScene and
revalidate weak target, document generation, renderer generation and stage membership.
Analysis uses 72 candidate poses at up to 48 pixels per longest image dimension, preserving the active viewport aspect, and current vertical FOV, applies only a
nonempty visible best candidate through `applyViewpointToOrbitCamera`, and maps
pixel coverage through the analyzer's per-call identifier table. UI reports candidate
index/count, score, visible coverage and the top three submeshes. Failed, empty or
stale work preserves camera pose. Analyzer teardown precedes stage resources.

See [viewpoint-camera.md](milestone_06_model_edit/viewpoint-camera.md), updated
maintained editor guide/README, manual editor matrix, feature inventory, M06-05 and
M07-03 camera handoffs. Static review and documentation checks only; no compile or
runtime evidence. M06-04 is delivered awaiting project-lead validation; next is M06-05.


## M06-03 UI parity correction delivered (2026-10-19)

The project lead reported that the draw_model_edit UI was not equivalent to
apps/model_edit and could not be accepted: floating windows did not reproduce the
Workspace layout, panel composition, menus or panel launch behavior. Reopened M06-03.

Correction delivered: `bg2e::ui::Workspace` now arranges a full-width toolbar, docked
Model Properties left panel, Environment Options right panel and bottom StatusBar.
File/View/Window menus include checkable dock visibility; UI/Graphics settings open
from Window. Toolbar geometry actions and weak callbacks remain draw-safe. The main
window launches maximized. Static checks passed; no build/runtime execution. M06-03
correction awaits project-lead validation. M06-04 remains implemented and unvalidated.

## M06-03/M06-04 continuation acceptance and M06-05 delivery (2026-10-19)

The project lead continued after the M06-03 Workspace parity correction. Under the
plan's continuation convention, M06-03 including that correction and M06-04 are
accepted; backend/runtime coverage is unspecified. Implemented M06-05 in
`apps/draw/model_edit`: added a Window-menu UV/Lightmap utility with UV1/UV2
diagnostics, guarded safe UV2 regeneration and draw-session previews; an
IntegratedBakerContext job records AO/GI samples into ordinary draw-frame command
buffers and commits each exact progress sample only after completion; cancellation,
document replacement/source edits and teardown retire work before releasing the
borrowed Engine context; completed composed output can be previewed and explicitly
applied to every target submesh's live AO texture and CPU material definition with
linear UV2 baked AO/GI metadata. M06-02 complete model/scene save persists the GPU
image as `.bg2tex`. The document graph is the bake root, excluding editor fixtures.
Static source/API/lifecycle/metadata/export and documentation checks passed. No
compile, shader, application or backend execution was performed. M06-05 is delivered
awaiting lead validation; the next pointer is M06-06 product validation.

## M06-05 continuation acceptance and M06-06 validation review (2026-10-19)

The project lead continued after M06-05. Under the execution protocol, M06-05 is
accepted by continuation; backend/runtime coverage is unspecified. M06-06 static
review confirmed the draw_model_edit target uses the shared SDL bundle helper, which
deploys engine shader/assets resources, and the distribution collector recursively
includes registered application executables with runtime dependencies/resources.
The maintained feature inventory and manual checks were reconciled into
`milestone_06_model_edit/product-validation.md`. Static whitespace checks pass.
No native configuration, compilation, shader build, package script, executable or
manual scenario was run. M06-06 is delivered awaiting project-lead runtime/package
validation; M06-07 is the next linked step.

## Global draw renderer defaults amendment (project lead direction, 2026-10-19)

The project lead clarified that GI mode, enabled GI and enabled reflections must
be the draw renderer's defaults, not an app-only setup. Updated
`RendererConfiguration::indirectLighting` to `GlobalIllumination` and changed the
default RTGI/reflection settings to enabled. `draw_model_edit` inherits this
configuration; no application-specific duplicate is required. Explicit app/pass
configuration remains authoritative, including `draw_lightmaps`' explicit IBL/off
comparison. Runtime availability/status and unsupported fallback do not change.
Updated RendererDeferred/RTGI/reflection/Settings references, draw app guides,
tutorials, manual deferred checks and M03 historical amendment. Static whitespace
and Markdown checks are recorded with the change. No compilation, shader or runtime
checks were run. The current M06 review acceptance/documentation handoff remains
in force.

## M06-05 correction — nonfatal UV2 overlap (2026-10-19)

The project lead reported that draw_model_edit rejects an atlas for positive-area
triangle overlap, including after UV2 regeneration, and explicitly requested bake
continuation even for genuinely overlapping maps. Added a Warn overlap policy to
the shared CPU validator while retaining its strict two-argument overload. Draw
surface packing, production LightmapBaker and standalone preflight now warn rather
than reject/skip these targets; generator chart collisions also warn. Other
structural/coordinate errors remain fatal. The draw editor retains a visible UV2
warning independently of transient bake status, and legacy preview shows warnings.

Generator chart checks sample texel centers and compare different chart IDs; the
geometric validator compares continuous triangle areas, including within charts.
Their difference explains why generation can succeed before the old bake check
rejects the result. The clipping routine also mixed tolerant classification with
zero-boundary intersections; it now uses consistent boundaries and translated
polygon area to reduce cancellation. No supplied model was available to establish
the exact source of this reported overlap; the screenshot alone cannot distinguish
a small genuine overlap from a numeric false positive.

Regression source covers warning statistics for fully duplicated UV quads, strict
validation compatibility, thin shared edges, malformed indices and continued UV
surface packing. No compile/test/GPU execution was performed. Updated maintained
API, architecture and editor guidance. Correction awaits project-lead validation;
M06-07 also remains pending and the next planned step is still M06-08.


## M06 editor corrections accepted; M06-08 started

The project lead explicitly reports correct rendering without lights and asks to
close the current step and implement the successor. This accepts M06-07 and the
subsequent editor corrections under the continuation convention: UV overlap
warnings, automatic temporary-file bake application and retained result preview,
graphics/diagnostic separation and nonpersistent diagnostics, focused reflected
light/transform editors, numeric dragging, light-only gizmos, removal of all lights,
and initial/random light positions. The lead reported compiler errors during the
corrections; their fixes were delivered. Backend/package scenario coverage beyond
the explicit zero-light statement remains unspecified. No further zero-light
renderer analysis is requested or performed. M06-08 documentation is now active.


## M06-08 cumulative documentation delivered

Reconciled maintained documentation with accepted M06 editor corrections. Added a
sequential model editor tutorial; refreshed graphics/debug references, complete
resource IO, app/architecture guidance and navigation. The documentation inventory
maps affected and retained pages to source and assigns future Composer/batch/
retirement obligations to M07/M08/M09, with M10 as the final audit. Product evidence
now records M06-06 continuation acceptance and the lead's explicit positive
zero-light result while leaving package/backend coverage unspecified. Static
link/anchor/fence/source/whitespace checks passed. No build, shader, app or package
execution was performed. M06-08 awaits lead acceptance; next is M07-01.


## M06-08 accepted; M07-01 application foundation delivered (2026-10-19)

The project lead requested the next step, accepting M06-08 by continuation.
Backend/package coverage remains unspecified. Created the separate
`draw_bg2e_composer` target with typed draw startup, maximized Workspace, independent
app/bundle/preferences/environment/temp identities and shared engine resources.
Copied app-owned M06 orchestration while reusing engine DB/material/reflection/
selection/settings/viewpoint/bake services unchanged. Initialized one editable
Scene group containing the startup cube; editor camera/floor/fill are fixtures.

The [application inventory](milestone_07_bg2e_composer/application-inventory.md)
maps production scene/component/tree/import/server/multi-target/export/bake
workflows to actual replacements or M07-02–06 work. Open model actions explicitly
replace the document. No scene insertion, async HTTP service or multi-job bake
parity is claimed. M07-02 now records the concrete root/camera/import handoff.

Static GPL/include/identity/draw-boundary/resource-registration/ownership and
whitespace checks passed. No build/configuration/shader/application/package
execution, commit, staging or reset. Existing production and model-editor sources
remain unchanged; starting/finishing HEAD is
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d` with inherited changes preserved.
M07-01 awaits project-lead validation; next is M07-02.


## M07-01 accepted; M07-02 scene IO/cloning delivered (2026-10-19)

The project lead requested the next step, accepting M07-01 by continuation.
Starting/finishing HEAD is `bd6a95076141335550f415f47ea2a824703b119c`, checkout
initially clean; no commit, staging or reset. Backend/package coverage unspecified.

Composer now queues stopped Open/Save/New/Import/Export operations. Shared path
helpers preserve Name/Name.json naming and redirected-overwrite cancellation;
complete draw writers retain GPU-only resources and mark clean after publication.
Document replacement is dirty-guarded; model/prefab imports insert under a weak
selected parent with document generation and separate unit/axis wrapper. Selected
model exports exclude descendants and node placement; each batch item publishes
independently, with failed destination reported. Active bake work retires before IO.

Added explicit Scene root ownership transfer, exact single-root unwrapping and
complete multi-root containers. Camera-required scene Open uses schema validation
then draw resource realization, retains perspective/optical parameters, supplies
missing optical projection defaults and chooses the first enabled camera. The
initial camera is now authored document state; camera controls mark changes dirty.
Shared lookup resolves component subclasses; factories enforce detached registered
types; Node clones reject sliced/shared/owned copies; camera projections clone
independently; controller clones clear input gestures. Existing complete draw
resource writers remain authoritative.

[Scene IO handoff](milestone_07_bg2e_composer/scene-io-cloning.md) and
[source probes/manual matrix](../../../tests/draw/composer_io/README.md) record
actual APIs, resource lifetime, custom component hooks, batch failure boundaries
and checks. Updated M07-03/04 and M08-02 with concrete ownership/input/import
requirements. Full scene UI, fixture/light policy and async/multi-bake scheduling
remain in their planned successors.

Static GPL/style/delimiter/include/interface/mode/ownership/resource-path/cancel/
rollback and Markdown/whitespace checks passed. No compilation, configuration,
shader, executable, source-probe or packaging run was performed. M07-02 awaits
lead validation; next is M07-03.


## M07-02 accepted; M07-03 scene editor delivered (2026-10-19)

Project lead continuation accepts M07-02; backend/package coverage unspecified.
Starting/finishing HEAD is f7c2498cb4c29e27921c91809043dcf5fe4d963c, initially
clean. No CMake edit, build/configuration, shader, executable, package, commit,
staging or reset operation was performed.

Implemented the hierarchical SceneTree with stopped weak selection/reparent
intents, explicit node-only selection, separate typed submesh/material rows,
lightweight staged reflected component editing, node/component creation/removal,
enable validation and rollback. Eight enabled document lights and at least one
enabled camera are enforced across these graph changes. Transform tools now apply
to selected scene nodes; runtime Gizmo Drawable storage is excluded by the explicit
isSceneGeometry contract from Node::drawable and Composer geometry/bake lookup.

Only the active authored camera controller receives viewport input. Camera changes
clear controller gesture state; framing/best-view application converts world
candidates into a rigid/uniform orbit parent, preserving projections and rejecting
unrepresentable parent transforms before changing the pose. Existing materials,
viewpoint and renderer/upscaler settings import/export remain integrated.

[Scene editing handoff](milestone_07_bg2e_composer/scene-editing.md) records source
contracts, primary-node versus selection-wide action policy, rollback/lifecycle,
projection/parent limits and the lead's Vulkan/Metal/fallback/production regression
checklist. M07-04/05 consume these mutation and invalidation hooks. Static checks
only; no compilation or runtime evidence is claimed. M07-03 awaits project-lead
validation; next is M07-04.


## M07-03 correction — submesh selection bool reference

The project lead reported compilation failure in SceneEditor.cpp: a temporary
bool returned by SelectionManager::isSelected was passed to SelectableList::item,
which requires bool&. Stored the result in a mutable local bool before calling
the widget; the existing queued selection action remains authoritative. This is
unrelated to the const qualification of isSelected. Static signature/call-site
inspection and git diff --check passed; no compilation or runtime check was run.
M07-03 including this correction awaits lead validation; next remains M07-04.


## M07-03 accepted; M07-04 asynchronous import/service delivered (2026-10-19)

The project lead requested the next step, accepting M07-03 and its bool-reference
compiler correction by continuation. Starting/finishing HEAD is
f7c2498cb4c29e27921c91809043dcf5fe4d963c with inherited M07-03 modifications
preserved. No CMake/configure/build/shader/executable/HTTP/package/probe command,
commit, staging or reset was performed. Backend/HTTP coverage is unspecified.

Added app-local ImportServer/Request/Settings and a joinable SceneImporter CPU
worker with one active/16 waiting jobs, Loader/cancel/queue UI and independent
service preferences. The localhost POST /import and GET /status protocol retains
its occupied HTTP slot, errors, units/axis settings and canonical remote reimport.
Worker results carry independent request/document/cancellation and weak destination/
reimport identities. Admission captures document generation before main-thread
polling; successful scene replacement cancels old admission/jobs and clears tracking.
Same-renderer scene/resize rebinds reacquire current generation, while a different
renderer or removed/moved target rejects publication. Worker/HTTP shutdown joins
before Engine teardown; no detached loading task touches draw resources.

DB now separates CPU glTF parser/buffers, OBJ/MTL geometry/attributes and existing
BG2 parsing from explicit stopped draw realization. Prefab schema parsing is also
worker-only; factories, vertices/images/material realization and GPU submissions
stay on the main thread. Source parser subsets remain unchanged. Stage import
publication preflights camera/eight-light constraints excluding the old subtree,
keeps previous resources alive through extraction/settings rebind, and restores
graph/sibling/camera/environment/dirty state on ordinary failure. Bake source work,
selection/gesture capture, clipboard and histories are invalidated at the relevant
boundaries. Failed rollback bindings propagate to renderer-first teardown.

[Async import handoff](milestone_07_bg2e_composer/async-import-service.md) records
actual APIs, resource/thread/pause/cancel/publication semantics, cooperative parser
cancellation and synchronous realization limits. Maintained DB/service references,
app README, inventory, M07 review/bake and M08/M09 handoffs were updated. The
[lead matrix](../../../tests/draw/composer_import/README.md) covers queue, HTTP,
reimport, stale targets, shutdown and Vulkan/Metal/fallback/production regression.
Static checks only; no compiled, executed or hardware behavior is claimed.
M07-04 awaits project-lead validation; next is M07-05.

## M07-04 accepted; M07-05 scene baking delivered

Project lead continuation accepts M07-04; backend/package/HTTP coverage remains
unspecified. Starting HEAD is f7c2498cb4c29e27921c91809043dcf5fe4d963c with inherited
M07-03/04 changes preserved. No commit/reset/staging, CMake, core API or shader
changes, configuration, compilation, executable, GPU, probe or package execution.

Composer now schedules unique selected-subtree or whole-document receivers
serially through one integrated context and one live baker. UV2 preflight retains
Warn overlap policy. Every typed result is captured after completion before any
receiver material changes; previous bakers release before successor creation.
Stopped application validates context/document/node/drawable identities, prepares
all temporary linear byte/HDR textures before mutation, preserves live/stored
baked mode/UV metadata and marks dirty for complete scene save. Per-target previews
retain applied images separately and release before Engine teardown. Cancellation,
source edits/imports and renderer-binding changes invalidate pending adoption.
Partial material adoption failures remain explicitly reported/non-atomic; unused
preparation files roll back, while referenced applied sources remain available.

[Scene-baking contracts](milestone_07_bg2e_composer/scene-baking.md) and
[lead checks](../../../tests/draw/composer_baking/README.md) record memory,
completion, precision, save and verification limits. Static source/API/style,
ownership, local link and diff whitespace checks performed; no native behavior is
claimed. M07-05 awaits lead validation; next is M07-06 product validation, not the
milestone review or documentation closure.

## M07-05 accepted; M07-06 product validation delivered

Project lead continuation accepts M07-05; exact backend/scenario/package coverage
is unspecified. HEAD remains f7c2498cb4c29e27921c91809043dcf5fe4d963c and inherited
changes are preserved. No commit/reset/staging, CMake edits, configure/build,
shader compilation, app, parser, HTTP, GPU, fixture or package execution.

[Product validation](milestone_07_bg2e_composer/product-validation.md) reconciles
the historical M07-01 inventory with actual M07-02–05 IO, tree/component, import,
material/camera/settings and bake implementations. Read-only inspection confirms
shared bundle resources and recursive package target collection, not native launch.
Comparison found missing production Chain add/remove controls; SceneEditor now
adds the CPU component through queued document-checked mutations and exposes
removal through its lightweight component inspector. Existing clone and pure
serialization paths are reused; this missing feature is not deferred to retirement.

Static source/license/style/API/ownership, Markdown path and working-tree/index
whitespace checks performed. The consolidated lead matrix includes the correction,
complete scene/save resources, cancel/shutdown, dialogs, Vulkan/Metal/no-RT/FSR,
deployed resources and production regressions. M07-06 awaits validation; M07-07
technical review is the linked successor, with M07-08 documentation still required.

## M07-06 selection crash reported and correction delivered

The lead reports object selection crashing in ImGui::TableNextRow with a null
current table. Static inspection found SceneEditor.cpp calling SelectableList::item
outside any list/table scope. This delivered M07-03 inspector defect is corrected
under the current M07-06 product failure; prior acceptance history is preserved.
It is not deferred work for milestone completion. No claim of runtime reproduction
is made and no ImGui/vendor modification is needed.

Composer now opens/closes its submesh list. Shared SelectableList::beginList returns
bool and forwards BeginTable's success flag; Composer, draw/production model_edit,
SubmeshSelector and GPUSelectionDialog skip items/endList on false. This fixes the
separate latent clipped/skipped-table misuse without suppressing null accesses in
ImGui. External callers need the same conditional scope and a rebuild. Public UI
references and tests/ui/selectable_list/README.md record the contract/checks.

Static call-site/API/scope/license/style and whitespace checks only; no build,
app, shader, fixture or native runtime execution. M07-06 remains awaiting lead
validation including this correction. M07-07 is held as successor; the milestone
does not advance on this failure report.

## M07-06 correction accepted; M07-07 technical review delivered

The lead explicitly confirms the selection-crash correction is checked and asks
to continue. M07-06 including corrections is accepted; OS/backend/SDK/package and
other scenario coverage remains unspecified. Starting/finishing HEAD remains
f7c2498cb4c29e27921c91809043dcf5fe4d963c, with inherited tracked/untracked work
preserved. No source/shader/CMake edits, staging, commit/reset, configuration,
build, executable, HTTP, GPU, fixture or package execution in this review.

[M07 review](milestone_07_bg2e_composer/review.md) records actual root transfer,
subclass/clone/projection/custom component hooks, complete versus reference IO,
queued tree/material/camera/Chain/UI edits, separate request/document/renderer
identities, joined CPU/HTTP workers, rollback/reimport and serial integrated bake
completion/adoption. The lead-checked bool SelectableList scope correction remains
part of the surviving UI contract. Package routing is source evidence only.

Compared all pending M08–M10 summaries/steps and updated three summaries plus
eleven concrete owning steps: M07-08; M08-01/02/03/05; M09-01/03/05/06;
M10-02/04. Root index and M07 summary/product evidence are reconciled. The review
distinguishes single-resource complete save, per-item selected export and temporary
material adoption from still-required standalone protected batch publication.
Remaining transitive production/shared DB/UI/app headers and the legacy output
writer are assigned to actual M09 cleanup, not hidden by the app's draw path.

Static review includes 43 owned source GPL/function-separation inventory, scoped
brace/include/namespace/method-group review, API/ownership/dependency checks,
Markdown paths and working-tree/index whitespace checks. No all-backend/numeric/
package results are inferred. M07-07 awaits lead acceptance; M07-08 cumulative
documentation is next and must be accepted before M08 begins.

## Requested Composer UI parity correction after M07-07 delivery

The lead requests removal of Environment Options, right-hand Scene properties,
left-hand selected drawable/material editing and production component controls.
This is a correction of M07-03/UI parity and the pending M07-07 assessment, not
acceptance of M07-07 or permission to implement M07-08. Inherited work preserved.

Removed the panel/draw function and environment/light/preset controls. Workspace,
menu checkmarks and toolbar toggles now bind left Model Properties and right Scene.
The Scene panel uses production-style independent tree/inspector child regions.
Shared ComponentInspector retains its production defaults and adds optional
add/remove/draw intent callbacks; Composer queues registered component creation/
resource validation and staged CPU property edits instead of drawing fixed buttons.
Drawable/submesh/material controls move left; multi-node editing remains materials
only. Node creation and deletion remain toolbar actions. Floor/fill fixtures start
disabled, document environment supersedes fallback on refresh, and tool preparation
does not recreate removed authored Transforms. No production app, CMake, shader or
native backend edits; no compile/runtime execution.

[Correction and lead checklist](milestone_07_bg2e_composer/ui-parity-correction.md)
record actual contracts. Review and M07-08 handoff are reconciled; static API,
ownership/scope/license/style/link/whitespace checks performed. M07-07 plus this
correction await validation; M07-08 remains the conditional successor.

## Open Scene failure report and bounded correction

The lead reports Open Scene appears to do nothing; whether the native chooser
opens or a selected file fails validation is not specified. Source inspection
confirms the menu queues a stopped document operation. Two concrete defects were
corrected without claiming a reproduced or fully diagnosed runtime failure:

- FileDialog::openFile/saveFile constructed an unnamed NFD::Guard temporary,
  calling Quit before OpenDialog/SaveDialog. Named guards now retain initialization
  until after dialog/path owners are released. PickFolder already did so.
- Composer hid operation errors behind the Unsaved changes status. Dirty status
  now retains the operation message; chooser exceptions and document-operation
  failures show a visible warning, and Open explicitly records its queued kind
  and Opening status. Existing failed-candidate cleanup/dirty replacement guard
  remains intact; scene validation is not bypassed.

No build/runtime/native dialog reproduction. Static API/lifetime/caller/whitespace
checks only. Lead should test Open/Save chooser, cancel, opening a valid camera-
bearing scene and an invalid/missing-resource scene; report the displayed error
if opening still fails. Open and earlier UI corrections await validation, and the
pending M07-07 review/M07-08 pointer does not advance.

## Open Scene correction — unregistered components are nonfatal

The lead identifies an unregistered JSON component as the load failure and requires
warning/skip rather than abort. This supersedes historical strict unknown-type
rejection in the schema/draw path. ComponentFactoryRegistry now warns and returns
null for unregistered types; the draw overload guards null before loadResources.
Node already skips null components and continues with later components/children.
Registered constructors/parsers/resource failures remain distinct errors.

Updated the CPU DB probe to put an unknown type before valid components and check
successful continued loading, plus raw JSON retention. Added the missing owned
probe GPL header. Maintained reference and operative decisions record that typed
saves omit skipped components while raw IO remains lossless. No build or probe/
runtime execution. Lead should reopen the reported scene and check a warning,
remaining components/hierarchy/resources and save semantics. Correction awaits
validation; the review/documentation pointer does not advance.

## Open Scene correction — legacy null component entries

The lead supplied `primitives_scene/primitives_scene.json` and the error
`Invalid component representation in node Scene`. Read-only JSON inspection
found twelve null component entries, one at index zero of each affected node,
including Scene and Camera. The CPU Node parser threw before calling the factory;
the previous unknown-type correction therefore could not handle this failure.
No application/backend reproduction was performed and the supplied file is unchanged.

Node schema loading now warns/skips nonobject component and child entries, matching
the production loader's existing policy. Factory schema loading warns/skips
missing/invalid/empty types and components without a schema constructor, as well
as unregistered types. Node serializers omit empty/nonobject component results
instead of emitting null placeholders. The original producer of the supplied
file's placeholders is not established. Registered constructor contract violations,
parser errors and required-resource failures are not indiscriminately swallowed.
Composer still validates an enabled camera, valid camera projection and at most
eight enabled lights before realizing resources and adopting the candidate;
failure retains the current document. Typed save omits skipped data; raw IO is
the lossless path.

Extended the manual CPU DB probe with null/scalar/untyped component entries and
an invalid child followed by a valid descendant. Static call-path/API/JSON/diff
checks performed; `git diff --check` passes. No compilation, C++ probe, app,
shader or GPU execution. Lead validation: reopen the supplied scene and verify
warnings, camera, hierarchy and eight drawables; check unknown components before
valid entries and save/reopen. M07-07 corrections remain awaiting project lead;
M07-08 is still the conditional successor, not authorized by this failure report.

## Guarded scene loading correction and macOS tracing investigation

Starting HEAD `a5bdc351e7a7`, preserving the earlier uncommitted null-entry
correction. The lead reports production uses asyncLoadGuarded/Loader and requests
equivalent draw scene loading, plus analysis/options for tracing overflow. This
does not accept the full pending M07-07 review/UI work or authorize an RT redesign.

Draw Composer had no asyncLoadGuarded and Open was synchronous. Added the guard,
shared MainLoop async scene-document IO/JSON parsing, document-generation checked
main-thread completion, and one-component-per-frame resource realization with a
visible Loader while scene drawing/editing/publication is paused. Dirty protection
runs before the worker; candidate adoption waits for all resources; ordinary
failures preserve the previous document before adoption. Concurrent IO/input/close
is guarded. Component GPU uploads, model/image decode and renderer/environment
initialization are not fully asynchronous or interruptible and may stall their
individual frame. The separate existing model import service remains unchanged.

MainLoop's detached worker lifetime was unsafe on exceptional exit; workers are
now joined before normal/exceptional app/UI/Engine cleanup, and exiting-run
completion tokens expire. No graphics queue is touched by the new scene read
worker. The [investigation](milestone_07_bg2e_composer/loading-and-tracing-investigation.md)
records actual contracts, source anchors, the conservative Metal 256 total ceiling
plus reserved bindings selecting profile 128, per-resource identity deduplication,
neutral-texture/sampler inflation, and the uncaught post-load frame preparation
error. Options include graceful failure plus immutable-neutral/sampler
canonicalization, corrected native budgets/profiles, larger/bindless tables and
atlas/paging tradeoffs. No tracing source, shader, capacity or default is changed.

Static API/call-site/ownership/pause/progress/shutdown inspection and whitespace/
local-link checks only; no compilation, shader, app, C++ probe, GPU or package
execution. Lead checks are in tests/draw/composer_io/README.md. Validate loading
with advanced tracing explicitly disabled to isolate it, then select an RT remedy.
Reported tracing exit remains open and the pointer returns to M07-07 correction;
M07-08 is not started.

## Lead-selected tracing overflow fallback delivered

The lead requests the simplest nonfatal sampled-table policy: treat an excess
texture as absent, postponing image/sampler-management improvements. This is a
correction of M07-07, not acceptance of pending loading/UI work or M07-08 entry.
Earlier uncommitted source/documentation changes are preserved.

TracingSceneResources now checks both image and sampler capacity before admitting
either new resource. If either table cannot accept the pair, both material index
lanes become UINT32_MAX; tables stay bounded and admitted pairs remain usable.
TracingSnapshot exposes fallbackTextureSlots. Shared GLSL/Metal traceSample returns
white (flat normal for slot3) before descriptor access/color decoding. Scalars,
material alpha/cutoff and all instances remain; raster material state is untouched.
An overflowing alpha map may lose its holes in tracing, as explicitly authorized.
Warnings are rate-limited per service until successful recovery/clear. Other
geometry/resource/device errors are still explicit; profiles/budgets are unchanged.

Updated the maintained tracing API/operative decision and investigation/review.
Extended the manual RTAO fixture with 32-entry overflow, index bounds/sentinel
accounting, retained geometry/materials, repeat prepare and recovery with old
snapshot retention. Static CPU/shader ABI, descriptor-access ordering, native pair,
warning/lifetime and diff checks only; no build, shader compiler, C++ probe, app
or GPU execution. Lead must rebuild engine/shared native tracing shaders and
reopen the supplied scene with GI/reflections enabled on available Metal/Vulkan;
check warning without exit, unchanged raster textures and retained geometry.
M07-07 corrections await validation; M07-08 is restored as conditional successor.

### Composer component replacement compiler-warning correction

The lead reports Clang `-Wpotentially-evaluated-expression` for dereferencing
shared_ptr operands inside polymorphic typeid in SceneEditor.cpp. The replacement
validator now obtains raw pointers with get() before typeid, avoiding overloaded
operator* in its operands. Existing shared owners, null guards, dynamic-type and
detached-owner checks remain unchanged. Static diff/call-site checks only; no
compilation/runtime execution. M07-07 corrections remain awaiting lead validation;
this compiler output does not establish tracing or scene-loading runtime results.

## 2026-10-20 — Exact tracing capacity up to 16384 delivered

The lead explicitly selects the smaller of the device budget and 16384 and asks
for both native implementations and consistent plan references. This is a scoped
M07-07 correction, not a request to advance to M07-08 or acceptance of all prior
UI/loading/runtime checks. Entry HEAD is ae0de5bbe015 with a clean working tree.

ResourceArrayCapabilities::profileCapacity now returns the exact reserved-budget
minimum, including aggregate stage limits, with zero/underflow guards. Metal
removes the artificial 256/1024 limits and uses Tier-2 indirect texture and queried
sampler budgets. Vulkan retains queried normal descriptor limits. AO/GI/reflection
select `_256` deployment templates with native v2 contracts; Vulkan specializes
array size via UInt32 ID0 and Metal encodes separate pointer-indexed argument
records at buffers8/9 with native stride/length checks and unique residency entries.
Custom shader replacements must adopt v2; settings/reflection persistence IDs are
unchanged. No per-object material ownership, clone/import/export, neutral overflow
fallback, production render or CMake behavior is changed.

The [capacity report](milestone_07_bg2e_composer/tracing-capacity-16384.md) records
actual arithmetic/native contracts, sampler-budget caveat, retained primitive
probe templates and deferred refactor. Root index/operative decisions, M03 source
notes and four step contracts, M07 review/documentation handoff and M08/M09/M10
owning steps are reconciled. Historical delivery/review/progress entries explicitly
retain original provenance instead of claiming old code had these capacities.
Maintained tracing/effect/GPU references and native validation matrices are updated.

Static checks cover source/native API spellings, specialization data lifetime,
binding/record/residency agreement, capability arithmetic, licenses/style, includes,
local paths and diff whitespace. Manual probes add 16384/1021/132/aggregate/zero
capacity boundaries and non-power-of-two snapshot capacity37; they were not run.
No configure/build, shader compilation, C++ fixture, app/GPU or package execution.
Lead should rebuild engine and paired native shaders, run the named probes and
reopen the supplied Composer scene with enabled GI/reflections on available Metal/
Vulkan, checking exact capacity, overflow, no-RT/raster fallback and native
diagnostics/lifetime. M07-07 correction awaits validation; M07-08 remains the
conditional successor. No later milestone implementation is started.
### Shared clone typeid compiler warnings corrected

The lead reports Clang potentially-evaluated-expression warnings in
CameraComponent::clone and Node::clone. Both now extract raw pointers with get()
before typeid, avoiding overloaded smart-pointer dereference inside its operand.
Null/independence/owner and concrete dynamic-type checks remain unchanged, with
the original shared owners retaining lifetime. Static call-site/diff checks only;
no compilation or runtime execution. M07-07 remains awaiting lead validation;
the reported compilation progress is not scene/tracing runtime evidence.
## 2026-10-20 — Direct query-shadow table expansion delivered

This corrects the immediately preceding M07-07 report, which described the
eight-pair limit as still present. That previous statement is superseded; the
older initial-capacity diagnosis is retained only as chronological evidence.

Code/documentation baseline for this follow-up: inherited M07 capacity expansion
started at `ae0de5bbe015`, with current pre-correction source changes preserved.

The lead reports Model Edit projects shadows correctly but Composer scenes with
more objects do not. Static comparison found identical renderer configurations,
but SoftShadowPass still retained a separate eight-pair alpha table and whole-
scene neutral fallback. The supplied primitives_scene materials omit alphaCutoff,
so the default 0.5 flags even untextured materials for alpha queries; independent
white fallback textures can exceed eight slots with only two six-submesh cubes.
This source/data analysis is not a runtime reproduction; the lead's positive
Model Edit/negative Composer report does not establish cross-backend coverage.

The lead directs expansion of remaining needed limits and postpones texture-count
improvements until after API migration. Preserve all inherited uncommitted work;
this correction changes no app configuration, material defaults, object-local
ownership, texture creation/canonicalization, production render or CMake files.

SoftShadowPass now selects exact capacity via profileCapacity(2,1,9), bounded by
16384 and device budgets. Query kernels use native v2: Vulkan specializes ID0 with
image/sampler arrays at bindings16/24; Metal uses record buffers5/6. The existing
alphaTextureLimit diagnostic now reports alphaTextureCapacity(). Larger scenes
below that limit retain alpha-aware direct shadows. If a texture pair cannot fit,
its sentinel means neutral alpha with authored scalar alpha/cutoff retained; all
instances and the rest of the scene's shadows stay active. Warning/status replaces
the old ShadowTextureCapacityError/whole-scene neutral path; sceneSupported remains
true for an available degraded pass. No-query/no-array/format/kernel availability
failure retains explicit status and ordinary raster lighting. Immutable table
owners, completed-slot buffers and per-invocation descriptors remain retained.

Updated the query table fixture for more than eight pairs, forced capacity9 and
fallback/index/instance accounting; added its missing full GPL header and made its
intended opaque first material explicit. The CPU capacity helper checks shadow
reserves yielding 1023 from a 1024-sampler budget. Source/native API/binding/record/
ABI/lifetime/license/style and static arithmetic/link/whitespace checks only;
no compilation, shader tools, C++ fixture, application/GPU or package execution.
Maintained renderer/architecture guidance, test matrices and affected plan notes
are reconciled; texture-count optimization is a separately authorized post-
migration task, not hidden in this correction or scheduled as current migration.

Lead validation: rebuild engine and paired query/tracing shaders; open the same
Composer scene with shadow-casting light and more than eight alpha pairs, compare
Model Edit, inspect alphaTextureLimit/status, retained cutout holes and native
Metal/Vulkan diagnostics where available. Also check genuine overflow, scalar
alpha fallback, no-query/no-array, resize/reload/retained-slot ownership. M07-07
remains awaiting validation; M07-08 is still the conditional successor.
## 2026-10-20 — Composer Environment selection and editing correction

The lead requires loaded-scene Environment selection, HDR assignment/clearing,
procedural RGB edits, default sky after active-component removal, and ignored
additions while a component is assigned. This reopens M07-07 correction work, not
M07-08. Preserve inherited uncommitted source/native-table/shadow changes.

Implemented opt-in explicit Scene::setMainEnvironment, including intentional null
selection and weak component/node ownership with enabled membership validation.
Composer removes its hidden stage Environment, selects the first eligible authored
component on fresh adoption, retains valid selection over additions, and returns
to renderer defaults after removal without promoting ignored components. Newly
eligible components can fill an empty selection. Failed import rollback restores
the old selection/known-candidate snapshot. RendererDeferred, queue extraction and
live settings all honor that same source; non-opt-in Scene clients preserve their
previous automatic behavior and Model Edit workflow is unchanged.

Inspector updates built-in Environment fields in place at the stopped boundary,
retaining author/widget identity. Arbitrary edited clones no longer directly
publish generator resources or revert colors on a generation exception. Only the
selected source is synchronized by the renderer. HDR errors retain requested
fields/last-good resources and appear in status/inspector; clearing HDR restores
stored procedural colors. Both image property alias and legacy JSON reference load,
and missing/empty references clear old state. No new active-ID serialization format:
initial Open of multiple eligible components chooses first enabled depth-first.

The [source-backed report](milestone_07_bg2e_composer/environment-synchronization.md)
also corrects earlier chat hypotheses: CubemapRenderer::cleanup does not clear its
settings, synchronize already includes rebuild intent, and prepare runs while
presentation is paused. SceneInputs failed to reset the attached Scene's explicit
Environment selection on new root adoption; fixed. Automatic Scene selection now
skips disabled branches and captures default/null explicitly. The supplied
renderer startup lookup before root attachment remains implicit, so it cannot
freeze the default sky before Composer adopts the new scene. ResourcePicker Clear
is opt-in and accepts empty selection only for the HDR property, while Browse
cancellation preserves the current path. Fresh Open resets the generator source
cache and rereads even an identical HDR path. The supplied
primitives_scene points to black.jpg, an 8-bit JPEG incompatible with the
floating-point HDR loader; this can leave the procedural sky visible. Other
reported HDR inputs were not inspected and are not assumed invalid.

Extended CPU schema probe covers explicit/legacy selection, ignored precedence,
removal/default, disabled/detached/foreign sources, new selection, root release and
image alias/clearing, disabled auto-selection and stale-root reset. ResourcePicker
Clear is metadata opt-in; cancelled Browse preserves its selected resource and HDR
filters are `.hdr`/`.exr`; automatic selection skips disabled branches. Fresh Open
resets the generator source cache to reread an
identical path. The Composer matrix covers requested runtime cases, invalid sources,
visible generation error, save/reopen and import rollback. Static signatures/ownership,
selection/publication/serialization/style/license/link/whitespace checks only; no
configure/build, shader compiler, C++ fixture, application/GPU or packaging run.
No CMake, shader, GPU API, native table limit or texture-count optimization changes
are made by this correction. M07-07 remains awaiting lead validation; M07-08 is
the unchanged conditional successor.

## 2026-10-21 — M07-07 accepted; M07-08 documentation delivered

The lead asks to implement the next step, accepting M07-07 and delivered
Environment/UI/loading/table/shadow corrections by continuation. Compilation/checks
are assumed successful under the agreed convention; backend/HTTP/package/numerical
coverage remains unspecified. Baseline HEAD 78943d5820e69363c942d71136f479a0df87128e,
clean working tree. No engine/source/shader/CMake change or build execution.

Added maintained Composer guide/tutorial, scene/root/custom-clone API and draw
import lifecycle. Updated app/DB/draw/tutorial navigation, ResourcePicker and
SceneEditors intent contracts, schema warning/skip guidance, MainLoop worker
ownership and safe-update lifecycle. Replaced contradictory Composer README
startup/panel/pending-bake text. Complete DB publication, per-item model exports
and partial interactive bake adoption remain distinct.

Actual source is newer than the historical review ABI: tracing v3 uses persistent
Vulkan material set2, query shadows v5 persistent alpha set2 and reserves (2,1,10).
Retained 16384 device/sampler clamping, neutral overflow and independent materials;
updated renderer reference and operative/pending owner notes rather than restoring
v2. See milestone_07_bg2e_composer/documentation.md for cumulative changed/retained
coverage, source mappings and future M08/M09/M10 owners. No page retired.

Static declaration/caller/worker/shader-contract, local-link/fence and whitespace
checks only; no new backend/runtime evidence. Lead should review maintained usage
and limits against current Composer. M07-08 awaits acceptance; M08-01 is linked as
successor with its entry condition. No later implementation started.

## 2026-10-21 — Lead-directed engine/plan reconciliation

The previous next-step instruction accepts M07-08 by continuation. The lead then
interrupts M08-01 investigation and explicitly requests an audit, prioritizing
engine code over the migration plan and preserving optimizations. No M08 source
implementation had been written. Do not treat the audit as M08-01 delivery.

Baseline HEAD 78943d5820e69363c942d71136f479a0df87128e with inherited uncommitted
M07-08 Markdown changes retained. [R01–R14 report](code-reconciliation-2026-10-21.md)
records alpha defaults/threshold/import policy, opaque instance selection, material
revisions/raster bindings, indexed/persistent native tables, current v3/v5 ABI,
completion-based acceleration reuse, temporary resource leases/pools, lazy temporal
signatures, conservative transparent regions, narrow typed update drain elision,
CPU/GPU profiling and bool-noexcept save/publication semantics.

Updated operative index/decisions/protocols/source anchors and concrete M08–M10
summaries/owning steps, including preservation/validation/documentation gates.
Conflicting current pending ABI/remaining-infrastructure instructions are corrected
directly; original completed review and delivery histories retain provenance.
Implementation and measured/native acceptance of independent optimization/profiling
projects remain distinct. No added migration step, engine/CMake/shader modification,
build, executable/probe, native runtime or performance capture.

Static source/declaration/native contract and plan path/link/fence/whitespace checks
only. Audit is delivered for lead review; the next implementation pointer remains
M08-01, not started, with a new continuation/implementation instruction required.

## 2026-10-21 — M08-01 standalone context delivered

The lead requests implementation continuation after the code/plan audit, accepting
that reconciliation and authorizing M08-01 only. Baseline HEAD is
78943d5820e69363c942d71136f479a0df87128e with inherited uncommitted M07 documentation
and audit changes preserved. M07-08 remains the last accepted implementation step;
no independent optimization/profiling native or measurement matrix is inferred.

Added bake StandaloneBakerContext/StandaloneLightmapBaker headers and standalone
implementation, exposed through bake/all.hpp. The context borrows an initialized
windowless draw Engine, owns Created/Initialized/SceneReady/Cleaned state and one
private serial slot; no camera/renderer/application/UI or production scheduler.
prepareScene captures loaded graph/light/material/environment sources; public
EnvironmentGenerator handles authored/default sources or callers provide explicit
resource snapshots. createBaker preflights required primitives/native TLAS. One
live target is released before the next. Every update owns synchronous submit/
completion and exact sample count; final composition/validation/readback precedes
output. Reset freezes settings only for the current scene; stale jobs cannot
revive. Cancellation, partial command failure and submit/wait failure never publish
pending output. Failed retirement blocks further work until successful cleanup.

Source checks include copied environment intent/native generations, material/
texture revisions, geometry/BLAS identity and enabled target membership. Existing
immutable table/completed-TLAS reuse and primitive pooling remain active. No alpha,
material ownership/default, capacity/native ABI, shared draw/gpu/render, CMake or
shader behavior is changed. The small implementation-only shared state/target Impl
keeps validity and staged GPU owners alive without hiding public pass extension APIs.

Added source-only manual fixture for exact counts/settings, cancel during recording,
partial-record failure/recovery, stale reset/update rejection, second target and
retained results; supplied backend/no-RT/environment/source/teardown checks. Static
source/API/native immediateSubmit/RecordingCompletion, ownership/dependency/style/
GPL/include/link/whitespace review only. No configure/build, shader compiler,
C++ fixture, application/GPU or package execution. Engine must outlive all target
and external GPU owners; caller controls graph updates and stopped producers.

Actual contract is recorded in milestone_08_lightmap_generator/standalone-context.md
and handed to M08-02/M08-04. M08-01 awaits lead validation; M08-02 is the linked next
step. Assembly, protected batch output and CLI are not implemented by this delivery.

## 2026-10-21 — M08-01 accepted; M08-02 assembly delivered

The lead requests the next step, accepting M08-01 under the continuation convention;
compilation/checks assumed successful, backend/test scenarios unspecified. Entry
HEAD remains 78943d5820e69363c942d71136f479a0df87128e with inherited uncommitted M07
docs, plan audit and M08-01 source preserved. No compilation/runtime check is run.

Added bake StandaloneBakeSceneAssembler header/source, exposed through bake/all.hpp,
and refined the unused SceneAssembler forward declaration to the actual class name.
Assembly returns a private context+target graph, targetRoot, source paths, stable
sanitized hierarchy/sibling output identities and per-target UV2 diagnostics/atlas
metadata. Shared scene factories preserve unknown-component warning/skip and real
registered failures. A small single-Node representation adapter uses the engine
JSON parser; there is no parallel component or geometry file parser. BG2/VWGLB/OBJ
reuse CPU DB readers, preserving materials/groups/visibility and model chain joints.

Prefab transfer releases the former Scene root, unwraps only one synthetic single-
root level and retains multi-root containers. Camera-less inputs work. Context
geometry is source-only, enabled target branches are selected, helper Drawables
are removed without a manipulation dependency. Required source/geometry/UV/transform/
material/light validity fails explicitly. UV overlaps warn and remain eligible.
Initial UV2 is generated/validated on independent CPU replacements before first
upload; already realized custom targets reuse generateUv2AndReload. No input files,
earlier assembly or live context are mutated by a failed candidate.

Optional target-resource false leaves CPU target meshes/material references while
context sources are realized. Explicit later targetRoot.loadResources is required
before prepareScene. Custom formats retain virtual GPU hooks, require CPU geometry
for CPU-only preparation, and may not discard prepared UV2 during final realization.
Engine/Device outlive all assemblies/resources; caller coordinates producers and
component updates before freezing scene jobs. No render/draw/gpu implementation,
alpha/default/material ownership, shader/table policy or CMake is changed.

Added source-only assembly fixture for camera-less/single/multi-root inputs, joints,
CPU/deferred/GPU realization, helpers, null/unknown components, deterministic identities,
clone independence, registered failure isolation, source bytes and real overlap.
Static API/schema/ownership/resource/UV/include/GPL/style/link/whitespace checks only.
No build, shader compiler, fixture executable, application/GPU or packaging run.

Actual contract is in milestone_08_lightmap_generator/scene-assembly.md. M08-03/M08-04
handoffs name actual APIs and the remaining OBJ MTL/custom external dependency-
discovery/protection obligation: known sourcePaths is not complete batch preflight.
M08-02 awaits lead validation. The required successor is M08-03; no batch writer,
map export, CLI or later milestone implementation is started.

## 2026-10-21 — M08-02 accepted; M08-03 batch/output delivered

The lead requests the next step, accepting M08-02 under the continuation
convention; compilation/checks are assumed successful, backend/scenario coverage
unspecified. Starting HEAD remains 78943d5820e69363c942d71136f479a0df87128e;
inherited plan reconciliation, documentation and M08-01/02 source are preserved.

Implemented bake::StandaloneBakeBatch and bake::LightmapOutputWriter. Frozen
serial jobs reuse the standalone context and actual completed typed readback;
callbacks can cancel even on the final completed sample without publication.
Already realized custom hooks are not repeated, target identities/CPU-uploaded
geometry and Engine ownership are checked, and sources are never adopted between
targets. Preflight supplements assembly dependencies with original OBJ/MTL files
from shared parser metadata and requires complete custom component declarations.
Canonical/ancestor/equivalent-file checks protect sources and output namespaces;
file fingerprints and scene/resource signatures reject changed jobs.

RGB8 lossless output is decoded/compared (explicit JPEG is lossy); RGB32F preserves
exact HDR bits in a CPU-written native RGBA32F archive. Detached model clones use
linear UV2 baked AO/GI and actual generated pixels, snapshot all native resources
through the checked bool-noexcept complete DB writer, and retain source assets.
All artifacts are private until final-manifest publication. Cooperating writers
use an exclusive directory lock; complete unique generations precede the atomic
manifest switch. Old generations remain intact; exceptional final publication
can leave an unreferenced generation. Crash recovery/garbage collection and
power-loss durability are explicitly outside the contract.

Generic DB additions preserve old signatures and production writer/CLI behavior;
no native rendering algorithm, shader, material default or CMake file is changed.
Static source/contract/include/license/style/archive/order/link/whitespace checks
are recorded in batch-output.md. Added opt-in CPU and stopped-Engine probes plus
a lead-run native matrix; none was compiled or executed. Updated M08-04/05 and
summary with actual APIs, manifest layout and validation boundaries. M08-03 awaits
lead validation; M08-04 is linked next and has not been started.

## 2026-10-21 — M08-03 accepted; M08-04 draw CLI delivered

The lead requests the next step, accepting M08-03 by the continuation convention;
compilation/checks are assumed successful, backend/scenario coverage unspecified.
Starting HEAD is 78943d5820e69363c942d71136f479a0df87128e. Inherited audit,
documentation and M08-01/02/03 source changes are preserved.

Implemented apps/draw/lightmap_generator with its own private parser and
DrawOffscreenApplicationDelegate. It preserves useful production CLI arguments
and defaults while adding backend selection, typed CPU output, explicit model
copies, AO radius/bias/exposure/dilation/atlas padding, shader-root and validation
controls. The SettingsCodec binds actual batch LightmapSettings under a declared
standalone identity; explicit CLI values override import regardless of order.
Unknown/unavailable fields reject or receive validated skip warnings; incompatible
renderer/schema/type identities always reject. Preferences/camera-only effect
profiles are not silently applied. Config and explicit shader roots are protected.

The app runs no camera frames: its 1x1 depthless context owns only initialization,
while finish runs public assembly and standalone batch work using requested UV
resolution. All GPU/scene owners release before Engine cleanup. Signal handlers
only store SIGINT/SIGTERM cancellation; native cleanup remains in the normal
OffscreenApplication path. Results report the authoritative manifest after cleanup,
with explicit 0/1/2/130/143 status and no skipped-required-target success.

M08-04 explicitly names apps/CMakeLists.txt/new target creation in its selected
scope: added that target and registration only. Existing bundle_app paired shader
refresh and recursive package discovery are reused; shared CMake/scripts and the
production CLI/native algorithms/shaders are unchanged. Native shader/deployment
validation remains the lead's responsibility. Added app guide, typed example,
opt-in CPU parser/config probe and executable/native matrix; none was compiled or
run. Static source/API/include/GPL/style/delimiter, legacy flag coverage,
config/lifecycle ordering, resource-rule, local link and whitespace checks passed.
Updated M08-05 with actual checks and limits. M08-04 awaits lead validation;
M08-05 is linked next and has not been started.

## 2026-10-21 — M08-04 accepted; M08-05 product validation delivered

The lead requests the next step, accepting M08-04 under the continuation
convention; compilation/checks assumed successful, backend/scenario coverage
unspecified. Baseline HEAD remains 78943d5820e69363c942d71136f479a0df87128e, with
inherited documentation/audit/M08 implementation retained.

Compared preserved production CLI/parser/batch/writer behavior with the delivered
draw/bake product. Required model/prefab/context/UV2/reset/accumulation workflows
are mapped; strict required-target failures, settings domains, final-manifest
publication, native float format/material semantics and final-sample cancellation
are explicit behavior changes rather than false flat-output parity claims.
Recorded P01–P11 independent output/reload/protection/interrupt/deployment and
current optimized/profiling regression requirements with native evidence pending.
No native implementation rollback or new algorithm was justified by this audit.

Added a read-only Python standard-library artifact inspector for committed
manifest paths, unique complete generations, metadata and PNG/native RGB32F
pixels. Optional BG2 material-section inspection checks local native sidecars and
actual linear UV2 baked map RGB against the standalone image, independently of
writer checks. It deliberately does not parse geometry, initialize an engine,
prove chart masks/lighting, preserve source hashes or execute cancellation jobs.
JPEG/BMP/TGA/large files require other independent decoders. Ten synthetic CPU
file tests passed, including PNG filters/CRC, HDR100000, orientation/multiple
identities, native material sidecars, malformed/truncated metadata/payloads,
path escapes/symlinks and source enum ordinals. These are inspector self-checks,
not engine/backend output artifacts or native reload evidence.

Updated preserved production CLI docs to distinguish old limits from draw/bake,
PACKAGING with actual target/resource/manifest checks, summary and M08-06/M09-01
handoffs. No C++/shader build, engine app/native probe, CMake configure, package
or profiling execution occurred; no native/production/shader/CMake implementation
was changed. Static source/format/protocol/resource-rule/link/whitespace checks
passed. M08-05 awaits lead validation under the existing protocol; M08-06 is
linked next and has not been started. No M09 retirement is authorized here.

## 2026-10-21 — M08-05 accepted; M08-06 technical review delivered

The lead requests the next step, accepting M08-05 under the continuation convention;
compilation/checks assumed successful, backend/scenario details unspecified.
Starting and finishing HEAD remain 78943d5820e69363c942d71136f479a0df87128e, with
all inherited documentation/audit/standalone implementation preserved. Host reports
Darwin; no SDK/device/runtime/performance query or product execution was performed.

Reviewed actual public standalone states/generations, Engine/Scene/native lifetime,
exact internal recording completion, public effect injection and immutable reuse,
UV2/schema/custom hooks, serial batch/source freezing, typed pixels/metadata,
cloned complete model resource saves, generic DB helpers and final-manifest atomic
publication. Recorded real CLI/config/bundle identities and all requested earlier
products. Classified naming/equivalent refinements versus stricter target failure,
manifest/native-format changes and verification/documentation gaps. Explicitly
checked R01–R14 source-preservation impacts without modifying native algorithms.

Built a refreshable per-file candidate retirement inventory: 1338 source/build/
resource files, 316 direct textual legacy matches; comments and whole retained
trees are included. It is not a transitive dependency graph or deletion approval.
Concrete incoming/replacement rows cover shared app/scene/DB/UI/umbrella production
branches, native Metal/Vulkan ImGui seams, actual old DB writer, root bake export
gap, app/example/test registrations, templates, paired shaders and neutral IO.
Updated M08-07 and every affected pending M09/M10 step plus root source/decision/
requirement/index handoffs and summaries. M08-07 owns the stale public bake index
and missing maintained standalone reference/tutorial/application authoring.

Static checks covered 25 M08-owned/modified C/C++ files (full GPL, project includes,
lexical delimiters/control braces plus structural namespace/function/group/include
review), candidate path/disposition/uniqueness/count/hash, Python syntax, local
links/fences and git diff --check. No style defect requiring source changes or
known native implementation failure was established. Earlier ten synthetic
inspector tests remain their original M08-05 evidence; no tests were rerun here.
No CMake configure/build, shader compilation, app/native probe/package operation,
source deletion or engine behavior change occurred. M08-06 awaits lead validation;
M08-07 documentation is the linked successor, with M09 still unstarted.

## 2026-10-21 — M08-06 accepted; M08-07 cumulative documentation delivered

The lead requests the next step, accepting M08-06 under the continuation convention;
compilation/checks assumed successful, backend/scenario detail unspecified. HEAD
remains 78943d5820e69363c942d71136f479a0df87128e; inherited source, docs/audit and
all current optimizations are preserved. This step changes documentation only.

Authored nine maintained pages: actual standalone context/baker/assembler/batch/
writer APIs, generic DB file/native archive IO, model/prefab C++ tutorial, full
new CLI/config/resource guide and ownership/publication architecture. Updated
22 maintained pages/indexes, including the stale bake/draw availability claims,
production/new writer distinction, integrated output/material/precision guidance,
app/offscreen/ShaderLibraries navigation and safe source freezing. Corrected
current complete DB bool-noexcept false handling, automatic destination parents
and exclusive-reserve/copy fallback rather than requiring hardlink support.

The coverage ledger maps all 31 affected pages to actual headers/source/callers,
retained M01–M07 foundations and authoring dispositions. Explicit bake include,
retained Scene/Engine/native lifetime, one live baker, immutable completed reuse,
UV versus camera samples, custom dependency limits, typed settings identity/CLI-
last precedence, exact native float output, model-copy scope, final-manifest
publication and cancellation/crash boundaries are documented as implemented.
No future final accessor/umbrella/promotion or ResourceManager/extra texture slot
is taught as available. Production reference pages remain scoped and retained.

Static checks covered 409 local targets/anchors across affected pages, project
include paths, fences/JSON, all 25 parser option names, current API/default/member
and tutorial ordering, save/precision/build-script source comparison and whitespace.
No snippets were compiled and no tests/app/native/GPU/package operation ran.
Earlier ten synthetic inspector tests retain their original M08-05 scope; no
backend/hardware execution detail is invented. Updated review closure, M09/M10
coverage handoffs and progress. M08-07 awaits lead acceptance; M09-01 inventory is
linked next and M09 deletion/promotion has not started.

## 2026-10-21 — M08-07 accepted; M09-01 manifest delivered

The lead requests the next step, accepting M08-07 cumulative documentation under
the continuation convention; compilation/checks assumed successful, backend and
scenario coverage unspecified. Actual entry HEAD is
1889bdb12ade85d820eaab4a7365760fa9c34f50 with a clean working tree.

Created a reproducible tracked-source inventory and per-file remove/migrate/keep/
relocate manifest: 1620 files, all 1338 historical M08 candidates plus 282 newly
covered files; 364 files with expanded direct legacy signals, 4329 resolved
lexical includes and 6365 reference/native/registration evidence rows. Per-source
hashes, incoming consumers and M08 disposition comparison preserve reviewability.
This is not a semantic graph or executable deletion list. Shader include resolution
respects each paired tree rather than confusing same-name legacy/draw helpers.

Recorded ordered replacements for shared scene/DB/UI/manipulation/app APIs,
umbrellas, old apps/examples/tests, native ImGui/device-selection/SDL seams,
shader/package/template consumers and blue-noise data relocation. Selected final
DrawableComponent::drawable() for draw objects, sole draw renderDelegate(), draw
MainLoop defaults, retained typed DrawOffscreen names and current draw_* targets/
stored identities. Neutral DrawableRegistry/CPU atlas/input/IO/data behavior,
author assets, SDK/native backends, SMAA bytes/licenses and R01–R14 survive.

Updated M09-02–05 and summary/index with concrete ownership and M08 documentation/
P01–P11 evidence boundaries. Static source/declaration/build/resource/template,
manifest coverage/paths/hashes/endpoints, Python syntax, links/fences and whitespace
checks only. No C++/shader/CMake/app behavior change, source deletion, configure,
build, fixture, package, GPU/native run or measured performance result. M09-01
awaits lead validation; M09-02 is linked next and has not been implemented.

## 2026-10-21 — M09-01 accepted; M09-02 canonical entrypoints delivered

Lead continuation accepts M09-01; compilation/checks assumed successful, backend/
scenario details unspecified. HEAD remains 1889bdb12ade85d820eaab4a7365760fa9c34f50
with inherited uncommitted M09-01 artifacts preserved.

MainLoop defaults now use draw EngineConfig; Application stores only a draw graphics
delegate under renderDelegate. Explicit backend overrides remain. Removed production
window execution and offscreen config/delegate/mode/native branch, retaining the
existing draw init/run/completion/cleanup sequence. Retired the old render offscreen
adapter pair/export to close its dependency on removed app types. No forwarding shim.

Removed 53 old app implementation/build/shader files and app registrations. Retired
legacy example registrations required by removed app contracts; their sources await
M09-05. Surviving draw/gpu products keep names/resources/preferences/config/import
identities. Ported the existing registered input display to draw and ordinary run.
Only app/example registration CMake changes required by the selected step; no root/
lib/platform/package/shader build rules changed.

Eleven maintained entrypoint/reference/architecture/tutorial pages updated; offscreen
probe guide uses surviving products. A 73-row before/after source/build hash ledger
records actual modifications/removals; historical M09-01 TSVs preserved and generator
allows explicitly recorded missing files. Static 14-file GPL/include/delimiter and
structural style review, retained lifecycle equivalence, unchanged draw frame/cleanup
and app identity comparison, supported reference/registration, docs/hash/whitespace
checks only. No configure/build, shader compiler, native/test/app/package execution
or measured result. R01–R14 and P01–P11 remain protected.

Actual contract, removal-order refinements and lead checklist are in entrypoints.md.
M09-02 awaits validation; M09-03 shared API cleanup is linked next.

## 2026-10-21 — M09-02 accepted; M09-03 shared APIs delivered

Lead continuation accepts M09-02; compilation/checks assumed successful, native
backend/scenario coverage unspecified. HEAD remains
1889bdb12ade85d820eaab4a7365760fa9c34f50 with inherited uncommitted inventory and
entrypoint changes preserved.

Shared scene/factory/DB/utils/manipulation/UI APIs now use draw resources only.
DrawableComponent::drawable and canonical weak selection/gizmo accessors replace
dual storage/modes. CPU schema/default constructors, root transfer/independent
clones/projections, chains, helper exclusion, selected environment, bounds and
ordinary visitors remain. Explicit reference IO and checked bool-noexcept complete
resource saves remain distinct. Specialized DB output/native widgets and scene
resources/bindings/visitors are retired; root all.hpp exports bake.

The delta has 107 modified/187 removed/two added code files. 153 removed rendering
consumers close dependencies on retired scene APIs, moving only that closure
forward; 23 render/native files remain M09-04. Generic UI lifecycle no longer has
production callbacks/closures; native ImGui/SDL and standalone selector OpenGL
confinement remain next. GPUSelectionDialog accepts explicit generic GPU properties.
Registered UV/bake targets reuse public probes with canonical windowless ownership;
original fixture-specific coverage comparison remains M09-05/06. No CMake, package
implementation, shader/ABI, material policy or new rendering algorithm changes.

Updated 25 maintained pages and added two current API/ownership pages, plus probe
guides. Static 109-file source style/license/include/delimiter checks, 273 protected
GPU/base/draw byte comparisons, 40 CPU/parser/clone/complete-save function comparisons,
retired-symbol/public-native/member/include checks, delta hashes and docs/whitespace
checks only. No configure/build, shader compiler, fixture/app/GPU/package or timing
execution. R01–R14 and P01–P11 evidence limits remain explicit.

Actual contracts, source/data compatibility boundaries and lead matrix are in
shared-api.md. M09-03 awaits validation; M09-04 is linked next and has not started.


M09-04 delivery: M09-03 accepted by project lead continuation instruction with
backend/scenario coverage unspecified. Native confinement and final render source
retirement implemented; source checks only. M09-04 awaits lead validation and
M09-05 is the linked successor.


M09-05 delivery: M09-04 accepted by project lead continuation instruction with
backend/scenario coverage unspecified. Canonical examples/shaders/standalone/
distribution and registered regression ports implemented; source checks only.
M09-05 awaits lead validation; M09-06 is the linked successor.


M09-06 delivery: M09-05 accepted by project lead continuation instruction with
backend/scenario coverage unspecified. Final source gate and concrete native
regression handoff delivered; no runtime or build-system changes. M09-06 awaits
lead acceptance; M09-07 technical review is next, followed by M09-08 documentation.


M09-06 build correction: lead-reported macOS arm64 Clang failure in db/scene.cpp
(countSceneNodes undeclared; JSON conversion diagnostic was cascading). Restored
the original neutral recursive node-count helper before saveSceneReferences;
no API/CMake/format change. Historical deltas retained, correction hash chain and
final source evidence updated. Static checks only; correction awaits lead validation.
Next step remains M09-07, not started.


M09-06 second build correction: relocated gpu/metal/GuiRenderer.cpp lacked the
IMGUI_IMPL_METAL_CPP definition formerly supplied to the UI source only. Enabled
Metal-cpp declarations locally under the macOS guard; vendor implementation already
has matching C++/ARC build flags. Vulkan declarations do not need that opt-in.
Correction ledger/native/final source audits updated; no compile/native execution.
Scene counter and Metal GUI corrections await lead validation; M09-07 stays next.


M09-06 third build correction: lead's arm64 shared-library link lacked VMA
implementation functions. Retirement had deleted the sole VMA_IMPLEMENTATION
owner. Moved its unchanged implementation/configuration block to global scope in
existing gpu/vk/Device.cpp. No new source registration/CMake or public API change.
Native audit now checks exactly one VMA implementation owner; correction hashes
and final source evidence refreshed. No compiler/linker execution by agent.
M09-06 corrections await full build validation; M09-07 stays next.

### M09-07 — Final production review delivery

Reconciled final public contracts and retirement evidence, updated all six M10
steps and cumulative M09-08 authoring handoff. Original evidence snapshots and
inherited implementation remain preserved. M09-06 corrections accepted by lead
continuation; exact scenario coverage unspecified. M09-07 awaits validation.

### M09-08 — Cumulative documentation delivery

M09-07 accepted by continuation. Updated API/tutorial/app/concept/build/navigation
guidance to final production draw/bake, retired obsolete render reference pages,
added final migration/product/extension coverage and reconciled every page hash.
Static checks passed without build/runtime execution. Await lead acceptance,
then enter M10-01 final documentation inventory audit.

### 2026-10-22 — M09-08 accepted; M10-01 inventory audit delivered

Lead continuation accepts M09-08 and closes M09. Entry HEAD is
`e2a32164a781dbf420a4464b25ddce9feb862c6a`, initially clean. Reconciled all
232 original M09 handoff rows plus nine additions and twelve removals against
229 actual maintained pages, hashes, source and audit owners. M01–M09 reports
supply 216 explicit historical page mappings, with grouped foundation coverage
preserved. Module discovery maps 317 public headers; current product coverage
maps all 33 registrations and guides.

Later app relocation and target renaming are incorporated without rewriting
historical M09 evidence. Twenty-one pages had already changed with that commit.
Corrected two additional stale bake descriptions claiming the removed DB writer
remained available. No substantive authoring gap identified at inventory depth;
per-symbol, tutorial, build and consistency audits remain M10-02–05. Updated their
current-source handoffs and final-review inputs. Optimization/profiling references,
settings identity and independent evidence limits remain explicit.

Static page/hash/disposition/source/header/product checks and 1,870 local links/
anchors plus 111 project includes pass, with two existing illustrative-header
exceptions. No engine/shader/CMake/script implementation change or configure,
compile, native/test/package/output/performance execution. M10-01 awaits lead
validation; M10-02 is linked next and has not started.

### 2026-10-22 — M10-01 accepted; M10-02 API reference audit delivered

Lead continuation accepts M10-01 including the two writer-description corrections.
HEAD remains e2a32164a781dbf420a4464b25ddce9feb862c6a with inherited uncommitted
M10-01 artifacts preserved. Reviewed final renderer/upscaler/frame ownership,
live catalog/codec/facade, clone/root/factory/DB boundaries, manipulation/material/
preview/UI, native capability/GUI contracts, integrated/standalone bake and
R01–R14. Actual source overrides historical review names and coexistence.

Recorded 37 contract families across 55 maintained pages with 115 hashed source
anchors. Corrected 18 pages: stale production/DB/glTF/preview availability,
seconds delta, stored/live material divergence, generic settings identity and
opt-in automatic persistence, raw-executor denoise rejection, borrowed ImGui
versus owned GL context, device-clamped capacity and completed acceleration reuse
without refit. Clarified renderer/upscaler lifetimes, alpha/default/import routing
and RGBA8/D32 analysis; linked existing optimization/profiling API references.

Static contract/declaration/metadata/source/path/hash checks and maintained audit
pass: 229 pages, 1,881 local links/anchors, 111 includes and two retained illustrative
header exceptions. All 1,276 step-entry non-vendored implementation/build/script/
test files retain hashes. No compile/configure/shader/probe/native/package/output/
performance execution; no new feature or CMake edit. Original M10-01 page/hash
artifacts remain historical; separate API delta/source/audit artifacts record this
step. No substantive authoring gap identified in the audited contract families.
M10-02 awaits lead review; M10-03 is linked next and has not started.

### 2026-10-22 — M10-02 accepted; M10-03 tutorial audit delivered

Lead continuation accepts M10-02. Entry/finishing HEAD remains
`e2a32164a781dbf420a4464b25ddce9feb862c6a`; all inherited M10-01/02 work is retained.
Audited 31 tutorial pages: 26 numbered draw/GPU examples, three application/API
workflows and two indexes, with source/target/registration mapping. Classified
426 C++/GLSL/Metal blocks: 350 literal-token matches ignoring comments/whitespace,
76 reviewed extracts/recipes/alternatives. Tutorial 12's 17 blocks and tutorial
13's 22 reconstruct complete main.cpp files ignoring whitespace.

Corrected 14 maintained pages. Draw 06 now teaches actual EnvironmentGenerator/
SkyboxRenderer ownership, bindings, sizes/mips/HDR and lifecycle instead of old
manual generation. Other repairs include Model Edit command, Sobel binding syntax,
null acquisition guards, ring storage initialization/borrowing/retirement, matrix
order, offscreen frame association, nonwaiting endFrame, generic history
invalidation before partial edits and current capability/customization limits.
Standalone native archive wording and staged-versus-literal assembly are clear.

Static source inspection found a shared mutable light/camera UBO overlap risk
in primitive GPU 11/13, distinct from production RendererDeferred. Documented the
limit and safe application ownership contract; no sample-source correction or
reproduced native failure is claimed. Added explicit source follow-up to final
review/open issues; continuation must not imply that risk was repaired.

All 1,276 step-entry non-vendored implementation/build/script/test files retain
hashes. Matrix/source/page/hash/link/whitespace checks pass; maintained audit has
229 pages, 1,891 local links/anchors, 111 includes and two existing illustrative
header exceptions. No configure/compile/shader/native/test/package/output/profiling
execution. M10-03 awaits lead review; M10-04 is linked next and has not started.

### 2026-10-22 — M10-03 accepted; M10-04 applications/distribution audit delivered

Lead continuation accepts M10-03. HEAD remains
`e2a32164a781dbf420a4464b25ddce9feb862c6a`; inherited M10 work is preserved.
Audited app/current target and preference identities, workspace/scene/import/
Chain/table/environment policies, integrated bake and complete bool-checked saves,
Composer live/recorded Performance ownership, all three app parsers and camera CLI.
Mapped 18 contract families across 17 guides to 50 source anchors, 38 parser option
rows (including all 25 valued bake options), and 292 paired stage output paths.

Corrected ten documents (nine doc/ pages plus scripts/PACKAGING.md): obsolete
production coexistence and app paths/default-template wording, Model Edit launch,
Composer HTTP menu/preferences/worker/publication, Performance, current batch
comparison, platform archive/dirty/overwrite/dependency and deployment behavior.
No engine/app/example/shader/CMake or executable script changes.

Found a source integration blocker: all four standalone creators/updaters check
an obsolete flat bake/uv_surface.frag filename, while current compilation preserves
uv_surface/surface.frag. Fresh canonical trees cannot pass the test; docs now state
that rebuilding alone does not fix it. Source correction remains an explicit
M09-05 follow-up for final review, alongside inherited GPU 11/13 shared mutable UBO
risk. Neither risk is reproduced/cleared by this documentation audit or acceptance.

Static source/declaration/CLI/path/hash/output-uniqueness and whitespace checks pass.
Fresh maintained audit covers 229 pages, 1,899 links/anchors and 111 includes with
two existing illustrative-header exceptions. All 1,275 step-entry non-vendored
files outside doc/ and PACKAGING retain hashes. No configure/build, shader/parser/
app/HTTP/native/package/install/creator/updater/output/profiling execution.
M10-04 awaits lead review; M10-05 is linked next and has not started.

### 2026-10-22 — M10-04 accepted; M10-05 documentation validation delivered

Lead continuation accepts M10-04. All 229 maintained pages reconciled to inventory
and reachable from doc/index.md after navigation repairs. Corrected 16 pages,
including joined async lifetime, safe-update/callback ownership, current tracing
ABI and native effect order, bake accumulation/publication diagrams and obsolete
names. Thirty exact source excerpts now match current tokens; six diagrams received
static semantic review. Final checks cover 1,913 links/anchors and 111 includes
with two declared illustrative-header exceptions; fences/JSON checks pass with
two documented trailing-comma examples. All 1,276 non-document entry hashes remain
unchanged. No compilation, runtime, package or performance execution. Historical
audits preserved. Shared mutable UBO risk and standalone shader preflight blocker
remain unrepaired source follow-ups. M10-05 awaits lead validation; M10-06 is next.

### 2026-10-22 — M10-05 accepted; M10-06 final review delivered

Lead continuation accepts M10-05. HEAD remains e2a32164 with inherited uncommitted
M10 evidence preserved. Final review reconciles all 43 requirements-map decisions,
R01–R14, nine M01–M09 handoff reports/216 explicit page mappings, 229 maintained
pages, 317 public headers and 33 products. Revalidated 373 stored hash occurrences
across 350 distinct source paths; maintained page hashes unchanged since M10-05.
Fresh maintained audit: 1,913 links/anchors, 111 includes, zero errors, two declared
illustrative headers. No source/shader/script/CMake/maintained-page edits or build,
runtime, package, output or performance execution. All non-plan entry hashes are
preserved. Review records intentional refinements/exclusions and evidence limits.
F01 shared mutable UBO risk and F02 unmet M09-05 standalone shader preflight remain
unrepaired source findings; documentation acceptance cannot silently resolve them.
M10-06 awaits final lead acceptance. No successor implementation step; the plan is
not unconditionally certified complete by this delivery.

### 2026-10-22 — Requested source corrections delivered after M10-06

The lead explicitly requested repairs to both recorded findings and corresponding
documentation cleanup. GPU 11/13 now serialize shared UBO access with waitIdle;
GPU 11 guards null acquisition and GPU 13 increments sample count only after
successful acquisition. Four standalone helpers now check the canonical paired
bake shader paths, including shell macOS Metal. No CMake/engine/shader changes.
Current guides and final review no longer describe these findings as open; earlier
reports/hash ledgers remain historical with superseding correction notes. Shell
syntax and isolated canonical/missing/obsolete-only preflight fixtures pass;
PowerShell paths and source ordering checked statically. No build/native/full
creator/updater execution. Corrections await lead validation; final acceptance
remains pending and no successor milestone is added.

### 2026-10-22 — GPU 13 reported crash corrected

Lead reported std::invalid_argument from copyImage validation in the RT pipeline
example. RGBA storage and the normally BGRA surface are not identical copy
formats. Replaced the copy with a surface-format fullscreen graphics pass and
paired GLSL/Metal display shaders, removed manual BGR swizzles, closed the Metal
RT compute encoder before transitions, and rebind display resources on resize.
Updated tutorial and final-review correction handoff; no engine/backend/CMake
changes. Static checks and maintained links/includes pass. No build or native
execution; lead must rebuild the example/new shaders and confirm launch/resize.
M10-06 corrections and final acceptance remain pending, with no successor.
