# M01 code-versus-remaining-plan review

Date: 2026-10-15. [Progress](../progress.md) · [Review procedure](../review-protocol.md) · [Architecture decisions](../architecture-decisions.md)

## Baseline and acceptance

Starting and finishing source revision: `b955833f314d09a191de13b83c90cacc10666ae7`.
M01 is present as uncommitted working-tree changes, not an implementation commit.
At review entry, status enumerated 197 modified tracked paths and 173 untracked
files (including the existing plan/probes). Preserve that work; no reset, staging,
commit or CMake/source mutation was performed by this review. The host reports
Darwin arm64. Installed SDK versions and actual backend/device coverage were not
provided and were not inferred from vendored code or host architecture.

M01-01 through M01-15 are accepted by project lead continuation instruction;
compilation/checks are assumed successful under the agreed execution convention.
This request accepts M01-15. Specific runtime/backend cases are not specified.
Earlier lead reports include Metal shader-name/header fixes and the virtual
`ui::Window` destructor correction; those remain in current source. The agent
performed source/static inspection only: no configuration, compilation, shader
compilation, executable launch, runtime probe or image comparison.

Actual product: `draw_deferred_renderer`, registered from
[examples/CMakeLists.txt](../../../../examples/CMakeLists.txt), with
[main.cpp](../../../../examples/draw/10_deferred_renderer/src/main.cpp), local
[CMake target](../../../../examples/draw/10_deferred_renderer/CMakeLists.txt) and
[lead acceptance checklist](../../../../examples/draw/10_deferred_renderer/README.md).
Earlier `draw_*` examples and production `17_deferred_renderer` coexist. The
former are integration callers, the latter remains a production reference;
neither their existence nor startup assertions count as independently observed
runtime evidence in this review.

## Planned versus actual contracts

Classification: **ER** equivalent refinement; **SC** intentional scope/contract
change already recorded in M01; **MR** requirement still to implement in its
assigned future milestone; **VG** verification detail not supplied. MR below is
not a claim that a deliberately excluded M01 feature was delivered.

