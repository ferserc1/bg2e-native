# Shared implementation progress

[Plan index](index.md) · [Execution protocol](execution-protocol.md) · [Milestone review](review-protocol.md)

This is the execution record for all implementation chats. The **Next step to implement** field below identifies the step the next chat must open; the project lead does not need to supply its ID or filename. Each implementing agent must maintain that field according to the execution protocol. Do not infer accepted work from committed planning documents or pre-existing GPU/draw facilities.

## Current position

| Field | Value |
|---|---|
| Workflow state | `awaiting_project_lead` |
| Current milestone | M02 — Manipulation |
| Active step | None — M02-09 cumulative documentation delivered |
| Last implemented step | M02-09 — maintained API, tutorial, UI, input and application documentation |
| Last accepted step | M02-08 — accepted by project-lead continuation; M02-07 camera issue reported solved |
| Awaiting project lead validation | M02-09 documentation acceptance; final M02 Vulkan runtime validation remains outstanding |
| Next step to implement | [M03-01 — Advanced ray-tracing design checkpoint](milestone_03_ray_tracing/step_01_rt-design-checkpoint.md) |
| Next action | `wait_for_project_lead` |
| Condition before starting | Accept M02-09 and complete the outstanding Vulkan runtime check before starting M03-01. No build/runtime result is claimed by this documentation pass. Transparency remains deferred. |
| Record initialization baseline | `b955833f314d09a191de13b83c90cacc10666ae7`; working tree was clean before this workflow documentation update. |
| Validation owner | Human user / project lead |
| Agent build/runtime execution | Not performed; delegated to the project lead by default. |

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

## Open issues

The project lead reported a Metal deprecation warning in M01-03. The texture usage check now calls `isFramebufferOnly()` instead of deprecated `framebufferOnly()`; the corrected step was accepted by project lead continuation. M01-02 was accepted by continuation with unspecified backend coverage. M01-03 checks are assumed successful under the continuation convention; the expanded manual probe has not been run or registered in CMake. M01-04 checks are assumed successful under continuation, with unspecified coverage; M01-05 accepted by continuation with unspecified coverage. M01-06 accepted after lead compilation fixes; backend/scenario coverage unspecified. M01-07 accepted by project lead continuation, compilation/checks assumed successful and coverage unspecified. M01-08 accepted by continuation with unspecified coverage. M01-09 accepted by continuation with unspecified coverage. M01-10 accepted after lead compilation corrections and reviewed shader rename; runtime coverage unspecified. M01-11 accepted by continuation; compilation/checks assumed successful, backend coverage unspecified. M01-12 accepted by project lead continuation instruction, including its type-validation correction. M01-13 accepted by project lead continuation instruction; backend coverage unspecified. M01-14 accepted after the lead virtual Window destructor correction. M01-15 accepted by project lead continuation; backend/scenario coverage unspecified. M01-16 review acceptance is held for the reported M01-15 overflow correction. The M01-11 explicit alpha table supports eight distinct image/sampler pairs and throws on a ninth; M03 reconsiders arbitrary addressing. No known static failures remain; M01-13 upscaling and M01-12 CPU/persistence/lifecycle fixtures have not been compiled or run by the agent. RendererDeferred did not exist at M01-05: the manual minimal-renderer fixture covers the intermediate lifecycle, with the actual deferred/settings replacement required in M01-15.

## Decisions and handoff notes

- Implement one selected step per request, then stop for project lead validation.
- Compilation, shader compilation, runtime tests and build-producing scripts are delegated to the project lead by default; the agent performs relevant static checks.
- A project lead instruction to continue accepts the previously delivered step as compiled/checked without reported failures. Record that basis without inventing test details or asking for reconfirmation.
- A reported failure requires correction before advancing; deliver the corrected step for lead validation again.
- Maintain the explicit next-step link/action/condition at each handoff. The project lead can reference index.md and progress.md and simply request the next step. The last milestone review step is required and updates all affected pending milestones.

Future entries should record actual interfaces/paths/decisions and pending-plan changes needed by the next chat. Do not use this section to keep disconnected configuration values or replace architecture-decisions.md.

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
| [M02-09](milestone_02_manipulation/step_09_documentation.md) | `awaiting_project_lead` | Added cumulative references for manipulation and utils, draw UI wrappers, event forwarding, tutorial #11, app guidance and navigation; removed camera trace code from the example. | Static link/anchor/header/source/target checks and `git diff --check`; no build, shader compilation or runtime/backend check. | Documentation is delivered for lead acceptance. Lead reports the behavior is solved; Vulkan validation remains outstanding before M02 closure. | See [documentation handoff](milestone_02_manipulation/documentation.md). M03-01 is gated on M02-09 acceptance and Vulkan validation. |

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
