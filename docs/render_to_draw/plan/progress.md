# Shared implementation progress

[Plan index](index.md) · [Execution protocol](execution-protocol.md) · [Milestone review](review-protocol.md)

This is the execution record for all implementation chats. The **Next step to implement** field below identifies the step the next chat must open; the project lead does not need to supply its ID or filename. Each implementing agent must maintain that field according to the execution protocol. Do not infer accepted work from committed planning documents or pre-existing GPU/draw facilities.

## Current position

| Field | Value |
|---|---|
| Workflow state | `awaiting_project_lead` |
| Current milestone | M07 — Experimental bg2e_composer application |
| Active step | None — M07-01 delivered |
| Last implemented step | M07-01 — Application inventory and copy |
| Last accepted step | M06-08 — accepted by project lead continuation instruction |
| Awaiting project lead validation | M07-01 application startup and inventory |
| Next step to implement | [M07-02 — Complete scene graph IO, factories and independent cloning](milestone_07_bg2e_composer/step_02_scene-io-cloning.md) |
| Next action | `wait_for_project_lead` |
| Condition before starting | Project lead acceptance of M07-01; backend/package coverage remains unspecified. |
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

| [M07-01](milestone_07_bg2e_composer/step_01_application-copy.md) | `awaiting_project_lead` | HEAD bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d plus inherited changes preserved; new apps/draw/bg2e_composer app sources/README/target, apps registration and M07 inventory/handoff. | GPL/private includes/identity/draw boundary, resource helper, ownership and whitespace inspected; no configure/build/shader/runtime/package execution. | M06-08 accepted by project lead continuation instruction; backend/package coverage unspecified. | M07-01 awaits lead startup checks. Next M07-02; Composer-specific gaps mapped in application-inventory.md. |


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