| Area / classification | Actual code and contract | Pending impact |
|---|---|---|
| Runtime family / ER | `draw::Renderer`, `RendererConfiguration`, non-template `DefaultRenderLoopDelegate`; unique renderer owner, shared configured passes/upscaler/environment, borrowed Engine. Queued `setRenderer`; attach/detach callbacks and monotonically changing generations invalidate delayed edits before destruction. | M02 tooling attaches independently; M05 adds actual offscreen execution; M06/M07 reuse rather than copy loops. |
| Scene modes / ER | `DrawableComponent::ExecutionMode` is Uninitialized/Production/Draw; `drawDrawable()` is typed, production accessors explicitly reject draw contents. Clear before changing mode/context. Node clones create fresh identifiers and deep component/resource clones. | M02 typed SelectionItem, M06/M07 complete IO/clone workflows, M09 delete production branches. |
| Selectable / MR M02 | `SelectableComponent::update` still calls production `drawableBase()`, which throws for draw. It also lacks the new pure schema/reference hooks. It is absent from the M01 scene. | M02-01 must implement draw-aware update and hooks; preserving engine-independent intent does not mean leaving this implementation unchanged. |
| Resources/cloning / ER | `draw::Drawable` owns MeshPNUUT, GPU MeshPNUUT, materials and optional per-submesh BLAS. Loaded clones copy actual uploaded mesh data and independent material/texture/image/sampler resources. Loaded material() returns live attributes; stored SubmeshAttributes definitions are private, so M02-04 must add a narrow explicit CPU/live patch seam. Rare realization/reload/edit/clone operations drain work. | M02 modifier must preserve unrelated live fields; M04 topology/UV rebuild invalidates captures; M06/M07 independent clone/save coverage. |
| Cache / ER | `utils::TextureCache::load(draw::Engine*, path/settings)` uses device/settings-keyed weak draw entries. Production strong entries coexist. Clone isolation does not depend on sharing the cache. Engine/device outlive realizations. | M02/M06 reuse simple overloads; no ResourceManager redesign. M09 removes production map/overloads only. |
| DB levels / SC, MR M06/M07 | Raw readSceneDocument/writeSceneDocument preserve unknown JSON; schema-only loadScene/reference IO do not upload/open unresolved assets. Node/Component loadResources plus draw BG2/OBJ loaders realize resources. OBJ currently supplies geometry, example assigns maps explicitly. Full draw resource save rejects rather than silently dropping resources. | Complete glTF/MTL/high-level save, GPU-only texture export, joint/custom-factory completeness and editable-root behavior remain M06/M07/M08. |
| Pipeline family / ER | Public `pipeline::RenderLayer`, `PassContract`, `LayerConfiguration`, `RenderTarget`, `FullscreenPass`, `RasterFrame`, GBuffer/DeferredLayer/FinalLayer/SoftShadowPass; public environment objects. RendererDeferred validates known compatible stage replacements; unknown graph stages are rejected. | M03 must extend graph/effect profiles explicitly; compatible format alone is not shader ABI compatibility. |
| Attachments / ER | Six color G-buffer attachments plus depth; copied preserved opaque depth, two internal HDR colors and output-sized reconstructed HDR are slot-owned. Transparent/solid-transparent submeshes merge far-to-near, each recorded/composited separately. `queue()` exposes snapshots; concrete targets and camera fields remain private. | M02-03/M03/M04 add the smallest public borrowed surface/camera/overlay seam they need, rather than reach into private targets or duplicate the renderer. |
| GPU rendering/subresources / ER | Capabilities, color/integer/float formats, per-attachment blend/write/load/store, depthless MRT, mip/face views, uploads, transitions, exact single-subresource copies and mip generation exist. D16/D32 exact depth copy is implemented. | M04 UV targets reuse current operations subject to queried support; do not reimplement MRT. |
| Readback / ER, MR | `gpu::Image::readPixels` on Vulkan/Metal synchronously completes one initialized color mip/layer with TransferSrc and returns tightly packed raw format bytes. Metal strips staging row padding. `readPixelsRGBA8` accepts R8G8B8A8_UNORM only. Both reject depth. | M02 color picking can reuse this; add depth/subregion only if actual utility requires it. M04/M05 decode raw float/integer bytes and perform policy conversions above GPU. |
| Arrays / MR M03 | ImageType is only Image2D/Cubemap; arrayLayers is 1 or 6. Mip/face support is not a general 2D-array implementation. | M03-03 still needs genuine array descriptions/layers/views/upload if the chosen BlueNoise layout requires them. |
| Identity/bindings / ER | RenderQueue canonical one-based 24-bit object IDs are per snapshot; nodeId/submeshIndex are retained keys, node weak ownership expires. Ray collections use the same world matrices and retained resources. Dense TLAS order is separate from objectId. | M02 pick IDs cannot be assumed identical to global Selectable IDs; M03 tables retain explicit dense mapping; M04 target generation remains separate. |
| Query shadows / SC | Compute SoftShadowPass, two RGBA16F visibility masks for eight lights, 1..64 samples, alpha-aware candidate continuation, eight distinct image/sampler pairs with explicit overflow error. Generic BLAS opaque and RayTracingOpacity overrides exist on both backends. | This small UV/alpha table does not settle arbitrary M03 hit-material/resource-array architecture; explicit design checkpoint remains required. |
| Environment / ER | Public EnvironmentGenerator, CubemapRenderer/EnvironmentPass, ProceduralSkyRenderer, SphereToCubemapRenderer, IrradianceCubemapRenderer, SpecularReflectionCubemapRenderer, BRDFIntegrationMapTextureGenerator and SkyboxRenderer/SkyboxPass. Shared validated snapshots survive failed replacement. | M03/bake reuse actual linear HDR/PI-inclusive irradiance/GGX roughness/RG LUT contracts; no second private environment pipeline. |
| Settings / ER | SettingsCatalog holds live targets/metadata/unique generations, SettingsCodec/SettingsImport validate before commit, facade RenderSettingsPreferences optionally borrows Preferences. Schema bg2e.draw.settings/version string 1; numeric payload strings preserve exact types. Unknown/unavailable policy is explicit. Setter rollback failures are reported. | M05 headless uses codec directly; M06–M08 register actual targets, not copied preference structs. Preserve explicit-document precedence. |
| Upscaling / ER, SC | Open Upscaler/ResamplingUpscaler/FsrUpscaler; shared ResolutionSettings enabled/scale .25..1. Default renderer preserves fixed/native startup until enabled; product explicitly starts .67. Reconstruct composed linear HDR before one final tone/color operation. | M05 resolves live policy at explicit boundaries, M03 owns separate histories, apps preserve open identifiers. |
| FSR / SC | gpu::vk::FidelityFXBridge contains SDK/native code, only non-Apple linked SDK/header builds initialize it. Separate SDK/backend/hardware status. Camera-only motion, finite conventional RH perspective/zero-to-one depth, four queued slots; failed/unsupported/disabled paths reconstruct bilinearly. | Object/reactive motion/history enhancements remain M03; native Metal upscaling/DLSS are unimplemented extension cases, not required hidden implementations. |
| UI / ER, MR | GraphicsSettingsWindow enumerates live descriptors, uses transient ReflectionWidget::editValue and safe queued edits/import/restore. ResourcePicker supplies native-free HDR file intent. Window now has virtual destructor. Production RenderSettingsWindow remains intact. | Legacy TextureWidgets still contains production Vulkan/native interfaces; previews/material/editor widgets require M02/M06 adaptation, then M09 removal/relocation. |
| Working product / SC, VG | Native-free Application/MainLoop, four deterministic lights instead of reference ten (ABI maximum eight), cloned OBJ/glass/floor/HDR and cutout/UV/channel fixtures. External application pass/upscaler, replacement/stale-action probes, pause/scene counters, exact file/persistence controls. | No general editor/bake/offscreen/advanced-effect parity is claimed. Detailed lead scenario results remain unspecified. |

