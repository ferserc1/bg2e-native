# M04 technical review — integrated lightmaps

[Plan](../index.md) · [Progress](../progress.md) · [Milestone](summary.md) ·
[Next: cumulative documentation](step_08_documentation.md)

## Baseline and acceptance

Review date: 2026-10-18 (Europe/Madrid). Starting and finishing Git HEAD:
`95190d6d042bb85d6e9d72153b155192dac64377`. The M04 implementation, application,
shader/deployment correction and planning updates are uncommitted working-tree
changes above that revision. Earlier uncommitted changes are preserved; no reset,
commit or checkout replacement was performed. This review changes Markdown only.

M04-01 through M04-05 were accepted by project lead continuation. M04-06 initially
stopped on integration assumptions, then the lead requested resolving the missing
integration. Its implementation added draw_lightmaps, material metadata and shader
application. The lead subsequently reported a missing Metal library at the bake
UV-surface path. That failure revealed omitted build/copy/install registration for
the bake tree. The correction was delivered and is accepted by the current lead
continuation instruction. Subsequent backend/scenario details are **not specified**.
Earlier reported M02 macOS Metal/Linux Vulkan coverage is not M04 bake evidence.

Actual product: [examples/draw/12_lightmaps](../../../../examples/draw/12_lightmaps/README.md),
registered target `draw_lightmaps`. Production APIs/apps remain alongside draw.
No application, C++ fixture, shader compiler, CMake configuration/build, package
or GPU runtime was executed by the agent for this review. No output image,
exported model, hardware timing or numerical backend artifact was provided.
The prior failure identifies a Metal bundle, but not successful numerical coverage
or the exact SDK/device used for M04.

## Planned versus implemented contracts

