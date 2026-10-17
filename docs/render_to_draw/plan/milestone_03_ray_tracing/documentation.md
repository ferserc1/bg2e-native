# M03-11 — Cumulative documentation closure delivery

Date: 2026-10-17. [Technical review](review.md) · [Progress](../progress.md) ·
[Documentation policy](../documentation-policy.md).

M03-10 is accepted by project lead continuation instruction; backend/scenario
coverage is not specified. Starting/finishing HEAD is
`15f67dff9efad51c878740bc1ddbce94f880ed46` with preserved uncommitted prior work.
This step edits documentation only, adds no CMake/source/features, and remains
awaiting project lead validation. It is the **last ordered step of M03**. M04
implementation starts only after this closure is accepted; no extra M03 review or
second documentation step is introduced.

## Changed maintained-page inventory

The maintained set through M01–M03 is updated in place, not replaced by a parallel
API snapshot. Five pages are added and twenty-four existing pages updated. No
maintained page is retired. Each row maps to actual declarations/implementations
or the existing example caller; source snippets were not compiled.

| Maintained page | Disposition | Actual source / reconciled coverage |
|---|---|---|
| [doc/api/draw/TracingSceneResources.md](../../../../doc/api/draw/TracingSceneResources.md) | Added | TracingSceneResources.hpp/.cpp and native tracing kernels: packed layout, bounded capacities, cutout/dense identity and completion ownership. |
| [doc/api/draw/SurfaceInputs.md](../../../../doc/api/draw/SurfaceInputs.md) | Added | RTAmbientOcclusion.hpp/.cpp and GI/reflection/temporal/denoise consumers: camera and explicit format/coverage/lifetime contract. |
| [doc/api/draw/MotionVectorGenerator.md](../../../../doc/api/draw/MotionVectorGenerator.md) | Added | MotionVectorGenerator.hpp/.cpp: TemporalView, unjittered camera motion convention, validation and explicit-profile limits. |
| [doc/architecture/draw_advanced_lighting.md](../../../../doc/architecture/draw_advanced_lighting.md) | Added | RendererDeferred graph, actual native capabilities, effect signals, histories, settings and final processing; production reference remains separate. |
| [doc/api/ui/GraphicsSettingsWindow.md](../../../../doc/api/ui/GraphicsSettingsWindow.md) | Added | GraphicsSettingsWindow.hpp/.cpp, facade and both UI delegates: explicit connect/bind/draw/reopen, live targets, queued edits and cleanup. |
| [doc/api/draw/RendererDeferred.md](../../../../doc/api/draw/RendererDeferred.md) | Updated | Renderer.hpp/RendererDeferred.hpp/.cpp and SoftShadowPass: shared components, graph selection, actual overflow fallback, lifecycle and final processor. |
| [doc/api/draw/Settings.md](../../../../doc/api/draw/Settings.md) | Updated | SettingsCatalog, SettingsCodec, RenderSettingsPreferences, EffectSettings, FsrUpscaler and example pose extension; exact IDs/schema/typed values, diagnostics and queue/reset behavior. |
| [doc/api/draw/DenoiseFilter.md](../../../../doc/api/draw/DenoiseFilter.md) | Updated | DenoiseFilter.hpp/.cpp and native kernels: live controls and absence of chart-ID isolation. |
| [doc/api/draw/RTAmbientOcclusion.md](../../../../doc/api/draw/RTAmbientOcclusion.md) | Updated | Matching draw/pipeline header/source and native kernels; live metadata, actual public surface/snapshot links and retained-owner lifetime. |
| [doc/api/draw/RTGlobalIllumination.md](../../../../doc/api/draw/RTGlobalIllumination.md) | Updated | Matching draw/pipeline header/source and native kernels; live metadata, actual public surface/snapshot links and retained-owner lifetime. |
| [doc/api/draw/RTReflections.md](../../../../doc/api/draw/RTReflections.md) | Updated | Matching draw/pipeline header/source and native kernels; live metadata, actual public surface/snapshot links and retained-owner lifetime. |
| [doc/api/draw/TemporalAccumulator.md](../../../../doc/api/draw/TemporalAccumulator.md) | Updated | Matching draw/pipeline header/source and native kernels; live metadata, actual public surface/snapshot links and retained-owner lifetime. |
| [doc/api/utils/BlueNoise.md](../../../../doc/api/utils/BlueNoise.md) | Updated | utils/BlueNoise.hpp/.cpp and RendererDeferred shared atlas ownership, injected-owner priority and hash fallback. |
| [doc/api/gpu/RayTracingPipeline.md](../../../../doc/api/gpu/RayTracingPipeline.md) | Updated | RayTracingPipeline and ResourceSet native implementations, linked separate high-level consumer; GPU dependency boundary retained. |
| [doc/api/gpu/ResourceSet.md](../../../../doc/api/gpu/ResourceSet.md) | Updated | RayTracingPipeline and ResourceSet native implementations, linked separate high-level consumer; GPU dependency boundary retained. |
| [doc/api/draw/index.md](../../../../doc/api/draw/index.md) | Updated | Current draw public headers and canonical maintained API navigation, preserving production/experimental coexistence. |
| [doc/api/draw/reference.md](../../../../doc/api/draw/reference.md) | Updated | Current draw public headers and canonical maintained API navigation, preserving production/experimental coexistence. |
| [doc/api/ui/Settings_Windows.md](../../../../doc/api/ui/Settings_Windows.md) | Updated | GraphicsSettingsWindow.hpp/.cpp, facade and both UI delegates: explicit connect/bind/draw/reopen, live targets, queued edits and cleanup. |
| [doc/api/ui/index.md](../../../../doc/api/ui/index.md) | Updated | GraphicsSettingsWindow.hpp/.cpp, facade and both UI delegates: explicit connect/bind/draw/reopen, live targets, queued edits and cleanup. |
| [doc/architecture/index.md](../../../../doc/architecture/index.md) | Updated | RendererDeferred graph, actual native capabilities, effect signals, histories, settings and final processing; production reference remains separate. |
| [doc/deferred_render_model.md](../../../../doc/deferred_render_model.md) | Updated | Existing production render/bake workflows retained; explicit boundary/link to separate draw implementation, no future draw baking taught. |
| [doc/lightmap_baking.md](../../../../doc/lightmap_baking.md) | Updated | Existing production render/bake workflows retained; explicit boundary/link to separate draw implementation, no future draw baking taught. |
| [doc/safe_update_scene.md](../../../../doc/safe_update_scene.md) | Updated | DefaultRenderLoopDelegate and Renderer::invalidateHistories; current generation, safe queued mutation and partial-failure reset requirements. |
| [doc/apps/draw_deferred_renderer.md](../../../../doc/apps/draw_deferred_renderer.md) | Updated | Existing example entrypoints, facade/window ownership, light editor and platform/deployment rules; scripts/PACKAGING.md retained as authority. |
| [doc/apps/draw_manipulation.md](../../../../doc/apps/draw_manipulation.md) | Updated | Existing example entrypoints, facade/window ownership, light editor and platform/deployment rules; scripts/PACKAGING.md retained as authority. |
| [doc/tutorials/draw/10_deferred_renderer.md](../../../../doc/tutorials/draw/10_deferred_renderer.md) | Updated | examples/draw/10_deferred_renderer and 11_manipulation actual factories/UI constructors, safe edits, current defaults and deployed shader requirements. |
| [doc/tutorials/draw/11_manipulation.md](../../../../doc/tutorials/draw/11_manipulation.md) | Updated | examples/draw/10_deferred_renderer and 11_manipulation actual factories/UI constructors, safe edits, current defaults and deployed shader requirements. |
| [doc/tutorials/draw/index.md](../../../../doc/tutorials/draw/index.md) | Updated | examples/draw/10_deferred_renderer and 11_manipulation actual factories/UI constructors, safe edits, current defaults and deployed shader requirements. |
| [doc/api/draw/SMAAProcessor.md](../../../../doc/api/draw/SMAAProcessor.md) | Updated | FinalProcessor/SMAAProcessor declarations, paired native kernels and official lookup headers; accepted versus unspecified verification status. |