## Native ABI and state evidence

Inspected source: [ShaderData.hpp](../../../../lib/include/bg2e/draw/ShaderData.hpp),
[DataBinding.cpp](../../../../lib/src/bg2e/draw/DataBinding.cpp),
[DeferredLayer.cpp](../../../../lib/src/bg2e/draw/pipeline/DeferredLayer.cpp),
[SoftShadowPass.cpp](../../../../lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp),
[GPU Common](../../../../lib/include/bg2e/gpu/Common.hpp),
[Rendering validation](../../../../lib/src/bg2e/gpu/Rendering.cpp),
[Vulkan image](../../../../lib/src/bg2e/gpu/vk/Image.cpp) and
[Metal image](../../../../lib/src/bg2e/gpu/metal/Image.cpp).

| Interface | Actual packing / binding |
|---|---|
| CPU/shared shader data | Frame 240, Material 192, Object 336, Light 80, LightUniforms 656, Environment 16 bytes; six material texture slots, eight lights. |
| Raster frame/object | Vulkan set0 binding0/1 frame vertex/fragment; Metal buffer2/6. Set1 binding0/1 object vertex/fragment; Metal buffer3/7. |
| Raster material/environment | Set1 textures2..7/samplers8..13 -> Metal textures/samplers0..5. Light set2 binding0 -> Metal buffer4. Environment set3 textures0..3/samplers4..7 -> Metal6..9; UBO binding8 -> buffer5. |
| Fullscreen deferred | Set1 G-buffer/background textures0..7, samplers count+i; Metal0..7. Environment Metal8..11. Query variant adds visibility textures at Metal12/13 (Vulkan8/9, sampler18/19). |
| Query compute set0 | Uniform0 -> Metal buffer1; instance/UV storage1/2 -> buffers2/3; TLAS3 -> native4; normal/depth images4/5 -> textures0/1; sampler6 -> sampler8; output7/8 -> textures2/3. Cutout images16..23 -> textures4..11; samplers24..31 -> samplers0..7. |
| Reconstructed final | draw.raster.final.reconstructed.v1: first input output-domain RGBA16F HDR, seven internal G-buffer debug inputs. `finalContract(output, depth, true)` and FinalLayer's final constructor bool select it before injection validation. Standalone final.v1 remains internal-color. Debug views show the last processed layer. |
| Subresources | Copies require equal format/extent, one mip/layer and TransferSrc/TransferDst; depth only D16/D32. No implicit float/integer conversion, stencil/depth readback or subregion promise. |
| Upscale shader pair | draw/upscale/resample and motion; image0/sampler1 in set0 -> native texture0/sample0; fragment push buffer0, 16/128 bytes. Vulkan negative-height viewport and Metal framebuffer both invert NDC Y. |