| Area | Actual source/API and ownership | Classification and implication |
|---|---|---|
| Settings | [LightmapSettings.hpp](../../../../lib/include/bg2e/bake/LightmapSettings.hpp): validated live accessors, settingsInstance/type ID `bg2e.bake.lightmapSettings.v1`; per-job frozen copy | Implemented. UI addresses the real idle object; paired bias/distance and dilation constraints are validated before a job. Example defaults differ explicitly from library defaults. |
| Abstract context/target | [BakerContext.hpp](../../../../lib/include/bg2e/bake/BakerContext.hpp), [LightmapBaker.hpp](../../../../lib/include/bg2e/bake/LightmapBaker.hpp): borrowed Device and retained scene/environment/configured passes; explicit source/target generations | Implemented. StandaloneBakerContext, SceneAssembler and Batch remain forward declarations, not executable services. |
| UV2 reload | [Uv2Reload.cpp](../../../../lib/src/bg2e/bake/Uv2Reload.cpp): generateUv2AndReload stages CPU atlas, validates, deep clones and realizes a replacement drawable | Implemented. Source geometry/material ownership stays independent; caller publishes replacement and refreshes queues, tracing, selection and previews at a stopped boundary. Modifier stays in geo. |
| UV world surfaces | [UvSurfacePass.hpp](../../../../lib/include/bg2e/bake/UvSurfacePass.hpp), UvSurfaceMesh: public Device-only depthless UV2 raster, four MRTs/32 attachment bytes, separate computed coverage | Implemented. Position RGBA32F, signed normal RGBA16F, submesh/chart R32_UINT, coverage R8. Caller supplies immutable owners and completed-slot/serial discipline. UV v=0 is the top row. |
| Shared effects | [LightmapBakeExecutor.hpp](../../../../lib/include/bg2e/bake/LightmapBakeExecutor.hpp): exclusive public draw AO/GI passes, matching-capacity tracing snapshots and copied environment owners | Implemented without a private bake estimator. Required profiles/formats fail explicitly; GI requires an environment snapshot and supported nonempty tracing scene. Injected chart-unsafe denoise is rejected. |
| Integrated scheduling | [IntegratedBakerContext.hpp](../../../../lib/include/bg2e/bake/IntegratedBakerContext.hpp): one live target and one prepared/in-flight frame; prepareFrame/record/completeFrame, explicit discard/abandon | Implemented serial policy. Only confirmed completed current work counts. Context does not own a camera renderer; current draw Engine initialization remains window-only. Shared internal state/target Impl hold lifecycle bookkeeping, while algorithmic passes are public/injectable. |
| Accumulation | [UvTemporalAccumulator.hpp](../../../../lib/include/bg2e/bake/UvTemporalAccumulator.hpp): fresh raw float sums and exact integer valid-update counts, separate from committed history | Implemented. Final validation rejects invalid/missing coverage/counts, numeric overflow and raw half-float GI saturation at or above 65504. It is not a camera-frame average or display EMA/FSR history. |
| Composition/output | [LightmapCompositionPass.hpp](../../../../lib/include/bg2e/bake/LightmapCompositionPass.hpp): normalized AO/GI, channelwise environment-reference division and exact nearest-original-seed padding | Implemented. GI is incoming diffuse radiance/pi divided by environment irradiance/pi; zero-reference channels give zero. Covered texels do not change during dilation, padding copies a seed without chart averaging, outside padding is neutral white. Final completion validates and caches typed pixels before publication. |
| Result/pixels | LightmapResult retains sampled RGBA32F image/extent/mode/UV2/generations; LightmapPixels is tight linear RGB8 or RGB32F | Implemented. RGB32F preserves finite nonnegative HDR; byte GI alone applies exposure then clamp/round. Alpha/RGB are never coverage. Final validation/readback is synchronous and can drain other submissions. |
| Preview | [UvMapPreviewRenderer.hpp](../../../../lib/include/bg2e/bake/UvMapPreviewRenderer.hpp): CPU UV raster and sampled RGBA8 upload; [BakeLightmapPreview.hpp](../../../../lib/include/bg2e/ui/BakeLightmapPreview.hpp): draw UI session snapshot retaining image/sampler | Intentional implementation change from proposed GPU geometry rasterization, accepted as delivered by continuation. Nonmutating UV1/UV2 diagnostics, submesh fills/wireframes/border; no preview shader family. Resolution/image replacement must rebind handles, and UI/Engine teardown must retire owners. Performance/scaling remains a verification gap. |
| Material application | [MaterialAttributes.hpp](../../../../lib/include/bg2e/base/MaterialAttributes.hpp), packMaterial, G-buffer/deferred shader pairs: explicit IndirectTextureMode through AO slot; JSON ao/bakedAO/bakedGI | Intentional six-slot contract refinement. Linear opaque UV2 RGB maps bypass scalar AO and live indirect AO/GI per marked pixel. GI preserves specular IBL/reflections; direct/emission stay separate. Scalar AO and a distinct baked map cannot coexist in this representation. |
| HDR display | GBuffer::bakedMultiplier is RGBA16F, six color attachments total 32 bytes/sample | Intentional format refinement to avoid UNORM clipping. Example rejects composed map values above 65504; full-float bake/CPU output remains distinct. Custom raster layers must match the actual contract. |
| Example completion | draw_lightmaps submits one sample synchronously in prepareFrame before acquisition, then calls completeFrame after immediateSubmit returns | Equivalent scheduling refinement; no window render-loop callback is needed. GPU sample cost can block UI until that sample finishes. Static scene, settings freeze, sequential targets, cancellation/restart and capture-all-before-adoption avoid source mutation during a batch. |
| Deployment | lib/CMakeLists and cmake/utils.cmake: draw/bake paired source trees, distinct per-tree targets/AIR paths, shared aggregate, incremental bundle copy and RuntimeShaders install | Corrected missing requirement, accepted by continuation after the reported missing-metallib error. Rebuild/installed/package path coverage remains unspecified; the source prefix is uv_surface. |

## Completed, pending and excluded work

Delivered M04 includes reusable settings/pixels, safe UV2 replacement, UV surface
and tracing bridge, integrated job lifecycle, exact accumulation/composition and
chart-safe dilation, finished sampled maps, explicit material adoption, UV/result
previews and an interactive example with live indirect passes disabled. Its
unbaked originals and white/red clone retain independent resources. Only floor
and opaque cube targets are baked; walls/emission and transparent display objects
have explicit restrictions. No hidden production renderer or duplicate RT shading
family was added to bake. Native interop remains behind gpu/UI backend APIs.

The example updates both stored material metadata and live GPU slots, but GPU-only
source settings can have an empty path. This is **not complete export**. The
following requirements are assigned to later steps rather than claimed complete:

- M05: genuinely windowless draw Engine/application execution, exactly N camera
  scene samples, configuration precedence and encoded scene output.
- M06/M07: complete GPU-only texture/image materialization, model/scene round trips,
  editor/import/unsaved-state integration and responsive application job policy.
- M08: standalone lifecycle, scene assembly, batch/output/preflight/overwrite and
  actual byte/float image files with protected inputs. Proposed standalone class
  names must be reconciled with the actual forward declarations when implemented.
- M09: final canonical naming, removal of production APIs/apps/shaders and migration
  of remaining production UV widgets/fixtures. Legacy references are still needed.
- M10: audit documentation authored incrementally; do not defer M04 authoring here.

