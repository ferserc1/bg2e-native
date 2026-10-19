# M05 technical review — OffscreenApplication and accumulated scene export

[Milestone](summary.md) · [Progress](../progress.md) · [Documentation closure](step_07_documentation.md) · [Review protocol](../review-protocol.md)

## Baseline and product status

Review date: 2026-10-18. Starting source baseline: HEAD
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`, with the inherited M04/M05
working tree preserved. M05-01 through M05-04 were accepted under the project
lead's continuation convention; M05-05 was accepted by the continuation that
selected this review. Backend/scenario coverage for those acceptances was not
specified. No commit, reset, build, runtime execution or image inspection was
performed for this review. The current worktree contains uncommitted implementation
and documentation from earlier steps; this review does not attribute those
changes to a single revision.

The delivered product is the registered `draw_offscreen_application` CLI. It
renders the deferred example scene without a window/UI, requests an exact number
of completed samples, and publishes a verified `scene.png`. This is an
experimental draw path: `OffscreenApplication` still retains its original
production-render overload and `ProductionRender` default. No output image was
produced during agent validation, so actual file metadata, visual orientation,
backend behavior and numeric image evidence remain unreported.

Implementation anchors: the dual app contract and execution mode are in
`lib/include/bg2e/app/OffscreenApplication.hpp` and
`lib/include/bg2e/app/DrawOffscreenApplicationDelegate.hpp`; draw job settings are
in `lib/include/bg2e/app/DrawOffscreenApplicationDelegate.hpp`; the bridge is
`lib/include/bg2e/draw/DefaultOffscreenApplicationDelegate.hpp`. The serial driver
is in `lib/src/bg2e/app/OffscreenApplication.cpp`; accumulation and final-pass
hooks are in `lib/src/bg2e/draw/DefaultOffscreenApplicationDelegate.cpp` and
`lib/src/bg2e/draw/pipeline/FrameAccumulator.cpp`. Native transfer implementation
is confined to `lib/src/bg2e/gpu/vk/Image.cpp` and
`lib/src/bg2e/gpu/metal/Image.cpp`; backend viewport conventions are in the paired
`gpu/{vk,metal}/CommandBuffer.cpp` files. DB encode/decode remains in
`lib/src/bg2e/db/image_db.cpp`. The product caller is
`examples/draw/13_offscreen_application/src/main.cpp`.

## Planned versus actual contracts

| Area | Actual implementation and evidence | Classification / limit |
|---|---|---|
| Execution selection | `app::OffscreenApplication` keeps the production-render delegate overload and adds a typed `DrawOffscreenApplicationDelegate` overload. `OffscreenExecutionMode` reports `ProductionRender` or `ExperimentalDraw`; production remains the default. Draw init creates a windowless draw Engine and routes run/cleanup to the typed delegate. | Equivalent refinement to the planned explicit parallel path. It does not promote draw or remove production compatibility; that remains M09 work. |
| Public ownership | `DrawOffscreenApplicationDelegate` is a native-free public callback contract. `DefaultOffscreenApplicationDelegate` bridges the existing public `DefaultRenderLoopDelegate`, which owns one polymorphic Renderer and the same scene/replacement generations as the windowed draw path. Engine/surface/device outlive callbacks and cleanup. FrameContext and its command/images are borrowed during render callbacks. | Implemented. Custom delegates own their continuation policy; the default delegate owns the accumulation policy. |
| Headless lifecycle | Draw Engine initializes an Instance without a window and owns an OffscreenSurface. The app performs serial submissions; only a successful `immediateSubmit` advances completion and calls `didRenderFrame`. No SDL window, MainLoop, input or UI is constructed. Window-only UI/render-loop entry points reject a headless Engine. | Implemented. Current backend surfaces use one in-flight slot and execution is synchronous; this is not an asynchronous scheduler. |
| Samples and final processing | `DrawOffscreenApplicationConfig::frames` defaults to 1 and is validated in 1..1048576. The default delegate accepts exactly N completed linear camera samples. `FrameAccumulator` sums finite RGB in CPU double precision, ignores sample alpha, resolves output alpha to 1, and uploads one RGBA16F mean. The last sample is followed by one final-only color/reconstruction/SMAA submission; that submission is not another scene sample. | Implemented reference path. It incurs synchronous per-sample readback and CPU O(width×height) work, plus one upload. No performance claim is supported. |
| Time, effects and mutation | `timeStepSeconds` defaults to zero; first update delta is zero. History state resets before sample zero; AO/GI/reflection and FSR histories advance for each requested sample without hidden warm-up. Nonzero fixed timestep explicitly samples an animation sequence. The frozen default path checks scene/camera/material/light/renderer identity and rejects detected mutation; opaque writes to existing GPU allocations still require caller serialization or explicit invalidation. | Implemented with stated limits. Accumulated history-filtered frames are not independent raw Monte Carlo samples. Camera-frame averaging is distinct from `TemporalAccumulator` and bake UV accumulators. |
| Settings | `graphicsConfig`, optional borrowed persisted defaults, `unknownSettings`, fixed internal extent and scale are part of the job config. The explicit document bypasses persistence; resolution overrides take precedence and fixed extent disables scale policy. Import/configuration is applied at a stopped initial boundary before samples. Unknown IDs reject by default; explicit skip records diagnostics. `extendSettingsCatalog` is the app-owned mapping seam. | Implemented. This example does not map generated per-run light/environment IDs by name. Unsupported requested optional effects follow renderer fallback; no required-bake capability policy is implied. |
| Output/readback | The example explicitly requests `R8G8B8A8_UNORM`, reads only the final completed surface in the last callback and requires exact output extent/format. Existing `gpu::Image::readPixels` performs synchronous raw full-subresource readback for one initialized color mip/layer with TransferSrc; Metal strips staging row padding. `readPixelsRGBA8` accepts only R8G8B8A8_UNORM. No BGRA, sRGB or HDR conversion is supplied by that API. | Implemented reuse; no new GPU primitive was necessary. Generic raw readback covers other supported formats, but conversion/encoding must remain explicit at the caller. |
| PNG publication | DB `saveImage` writes the temporary PNG; `decodeImageRGBA8` reopens it. The CLI validates tight byte count, decoded dimensions and exact decoded RGBA equality, then creates a same-directory hard link at the final path (which fails rather than replacing an existing path) and removes the temporary. Exceptions remove the temporary. Existing final/temporary paths are rejected. | Implemented single-file no-clobber publication. This is not multi-file batch preflight, rollback, overwrite/replace support, or complete scene/model/resource serialization. A directory/permission failure is reported as an application error. |
| Orientation/color/alpha | Native readback copies rows in image order. The current Vulkan viewport uses a negative-height viewport from the top edge; Metal uses its upper-origin viewport. The PNG writer adds no row flip or color conversion and preserves the final readback alpha bytes. The accumulator itself resolves alpha to one before final rendering. | Source-backed orientation policy, but no independent asymmetric image was visually inspected on either backend. PNG round-trip proves encoder/decoder byte preservation, not that the rendered scene is visually upright. |
| Scene parity | The example reuses OBJ/submesh assets, materials, environment, lights and floor from the deferred example, with camera/light animation frozen. It excludes editor UI, selection, gizmos and overlays. N>1 averages completed linear-HDR reconstructed samples before final color/SMAA. | Intentional behavior differences, documented in the example README. Actual visual comparison with the interactive app remains unverified. |

## Requirements and evidence

Implemented: both execution overloads and headless Engine path; exact completed-sample scheduling; linear camera-image mean with final conversion once; direct settings import and precedence; no-window scene CLI; synchronous completed RGBA8 readback; checked PNG round-trip and temporary-output cleanup; explicit overlay/animation/comparison policy.

Not implemented by M05: complete GPU-only texture/material/model/scene saves; generic format conversion; multi-target bake scheduling; standalone bake context; bake batch transactions; async frame execution; production promotion of draw; deletion of legacy render/application paths.

Agent evidence for this review is static source/API/call-site/lifetime inspection and review of existing manual fixtures. The fixtures describe expected cases including N=1 and controlled 0.25/0.75 HDR scene inputs versus a 0.5 input, final-pass count, exact completed counts and reduced internal resolution; those fixtures were not run here. The CLI itself now verifies encoded PNG dimensions and exact RGBA bytes when run, but no run was performed for this review. There is no artifact metadata to report. The project lead's acceptance of M05-05 implies checks under the agreed convention, but reported platform, backend, scenario and visual evidence remain unspecified.

No source-style or API-boundary issue was found in the reviewed M05 implementation from the inspected files. `bg2e::gpu` remains independent of higher-level app/draw code; GPU-native Vulkan and Metal interop remains inside backend implementation files. M05 extends the shared OffscreenApplication wrapper but does not migrate or replace the production `bg2e::render` implementation; its production overload remains available.

## Pending-plan impact

| Contract change / actual API | Pending work affected | Review action |
|---|---|---|
| Full-subresource synchronous raw readback plus typed RGBA8 helper; example PNG is only a camera image | M06-02, M07-02, M07-05 | Reuse the correct raw transfer and completion lifetime, but preserve GPU-only texture format/mips/material metadata, complete graph saves, scene/job generations and unsaved state. Do not call the camera PNG path complete DB export. |
| Serial headless draw Engine and exact-N camera accumulation | M08-01, M08-03, M08 summary | Reuse context/surface/lifetime only. Keep standalone bake generations, UV sample coverage, LightmapPixels, HDR semantics and whole-batch protected publication independent. |
| Explicit production/draw OffscreenApplication branches; production remains default | M09-01, M09-02, M09-05, M09 summary | Inventory both branches, registered target, shader/assets and fixture. Migrate/preserve the requested product, then remove temporary selection and obsolete production products only after the accepted manifest/parity gate. |
| Interim CLI, settings, output format and bundled-resource behavior | M10-04 and M10 summary | Audit post-M09 canonical executable name and actual docs against final source; retain distinctions between camera image, full resource export and bake batch. |

Updated pending steps/summaries: `milestone_06_model_edit/step_02_complete-db-io.md` and `summary.md`; `milestone_07_bg2e_composer/step_02_scene-io-cloning.md`, `step_05_scene-baking.md` and `summary.md`; `milestone_08_lightmap_generator/step_01_standalone-context.md`, `step_03_batch-output.md` and `summary.md`; `milestone_09_cleanup/step_01_retirement-manifest.md`, `step_02_canonical-entrypoints.md`, `step_05_targets-tests-packages.md` and `summary.md`; `milestone_10_documentation/step_04_apps-build-distribution.md` and `summary.md`. M05 summary and progress record carry the review handoff. These are targeted handoff edits; other pending steps remain unchanged unless their own review establishes impact.

## Open verification and next gate

The project lead should build/run `draw_offscreen_application`, inspect asymmetric scene orientation and content/dimensions on available Vulkan/Metal, compare a one-sample result with equivalent interactive settings and overlays disabled, and exercise unwritable output plus pre-existing final/temporary paths. Backend and artifact evidence must be recorded only if supplied. Existing production offscreen/windowed paths remain regressions through M08.

M05-06 is delivered awaiting lead review. M05-07 is the next required step and closes the milestone's maintained documentation; review acceptance does not authorize entering M06.


## M05-07 documentation handoff (2026-10-18)

M05-06 accepted by project lead continuation; backend/scenario evidence remains
unspecified. [Cumulative coverage](documentation.md) records the five added and
23 updated maintained pages, retained foundations, source mappings and remaining
M06–M10 owners. Canonical references now cover the typed app/config, default scene
bridge and accumulator; new architecture, tutorial 13 and CLI guide explain exact
completed samples, import precedence and verified PNG output. Navigation and stale
render-only/always-valid surface descriptions were corrected. Production and bake
references retain their current scope.

Checks: 28 affected pages, 338 local links/anchors, balanced fences, three exact
example-source snippets, actual target/assets/API/lifetime/codec inspection and
git diff --check. No source/CMake changes or build/runtime/visual validation.
M05-07 is delivered awaiting acceptance. Its continuation closes M05 and selects
M06-01; this documentation delivery does not start that implementation.


## Tutorial construction follow-up (2026-10-19)

With the M06-01 continuation the lead additionally requested rewriting tutorial 13
into complete ordered application construction, assuming only basic C++ and
explaining the APIs. That follow-up is delivered with M06-01: all 22 consecutive
C++ blocks assemble to the current shipped example. This replaces the earlier
excerpt-only teaching format while preserving the actual APIs and product scope.
Static assembly/link checks are separate from unperformed build/backend/image
checks; M05-07 acceptance by continuation is preserved.