Paired sources live under shaders/src/draw with shared includes in draw/lib;
ShaderLib stage helpers load distinct draw pipeline paths. Existing CMake helpers
and standalone templates preserve nested paths; new target uses that deployment.
The review did not run those build/install/package paths. GPU sources contain no
new includes of draw/scene/render/manipulation/utils/UI/DB; UI backend/native
interop still existing outside GPU is a retirement audit item, not evidence that
all final M09 relocation has already happened.

## Verification ledger and remaining scope

| Evidence | Status |
|---|---|
| Project lead M01-01..15 continuation | Accepted, including latest M01-15 product. Assumes successful lead compilation/checks under protocol; specific targets/devices/scenarios not supplied. |
| Earlier explicit corrections | Metal isFramebufferOnly spelling, irradianceImage shader name/header fix and virtual Window destructor recorded in progress; current source preserves them. |
| Agent M01-16 | Inspected actual declarations/implementations/shader layouts/native image handlers/product calls and all pending milestone summaries/step work items. Checked local review links, progress/next-step consistency and whitespace. |
| Vulkan/Metal rendering, query holes, FSR transitions, images, deployment | Concrete lead checklists/probes exist; no new per-backend results/artifacts supplied. Do not mark this matrix independently passed or require the lead to repeat it to advance. |
| Old production regression targets | Preserved as source/targets and separate checks; detailed regression coverage not specified. |
| Manual fixtures | tests/gpu and tests/draw contain intermediate probes, generally not CMake targets. Runtime assertions/counters in the new example are implemented checks, not agent-observed success. |

Completed M01 scope is the raster deferred family, public environment/resource
services, primitive rendering/subresources, query shadows, live settings/codec/UI,
open scaling/FSR bridge and accepted example. Deliberate future requirements are
selection/gizmos/MaterialModifier/viewpoint utilities (M02), advanced RT/engine
histories/denoise/SMAA (M03), bake/UV previews (M04), windowless execution/exact
scene averaging (M05), complete app/IO workflows (M06–M08), retirement (M09) and
final documentation (M10). FSR SDK state is not general accumulation; raw bytes
are not encoded export; reference IO is not complete resource save. No newly
established M01 failure was recorded at initial review delivery; the subsequent
lead-reported example cutout overflow and correction are recorded below.

## Pending-plan impact and edited files

All nine pending milestone summaries receive the reviewed M01 baseline; actions
below are also changed in their actual owning steps, not left only in this table.