## Retained cumulative coverage and remaining ownership

| Retained page family | Disposition / reason | Current authority |
|---|---|---|
| Draw Engine, EngineConfig, RenderLoop/Delegate, FrameContext, Resources, Environment, quick_start | Retain foundation/lifecycle/resource descriptions; canonical indexes now navigate new effects. No ownership/clone contract was replaced. | Matching draw headers/implementations and accepted M01/M02 documentation |
| GPU Device/Common/PipelineLayout/Image/CommandBuffer/ShaderLibraries, synchronization and primitive tutorials | Retain existing capability/format/native-binding/queue APIs; updated ResourceSet/RT pipeline pages link high-level consumers without moving their policy into gpu. | GPU public/native source and prior accepted primitive documentation |
| Manipulation/utils material/viewpoint/scene camera, draw material/preview UI | Retain typed weak selection, clone independence, queued partial edits and synchronous readback boundaries. New light editor uses existing wrappers/catalog, not a replacement subsystem. | M02 documentation and current example callers |
| Reflection, app/input/window/workspace/DB/math/geo/JSON foundations | Retain: no source API change in this closure. New API links resolve against their current files. No complete IO/editor migration is claimed. | Existing maintained pages and corresponding headers |
| Production render/deferred/bake/offscreen/application guides | Retain supported production workflows with explicit draw boundary links where affected. Existing OffscreenApplication remains production-only. | Production code/docs until M09 retirement |