No new bake denoiser, native Metal upscaler, ResourceManager, instancing policy or
async staging service is introduced. Geometry preview performance is not evidence
of GPU raster performance; RGB32F export capability is not proof of an exported file.

## Verification and source-style evidence

| Evidence | Owner/status | Limits |
|---|---|---|
| Steps 01–05 and corrected 06 | Accepted by project lead continuation instruction | Backend/scenario details unspecified. Do not manufacture file/pixel/timing results. |
| Missing bake metallib | Lead-reported Metal error; corrected build/copy/install source inspected | Prior statement that recursive deployment already included bake was incorrect. New source registration is not an executed fresh/install/package test. |
| API, lifecycle, native profile and caller inspection | Agent, current review | Read actual bake headers/implementations, material/preview/shader application, example submission/adoption and paired library build paths. No runtime execution. |
| Source-style inventory | Agent, 91 current changed/new owned C/C++/shader files | GPL/header, balanced control/function structure and function separation checked mechanically with focused manual include/namespace/grouping review. No reported violations; private OutputHelpers.hpp includes are permitted source-private includes. Pattern checks are not compiler validation. |
| Earlier mathematical/static checks | Recorded M04-04/M04-06 handoffs | Includes dilation seed-reference and composition algebra comparisons, native pair/binding checks and local links; not GPU numerical results. |
| Export evidence | No new file artifact provided or created | Actual new-API batch/model export remains pending; legacy writers are not evidence for new bake output. |
| Lead validation matrix | [Example guide](../../../../examples/draw/12_lightmaps/README.md), tests/bake fixtures | Vulkan/Metal/no-RT, chart/clone/restart/error cleanup, display range, CPU formats and production regressions remain explicitly attributable to lead runs. |

The review preserves the original code-style rules: full owned-file GPL headers,
braced control bodies, new-line C/C++ block braces, unindented namespaces, angled
includes except private source helpers, blank separation of .cpp functions and
purpose-grouped class APIs. No style-only engine changes were made in this step.

## Pending-plan impact and concrete edits

| Changed contract | Pending owners updated | Required result |
|---|---|---|
| Synchronous integrated completion is not offscreen execution | M05-01, M05-03, M05 summary | Own windowless completion/failure policy and camera averaging; do not reuse UV accumulator as a generic scene average. |
| Raw pixel versus color/encoding/display precision | M05-05; M06-02; M08-03/05 | Check raw transfer size, orientation and color domain; materialize files and preserve HDR separately from half-float display limits. |
| Explicit AO-slot modes and GPU-only paths | M06-02/03/05; M07-02/05; M08-03 | Persist real images plus mode/UV2/linear metadata; update editing and clone/round-trip cases. |
| Serial target/generation and immutable source policy | M06-05; M07-04/05; M08-01/02 | Retire/discard commands before source replacement; capture complete results before adoption or rebuild generations explicitly. |
| CPU UV diagnostic and session preview lifetime | M06-03/05; M07-05; M09-03/04 | Use bake renderer/draw UI handles; do not port production Vulkan preview objects as the replacement. |
| Required bake capabilities versus display fallback | M08-01/04/05 | Fail unsupported jobs deterministically without conflating successful raster display with a successful bake. |
| Native bake tree was omitted from deployment | M08-04/05; M09-05; M10-04 | Preserve distinct compile/intermediate/output paths, aggregate dependency, incremental copy and installed resources. |
| Retirement and documentation status | M09-03/04/05; M10-02/03/04; all M05–M10 summaries | Retain production until replacement parity; audit actual final sources and evidence, not provisional API claims. |

Twenty pending step files and six summaries received the concrete M04-07
requirements above. Root index and architecture decisions now reference this
review and the intentional material/preview refinements. Unaffected steps retain
their work: offscreen scene construction/settings, camera/viewpoint tooling,
application inventories/product validation, canonical app defaults and cumulative
documentation ownership are not fulfilled merely by M04. M04-08 receives this
review as its source baseline; no future API should be documented as available.

## Transition and open conditions

M04-07 is delivered for project lead acceptance. The next required step is
**M04-08 cumulative documentation**, which must inventory and reconcile maintained
pages/tutorials/build guidance with these actual contracts and the corrected
shader deployment. That step records documentation.md and appends its handoff
here. M05 is not started by this technical review and the milestone is not closed.

No unresolved architecture choice blocks documentation. Numeric/performance and
export evidence remain the declared verification limits above. The serial sample
policy, single indirect-texture slot, CPU preview and half-float display range
must remain visible rather than being silently broadened in later callers.