| Milestone | Concrete plan updates / owning steps |
|---|---|
| M02 | 01 explicitly fixes Selectable draw access/schema hooks; 02 reuses synchronous raw color readback and separates canonical versus selectable IDs; 03 requires a public unjittered output-overlay/surface seam rather than private targets. 04 adds the explicit independent CPU/live patch seam because loaded material() aliases live state; 05/06/07 retain preview/viewpoint/product work. |
| M03 | 01 retains project-lead-owned advanced RT architecture checkpoint and eight-pair query-table limitation; 02 starts from existing opacity primitives; 03 explicitly owns missing arrays; 07 separates SDK motion/state from new effect histories and requires a public surface profile. |
| M04 | 02 reuses depthless MRT/formats/raw color transfer, adding only needed primitives; 03 requires isolated generation-checked public surface/effect inputs and actual shared environment snapshots. UV2 is texCoord1/selector1, not a third channel. Baked-map semantics remain real work; current baked RGB is neutral white. |
| M05 | 02 starts from window-only Engine/WindowSurface API and must add generic/headless context; 03 separates CLI precedence/exact linear averaging from FSR; 05 reuses raw color readback but owns conversion/orientation/encoding. |
| M06 | 02 starts from real raw/reference/loadResources/BG2/OBJ subset and completes glTF/MTL/save/GPU export; 03 inventories native legacy texture widgets separately from the completed graphics settings UI. |
| M07 | 02 preserves fresh node IDs/deep clones and distinct reference/full IO; 04 couples worker publication to actual renderer/catalog/scene generations and reload invalidation. |
| M08 | 01 must use M05 windowless context without camera renderer; 03 retains complete batch/preflight/writer semantics and completed M04/M06/M07 services. |
| M09 | 01 names actual coexistence artifacts and source/header/native seams; 05 preserves paired draw shader deployment, actual targets/templates and manual-probe disposition while retiring legacy ones. |
| M10 | 01 inventories maintained doc/ separately from historical docs/render_to_draw plan; 02 documents actual current APIs/schema plus later revisions; 03 uses the real product/CLI and final successors. |

Root index/architecture decisions link this review and record reusable primitives,
remaining gaps and phase boundaries. No milestone order is changed. All edited
paths are relative to docs/render_to_draw/plan as named above; each pending
summary contains the shared review link. Earlier completed step histories remain
unchanged apart from acceptance bookkeeping and this review handoff.

## Changed pending files

- milestone_02_manipulation: [summary.md](../milestone_02_manipulation/summary.md), [step_01_dual-manipulation.md](../milestone_02_manipulation/step_01_dual-manipulation.md), [step_02_picking-readback.md](../milestone_02_manipulation/step_02_picking-readback.md), [step_03_gizmos-overlays.md](../milestone_02_manipulation/step_03_gizmos-overlays.md), [step_04_material-modifier.md](../milestone_02_manipulation/step_04_material-modifier.md).
- milestone_03_ray_tracing: [summary.md](../milestone_03_ray_tracing/summary.md), [step_01_rt-design-checkpoint.md](../milestone_03_ray_tracing/step_01_rt-design-checkpoint.md), [step_02_rt-gpu-data.md](../milestone_03_ray_tracing/step_02_rt-gpu-data.md), [step_03_blue-noise-utils.md](../milestone_03_ray_tracing/step_03_blue-noise-utils.md), [step_07_temporal.md](../milestone_03_ray_tracing/step_07_temporal.md).
- milestone_04_lightmaps: [summary.md](../milestone_04_lightmaps/summary.md), [step_02_uv2-surface.md](../milestone_04_lightmaps/step_02_uv2-surface.md), [step_03_integrated-context.md](../milestone_04_lightmaps/step_03_integrated-context.md).
- milestone_05_offscreen: [summary.md](../milestone_05_offscreen/summary.md), [step_02_draw-offscreen-context.md](../milestone_05_offscreen/step_02_draw-offscreen-context.md), [step_03_accumulation-settings.md](../milestone_05_offscreen/step_03_accumulation-settings.md), [step_05_readback-export.md](../milestone_05_offscreen/step_05_readback-export.md).
- milestone_06_model_edit: [summary.md](../milestone_06_model_edit/summary.md), [step_02_complete-db-io.md](../milestone_06_model_edit/step_02_complete-db-io.md), [step_03_editor-tools.md](../milestone_06_model_edit/step_03_editor-tools.md).
- milestone_07_bg2e_composer: [summary.md](../milestone_07_bg2e_composer/summary.md), [step_02_scene-io-cloning.md](../milestone_07_bg2e_composer/step_02_scene-io-cloning.md), [step_04_async-import-service.md](../milestone_07_bg2e_composer/step_04_async-import-service.md).
- milestone_08_lightmap_generator: [summary.md](../milestone_08_lightmap_generator/summary.md), [step_01_standalone-context.md](../milestone_08_lightmap_generator/step_01_standalone-context.md), [step_03_batch-output.md](../milestone_08_lightmap_generator/step_03_batch-output.md).
- milestone_09_cleanup: [summary.md](../milestone_09_cleanup/summary.md), [step_01_retirement-manifest.md](../milestone_09_cleanup/step_01_retirement-manifest.md), [step_05_targets-tests-packages.md](../milestone_09_cleanup/step_05_targets-tests-packages.md).
- milestone_10_documentation: [summary.md](../milestone_10_documentation/summary.md), [step_01_documentation-inventory.md](../milestone_10_documentation/step_01_documentation-inventory.md), [step_02_api-reference.md](../milestone_10_documentation/step_02_api-reference.md), [step_03_tutorials.md](../milestone_10_documentation/step_03_tutorials.md).

