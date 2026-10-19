# M05 cumulative documentation coverage

[Plan](../index.md) · [Progress](../progress.md) · [Technical review](review.md) ·
[Documentation step](step_07_documentation.md)

Delivery date: 2026-10-18 (Europe/Madrid). Starting/ending HEAD:
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`, with the inherited M04/M05 working
tree preserved. M05-06 is accepted by project lead continuation instruction;
backend/scenario evidence remains unspecified. Work resumed after an intentional
turn interruption and kept the partially updated progress record. This step
changes Markdown only; no source, shader, CMake, commit/reset or build execution.

## Maintained page dispositions and source mapping

Paths below are relative to `doc/` except the packaging guide. Added pages extend
the existing canonical references; no parallel versioned API snapshot is created.
No page is retired: production render remains supported through M08.

| Page | Disposition | Source / implemented coverage |
|---|---|---|
| api/app/OffscreenApplication.md | Updated | app/OffscreenApplication.hpp/.cpp, DrawOffscreenApplicationDelegate.hpp, OffscreenExecution.hpp: actual typed init selection, complete config/defaults, generic completion/resolve hooks, lifetime/error ordering and production coexistence. |
| api/app/index.md | Updated | App navigation now exposes windowless draw jobs and the CLI. |
| api/app/reference.md | Updated | Config/delegate/mode entries added, production config anchor corrected after reference rewrite. |
| api/draw/DefaultOffscreenApplicationDelegate.md | Added | Header/source: constructor/scene owner, linear seam, live import/extensions, exact completed N, final hooks, mutation/time/history and cleanup. |
| api/draw/FrameAccumulator.md | Added | pipeline/FrameAccumulator.hpp/.cpp: every public method, double RGB sums, ignored input alpha, opaque resolve, RGBA16F bounds/transfer usages and reference cost. |
| api/draw/RendererDeferred.md | Updated | Renderer.hpp and RendererDeferred split record methods; current camera-average availability and links to maintained job/accumulator references. |
| api/draw/Settings.md | Updated | DefaultOffscreenApplicationDelegate::prepareFrame and RenderSettingsPreferences: stopped bind/import, explicit-document persistence bypass, overrides, IDs and diagnostic skipping. |
| api/draw/Engine.md | Updated | Engine.hpp/.cpp: context versus scene-job ownership; direct Device-backed headless usage remains distinct. |
| api/draw/EngineConfig.md | Updated | EngineConfig default BGRA versus the CLI's explicit RGBA8; no implicit conversion in typed readback. |
| api/draw/FrameContext.md | Updated | OffscreenApplication::runDraw: same slot/serial for final-only submission, no extra scene update/count, completed callback ordering. |
| api/draw/index.md | Updated | Correct obsolete production-only offscreen claim; link new public types/tutorial/CLI. |
| api/draw/reference.md | Updated | Correct coexistence description; catalog job bridge and accumulator. |
| api/gpu/OffscreenSurface.md | Updated | gpu/OffscreenSurface and both native implementations: actual Size2D constructor, virtual resource base, Device-created targets, native validity, one-slot completion, recreate/release and borrowed images. Replaces obsolete always-valid/thin-wrapper description. |
| api/gpu/Surface.md | Updated | Actual native Surface common implementation versus concrete window/offscreen frame behavior. |
| api/gpu/Image.md | Updated | Image.hpp and native staging: full-subresource raw readback, RGBA8/D32 typed restrictions, explicit final layout, lifetime and no conversion. Presentation example now identifies window targets. |
| api/gpu/Subresources_and_rendering.md | Updated | Retain raw transfer contract; correct earlier broad depth-readback exclusion to acknowledge the existing typed D32 path. |
| api/db/index.md | Updated | db/image.hpp and image_db.cpp: GPU-free writer/decoder versus completed camera capture and complete resource export. |
| apps/draw_offscreen_application.md | Added | Shipped example parser/init/completion/saveVerifiedPng and target: all options, defaults/precedence, scene assets, output path, exact codec check, hard-link publication/cleanup/filesystem limitations, errors and build/deployment. |
| apps/index.md | Updated | Navigation separates production tools, integrated lightmaps and camera-image CLI. |
| tutorials/draw/13_offscreen_application.md | Added | Example 13 main/initConfig/didRenderFrame excerpts plus runnable CLI workflow, settings import, exact sample/final policy and independent inspection expectations. |
| tutorials/draw/index.md | Updated | Tutorial 13 and actual delivered target. |
| architecture/draw_offscreen_rendering.md | Added | App/Engine/default delegate/accumulator: context/data flow, serial completion, exact camera mean versus temporal/UV counts, final output/color/alpha and raw transfer boundary. |
| architecture/index.md | Updated | Conceptual navigation to headless architecture. |
| architecture/MainLoop_render_draw_gpu.md | Updated | Correct obsolete render-only offscreen statement and link the separate headless execution path. |
| architecture/draw_integrated_lightmaps.md | Updated | UV bake remains separate; recognize delivered windowless camera accumulation without changing bake claims. |
| delegate_system.md | Updated | Retain production windowed tutorial; add actual draw headless callback/scene owner navigation. |
| deferred_render_model.md | Updated | Retain production pipeline reference; correct obsolete claim that windowless orchestration remains pending. |
| ../scripts/PACKAGING.md | Updated | Actual example target/resource-helper and draw shader/asset dependencies, with no unsupported assertion that every package variant contains it. |

## Retained cumulative coverage

The accepted [M04 coverage](../milestone_04_lightmaps/documentation.md) and
[M03 coverage](../milestone_03_ray_tracing/documentation.md) remain the baseline for
earlier foundations. Retention here means relevant scope/ownership was inspected;
it does not claim a fresh whole-engine API or numerical audit.

| Retained page family | Current source/coverage and reason |
|---|---|
| api/app/MainLoop.md and Application_and_input.md | Interactive Application/MainLoop, runtime execution selection and UI/input remain windowed. The new app reference links separate headless execution rather than rewriting this lifecycle. |
| api/draw/Resources.md and Environment.md | Existing independent draw resources/clones and HDR/IBL owners supply example scene construction. Camera capture does not materialize complete scene saves. |
| api/draw/TemporalAccumulator.md, DenoiseFilter.md, SMAAProcessor.md | Effect history/filter/final-AA semantics retained; new job references clarify that each sample can already be history-filtered and final SMAA runs once. |
| api/bake/index.md, IntegratedBakerContext.md, LightmapOutput.md and draw/BakedIndirectMaps.md | Integrated UV bake and typed RGB output remain independent of FrameAccumulator and camera PNG. No standalone/batch implementation is documented as delivered. |
| api/gpu/ShaderLibraries.md, Submission_tracking_and_waitIdle.md and GPU tutorials 03/06 | Native paired shader deployment, completion and direct low-level offscreen workflows remain valid; the draw app wraps them at a different layer. |
| safe_update_scene.md | Existing windowed queued edits and integrated bake retirement/generation discipline remain valid. Headless job import is a direct stopped boundary; it does not use MainLoop queue scheduling. |
| workspace_api.md, window_api.md and input_delegate.md | UI/window/input foundation pages remain interactive. No UI workspace, input routing or hidden window is introduced by offscreen jobs. |
| apps/lightmap_generator/index.md and architecture/lightmaps/ | Existing production bake CLI/context/batch/output references remain supported; the new camera CLI has its own guide and does not replace them. |
| apps/draw_lightmaps.md, lightmap_baking.md and tutorial 12 | Actual integrated draw bake/adoption/preview contracts retained; camera-image export is now linked from the shared navigation without changing UV sample/pixel meanings. |

## Static checks and evidence

- Checked 338 local links/anchors and balanced fences across 28 affected maintained
  pages, including the packaging guide. Checked the canonical production config
  anchor after the app reference rewrite.
- All three tutorial C++ excerpts are exact substrings of the shipped example
  main.cpp. The tutorial runs the registered target; excerpts are labeled as
  excerpts, not an independently compilable alternate implementation.
- Compared documented options/defaults/constraints with parseOptions/initConfig,
  callback guards with didRenderFrame, and publication/decoder/hard-link/cleanup
  behavior with saveVerifiedPng. File-system support and best-effort pending-link
  cleanup are explicit; codec equality does not certify visual orientation.
- Inspected config/delegate/renderer/accumulator headers and implementations,
  Engine headless accessors, native surface validity/frame lifecycle and typed
  raw readback; checked actual target/resource helper and referenced assets.
- Searched affected maintained pages for obsolete production-only/unavailable
  headless assertions and corrected the current statements while retaining
  historical planning records and accurate production usage.
- `git diff --check` passes. Source-style/license rules introduce no new or edited
  C/C++/shader files in this Markdown-only step.

No configure/build, shader compilation, C++ fixture, application/GPU, packaging,
install or visual image check was performed. Lead continuation accepts M05-06;
backend/scenario/artifact metadata remains unspecified. Vulkan/Metal availability,
no-RT/FSR fallback, actual file orientation/content and packaged runtime behavior
remain subject to the project lead's checks described in the CLI guide/review.

## Remaining owners and transition

| Owner | Remaining documentation/implementation boundary |
|---|---|
| M06 / M07 | Complete GPU-only texture/resource/scene saves, editor/composer/import/dirty-state/bake integration; update canonical DB/material references when those APIs exist. |
| M08 | Actual standalone bake context, scene assembly, UV sample scheduler, complete protected batch output and model copies. Camera PNG publication is no batch transaction. |
| M09 | Draw production promotion, final names/defaults, removal of production branches and obsolete application references, distribution/standalone reconciliation. |
| M10 | Audit cumulative links/snippets/final API/product dispositions; no remaining M05 bulk documentation authoring is deferred there. |

M05-07 is delivered awaiting project lead acceptance. Its continuation closes M05
and selects [M06-01](../milestone_06_model_edit/step_01_application-copy.md), whose
prerequisites now explicitly include M05-07 documentation closure. No M06 product
implementation is started in this delivery.


## Supplemental tutorial revision requested with M06-01 (2026-10-19)

The project lead accepted the next-step transition and explicitly requested a
from-scratch offscreen tutorial matching the numbered construction format of
tutorials 10 and 12. Tutorial 13 now has 22 consecutive C++ blocks that reconstruct
the complete current example main.cpp, starting with the full license/includes and
ending with main. Each step explains its engine APIs and required C++ ownership,
callback, math, configuration, scene/material, readback/codec and cleanup concepts,
assuming only basic C++. Platform build/resource setup and run/inspection steps
are included. The earlier three-excerpt check above is historical delivery
evidence; current validation compares all 22 blocks to the complete shipped
source modulo whitespace. No build/runtime/image validation is implied.