M04 owns UV chart/surface generation, exact bake sample sums, chart-safe filtering,
actual baked RGB/presence/application and integrated example. M05 owns windowless
application execution and exact HDR frame averaging/export. M06–M08 own complete
IO/new editors/composer/standalone bake workflows. M09 owns production deletion and
canonical API/distribution reconciliation. M10 audits this accumulated set; no
M03 authoring work is deferred to that audit.

## Corrections and tutorial construction

Removed stale claims that advanced effects/manipulation were not delivered or that
visible denoise controls remained future work. Corrected deferred scale defaults,
legacy shadow overflow behavior, display histories versus FSR, and denoise guide
versus chart isolation. Added exact live target/schema identities and explicit
requested-state/capability separation. Described final SMAA after current final
color, with official lookup/native resources and no RT/FSR gate.

Tutorials preserve their ordered from-source construction rather than replacing
them with an overview or whole-file listing. Factories use current shipped values;
focused final sections add actual settings-window members/constructors, draw/reopen
calls and manipulation per-light metadata/editor. Existing selection/material/
camera explanations remain. Current source remains authoritative for complete class
bodies and cleanup. No new example target or future workflow snippet is introduced.

## Checks, acceptance and limits

- Static checks pass for 29 maintained Markdown pages: 344 real local links,
  referenced anchors, balanced code fences and 73 existing public/private source
  header references. Markdown code/lambda syntax is excluded from link parsing.
- Current example UI constructor snippets match source text. Factories, GI/reflection
  defaults, light pose target/getter/setter and connect/bind/draw/reopen names were
  checked against actual declarations/callers. New surface/motion/snapshot pages
  name real headers and members. Symbol checks are not compilation.
- git diff --check passes. No production text is deleted or recast as draw parity;
  native capabilities, SDK support and scene errors remain distinct.
- No configure/build, native shader compiler, example, fixture, image export or
  packaging execution by the agent. No new screenshot/performance/diagnostic
  evidence was supplied for this step. Existing lead continuation acceptance is
  recorded without invented backend coverage; manual matrix remains lead-owned.

Deliver M03-11 for normal acceptance. It is the final M03 step; after acceptance
progress selects [M04-01](../milestone_04_lightmaps/step_01_bake-contracts.md).