Root index/architecture decisions, M01 summary/step16 and progress record are updated with the same review/acceptance handoff.

## Entry into M02

After lead acceptance/continuation of M01-16, implement
[M02-01](../milestone_02_manipulation/step_01_dual-manipulation.md), including the
explicit Selectable draw/schema corrections. Do not start manipulation in this
review turn. Do not reopen accepted M01 merely because backend logs were not
provided. Do not treat query-table layout as approval of the M03 advanced RT
design: its checkpoint still belongs to the project lead after concrete options
are prepared. Native Metal upscaling/DLSS and ResourceManager redesign remain
outside the approved implementation scope.

## Subsequent M01-15 runtime failure and correction

After this review was delivered, the lead reported the query cutout-capacity
exception closing the example. This reopens M01-15 despite its prior continuation
acceptance and holds M01-16/M02 progression. The generic cube's six face submeshes
were incorrectly treated as a single-material fixture; unconfigured face slots
retained positive cutoff and distinct fallback textures. The example correction
uses one full-index submesh for each canopy/channel cube and explicit opaque
cutoff for floor/channel materials. All geometry remains and the canopy keeps
real alpha queries, now one pair at startup. The eight-pair guard is unchanged.
Static source/range/material/count reasoning and whitespace checks were performed;
no build/runtime verification is claimed. Lead validates the correction before
the pending M01-16 review resumes; the prior acceptance history remains recorded.


## Lead-directed documentation scheduling revision

The project lead validated the HDR precision correction and requested cumulative documentation closure at every milestone. The prior next-milestone handoff is superseded: M01-16 review hands off to [M01-17 documentation](step_17_documentation.md), which must be delivered and accepted before M02-01. This is a plan update, not a claim that maintained documentation has already been updated. The separate transparency issue remains explicitly deferred by the lead and is not reported as fixed. See [policy](../documentation-policy.md) and progress for current state.

## M01-17 documentation closure (2026-10-15)

The project lead explicitly validated M01-16. Maintained documentation now
covers the delivered scene renderer/resources/environment/settings/GPU/DB/UI
contracts and numbered working example; see [coverage and checks](documentation.md).
The lead waived separate documentation validation and authorized immediate M02-01.
The accepted HDR correction and deferred transparency issue remain visible; no
additional runtime/backend evidence is inferred. Earlier handoffs are historical.
