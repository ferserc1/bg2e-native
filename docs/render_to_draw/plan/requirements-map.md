# Requirement coverage and execution checkpoints

[Plan index](index.md) · [Architecture decisions](architecture-decisions.md) · [Review protocol](review-protocol.md)

Step selection, cross-chat handoff and project lead validation are governed by [execution-protocol.md](execution-protocol.md), with actual execution status in [progress.md](progress.md).

This table makes the user conclusions traceable to concrete step files. It records planned coverage, not successful implementation. The linked steps identify work, current references, verification and downstream impact. At milestone reviews update this map if a task moves or its contract changes.

| Requirement/decision | Concrete steps |
|---|---|
| Reimplementation over GPU; retain production as readable behavior reference until removal | [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md), [M09-01](milestone_09_cleanup/step_01_retirement-manifest.md), [M09-04](milestone_09_cleanup/step_04_native-render-deletion.md) |
| Bake owns all baking resources, configuration/pixels, UV preview, contexts/assembly/batch/output | [M04-01](milestone_04_lightmaps/step_01_bake-contracts.md), [M04-05](milestone_04_lightmaps/step_05_uv-preview.md), [M08-01](milestone_08_lightmap_generator/step_01_standalone-context.md), [M08-02](milestone_08_lightmap_generator/step_02_scene-assembly.md), [M08-03](milestone_08_lightmap_generator/step_03_batch-output.md) |
| UV2 modifier stays in geo; safe geometry rebuild | [M04-02](milestone_04_lightmaps/step_02_uv2-surface.md), [M06-05](milestone_06_model_edit/step_05_editor-baking.md), [M08-02](milestone_08_lightmap_generator/step_02_scene-assembly.md) |
| New draw/GPU viewpoint tools in utils; production ViewpointAnalyzer stays untouched until deletion | [M02-06](milestone_02_manipulation/step_06_viewpoint-utils.md), [M06-04](milestone_06_model_edit/step_04_viewpoint-camera.md), [M09-04](milestone_09_cleanup/step_04_native-render-deletion.md) |
| Stateless RenderSettingsPreferences; object accessors are the only configuration source | [M01-12](milestone_01_deferred_renderer/step_12_reflected-settings.md), [M01-14](milestone_01_deferred_renderer/step_14_settings-ui.md) |
| Reuse actual reflection metadata/types/getters/setters; no duplicate UI values | [M01-12](milestone_01_deferred_renderer/step_12_reflected-settings.md), [M10-02](milestone_10_documentation/step_02_api-reference.md) |
| Runtime property catalogs and safe renderer/component rebinding | [M01-05](milestone_01_deferred_renderer/step_05_runtime-renderer.md), [M01-12](milestone_01_deferred_renderer/step_12_reflected-settings.md), [M01-14](milestone_01_deferred_renderer/step_14_settings-ui.md) |
| Persist/restore and serialize/deserialize/export/import configuration; headless consumption | [M01-12](milestone_01_deferred_renderer/step_12_reflected-settings.md), [M05-03](milestone_05_offscreen/step_03_accumulation-settings.md), [M08-04](milestone_08_lightmap_generator/step_04_cli-application.md) |
| Generic BlueNoise in utils | [M03-03](milestone_03_ray_tracing/step_03_blue-noise-utils.md) |
| Advanced public layer/effect components grouped under draw::pipeline | [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md), [M01-10](milestone_01_deferred_renderer/step_10_raster-layers.md), [M03-04](milestone_03_ray_tracing/step_04_rtao.md), [M03-08](milestone_03_ray_tracing/step_08_denoise.md) |
| Public environment/cubemap/IBL/BRDF family under draw::environment | [M01-08](milestone_01_deferred_renderer/step_08_environment.md) |
| Runtime renderer/delegate relationship without renderer template locking | [M01-05](milestone_01_deferred_renderer/step_05_runtime-renderer.md), [M01-15](milestone_01_deferred_renderer/step_15_working-example.md), [M05-02](milestone_05_offscreen/step_02_draw-offscreen-context.md) |
| Native Vulkan implementation confined to GPU; no render::vulkan move into draw | [M01-02](milestone_01_deferred_renderer/step_02_gpu-rendering-primitives.md), [M01-13](milestone_01_deferred_renderer/step_13_upscaling-fsr.md), [M09-04](milestone_09_cleanup/step_04_native-render-deletion.md) |
| Simple dual engine-mode overloads where useful; remove engine dependency selectively | [M01-06](milestone_01_deferred_renderer/step_06_drawable-resources.md), [M01-07](milestone_01_deferred_renderer/step_07_db-levels.md), [M02-01](milestone_02_manipulation/step_01_dual-manipulation.md), [M05-01](milestone_05_offscreen/step_01_execution-selection.md), [M09-03](milestone_09_cleanup/step_03_shared-api-cleanup.md) |
| Namespaces may depend on GPU/draw; protect only genuinely graphics-free operations | [M01-07](milestone_01_deferred_renderer/step_07_db-levels.md), [M06-02](milestone_06_model_edit/step_02_complete-db-io.md), [M10-02](milestone_10_documentation/step_02_api-reference.md) |
| Two DB levels: pure representation/file IO and complete engine-aware resource IO | [M01-07](milestone_01_deferred_renderer/step_07_db-levels.md), [M06-02](milestone_06_model_edit/step_02_complete-db-io.md), [M07-02](milestone_07_bg2e_composer/step_02_scene-io-cloning.md) |
| Saving GPU-only generated textures requires host readback, not incomplete schema export | [M05-05](milestone_05_offscreen/step_05_readback-export.md), [M06-02](milestone_06_model_edit/step_02_complete-db-io.md), [M07-02](milestone_07_bg2e_composer/step_02_scene-io-cloning.md) |
| Keep TextureCache purpose and add draw overloads; defer ResourceManager redesign | [M01-06](milestone_01_deferred_renderer/step_06_drawable-resources.md), [M09-03](milestone_09_cleanup/step_03_shared-api-cleanup.md) |
| Manipulation stays high-level with accepted scene/draw/GPU dependencies | [M02-01](milestone_02_manipulation/step_01_dual-manipulation.md), [M02-03](milestone_02_manipulation/step_03_gizmos-overlays.md), [M06-03](milestone_06_model_edit/step_03_editor-tools.md), [M07-03](milestone_07_bg2e_composer/step_03_scene-editing.md) |
| No new private implementation classes without necessity; public compatible configuration injection | [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md), [M01-10](milestone_01_deferred_renderer/step_10_raster-layers.md), [M03-04](milestone_03_ray_tracing/step_04_rtao.md), [M03-08](milestone_03_ray_tracing/step_08_denoise.md), [M04-01](milestone_04_lightmaps/step_01_bake-contracts.md), [M05-01](milestone_05_offscreen/step_01_execution-selection.md) |
| User subclasses/custom shaders and external advanced renderers are supported | [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md), [M01-10](milestone_01_deferred_renderer/step_10_raster-layers.md), [M03-04](milestone_03_ray_tracing/step_04_rtao.md), [M10-03](milestone_10_documentation/step_03_tutorials.md) |
| Draw Drawable retains CPU/GPU connector and current coupling; dual DrawableComponent | [M01-06](milestone_01_deferred_renderer/step_06_drawable-resources.md), [M09-03](milestone_09_cleanup/step_03_shared-api-cleanup.md) |
| Deep CPU and GPU mesh/material/texture clone; no default instancing | [M01-06](milestone_01_deferred_renderer/step_06_drawable-resources.md), [M02-04](milestone_02_manipulation/step_04_material-modifier.md), [M06-03](milestone_06_model_edit/step_03_editor-tools.md), [M07-02](milestone_07_bg2e_composer/step_02_scene-io-cloning.md) |
| All new data bindings and rendering/RT visitors move into draw | [M01-09](milestone_01_deferred_renderer/step_09_bindings-visitors.md), [M09-03](milestone_09_cleanup/step_03_shared-api-cleanup.md) |
| Update/Input/Transform visitors stay scene; ResizeViewportVisitor resolved by simple dependency removal | [M01-09](milestone_01_deferred_renderer/step_09_bindings-visitors.md), [M09-03](milestone_09_cleanup/step_03_shared-api-cleanup.md) |
| No MSAA and no forward renderer in the new framework | [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md), [M01-10](milestone_01_deferred_renderer/step_10_raster-layers.md), [M09-06](milestone_09_cleanup/step_06_final-product-validation.md) |
| Rare mesh/texture edits use coordinated waitIdle; no asset resource rings | [M01-06](milestone_01_deferred_renderer/step_06_drawable-resources.md), [M02-04](milestone_02_manipulation/step_04_material-modifier.md), [M04-02](milestone_04_lightmaps/step_02_uv2-surface.md) |
| GPU capability/format/limit queries feed UI availability; SDK availability remains distinct | [M01-02](milestone_01_deferred_renderer/step_02_gpu-rendering-primitives.md), [M01-11](milestone_01_deferred_renderer/step_11_query-soft-shadows.md), [M01-12](milestone_01_deferred_renderer/step_12_reflected-settings.md), [M01-13](milestone_01_deferred_renderer/step_13_upscaling-fsr.md) |
| FSR native handlers/casts allowed at GPU backend interop seam | [M01-13](milestone_01_deferred_renderer/step_13_upscaling-fsr.md), [M09-04](milestone_09_cleanup/step_04_native-render-deletion.md) |
| Readback convenience versus primitive commands selected by cross-backend complexity | [M02-02](milestone_02_manipulation/step_02_picking-readback.md), [M02-06](milestone_02_manipulation/step_06_viewpoint-utils.md), [M05-05](milestone_05_offscreen/step_05_readback-export.md) |
| Explicit Vulkan/Metal shader binding data; no new shader abstraction rules | [M01-04](milestone_01_deferred_renderer/step_04_shader-libraries.md), [M01-09](milestone_01_deferred_renderer/step_09_bindings-visitors.md), [M03-01](milestone_03_ray_tracing/step_01_rt-design-checkpoint.md), [M03-02](milestone_03_ray_tracing/step_02_rt-gpu-data.md) |
| Consult current GPU RT docs and defer advanced tracing decisions until M03 | [M03-01](milestone_03_ray_tracing/step_01_rt-design-checkpoint.md), [M03-02](milestone_03_ray_tracing/step_02_rt-gpu-data.md) |
| Bake reuses public draw::pipeline algorithms/objects | [M03-04](milestone_03_ray_tracing/step_04_rtao.md), [M03-08](milestone_03_ray_tracing/step_08_denoise.md), [M04-02](milestone_04_lightmaps/step_02_uv2-surface.md), [M04-03](milestone_04_lightmaps/step_03_integrated-context.md) |
| Open upscaler interface, stable label/identity and reflected options for external implementations | [M01-13](milestone_01_deferred_renderer/step_13_upscaling-fsr.md), [M10-03](milestone_10_documentation/step_03_tutorials.md) |
| Default remains reduced internal resolution without FSR; Metal/DLSS implementation deferred | [M01-13](milestone_01_deferred_renderer/step_13_upscaling-fsr.md), [M05-03](milestone_05_offscreen/step_03_accumulation-settings.md) |
| M01 RT only query soft shadows/non-opaque geometry; no advanced effect/engine histories | [M01-11](milestone_01_deferred_renderer/step_11_query-soft-shadows.md), [M01-15](milestone_01_deferred_renderer/step_15_working-example.md), [M03-01](milestone_03_ray_tracing/step_01_rt-design-checkpoint.md) |
| Mips/subresource generation extends existing cubemap support at GPU level | [M01-03](milestone_01_deferred_renderer/step_03_gpu-subresources.md), [M01-08](milestone_01_deferred_renderer/step_08_environment.md) |
| Shader folders per pipeline contain both GLSL/Metal, separate from old production libraries | [M01-04](milestone_01_deferred_renderer/step_04_shader-libraries.md), [M03-02](milestone_03_ray_tracing/step_02_rt-gpu-data.md), [M04-02](milestone_04_lightmaps/step_02_uv2-surface.md), [M09-05](milestone_09_cleanup/step_05_targets-tests-packages.md) |
| UI migration assessed per widget: overload or differently named new implementation | [M01-14](milestone_01_deferred_renderer/step_14_settings-ui.md), [M02-05](milestone_02_manipulation/step_05_widget-adaptation.md), [M06-03](milestone_06_model_edit/step_03_editor-tools.md), [M07-03](milestone_07_bg2e_composer/step_03_scene-editing.md) |
| OffscreenApplication uses initialization-overload execution selection like MainLoop | [M05-01](milestone_05_offscreen/step_01_execution-selection.md), [M05-02](milestone_05_offscreen/step_02_draw-offscreen-context.md) |
| All current non-forward functionality remains accounted for, including generic graphics helpers/SMAA | [M01-10](milestone_01_deferred_renderer/step_10_raster-layers.md), [M03-09](milestone_03_ray_tracing/step_09_existing-example-integration.md), [M09-01](milestone_09_cleanup/step_01_retirement-manifest.md) |
| Every milestone yields the specified example/application or final production/documentation product | [M01-15](milestone_01_deferred_renderer/step_15_working-example.md), [M02-07](milestone_02_manipulation/step_07_working-example.md), [M03-09](milestone_03_ray_tracing/step_09_existing-example-integration.md), [M04-06](milestone_04_lightmaps/step_06_working-example.md), [M05-04](milestone_05_offscreen/step_04_working-example.md), [M06-06](milestone_06_model_edit/step_06_product-validation.md), [M07-06](milestone_07_bg2e_composer/step_06_product-validation.md), [M08-05](milestone_08_lightmap_generator/step_05_product-validation.md), [M09-06](milestone_09_cleanup/step_06_final-product-validation.md), [M10-05](milestone_10_documentation/step_05_documentation-validation.md) |
| Review actual code and edit every affected pending milestone after each milestone | [M01-16](milestone_01_deferred_renderer/step_16_review.md), [M02-08](milestone_02_manipulation/step_08_review.md), [M03-10](milestone_03_ray_tracing/step_10_review.md), [M04-07](milestone_04_lightmaps/step_07_review.md), [M05-06](milestone_05_offscreen/step_06_review.md), [M06-07](milestone_06_model_edit/step_07_review.md), [M07-07](milestone_07_bg2e_composer/step_07_review.md), [M08-06](milestone_08_lightmap_generator/step_06_review.md), [M09-07](milestone_09_cleanup/step_07_review.md), [M10-06](milestone_10_documentation/step_06_final-review.md) |

## Scope boundaries that must not drift

- FSR SDK temporal processing in M01 is allowed; engine temporal accumulation and denoise begin in M03.
- M03 upgrades the existing deferred/manipulation examples; no new advanced-RT example is required.
- M04 disables live indirect effects while applying real baked maps; IBL and direct/emissive lighting remain available.
- M05 exports an accumulated scene image with explicit sample/color/time semantics, not an undocumented warm-up-only last frame.
- Specialized bake output policy belongs in bake; generic file/image IO belongs in DB.
- Production/native code remains a reference through M08, then is deleted/relocated. No perpetual old-application compatibility survives M09.
- New native implementation lives in GPU; explicit backend binding indices and permitted backend casts are not a new shader abstraction.
- TextureCache gets only the minimal dual path, not the future ResourceManager.
- The choice of draw::pipeline as the advanced-public subnamespace is a concrete proposal, refinable during M01 review without hiding its public objects.
- Compilation/runtime checks belong to the project lead. Their continuation instruction accepts the preceding delivery without requiring reconfirmation or logs; record unspecified backend coverage accurately. Planning documents alone and silence never establish acceptance.
- Different chats implement one step at a time and update the shared progress record. A failure report keeps correction work on the reported step; each technical review and subsequent final cumulative documentation update are tracked steps before entering the next milestone.

## Cumulative documentation requirement

All original M10 final documentation outcomes remain required under the [coverage and ownership map](documentation-policy.md). Authoring moves to these final milestone steps; existing M10 references in the table above identify final audits, not deferred authoring. Only implemented functionality through each milestone is documented.

- [M01-17](milestone_01_deferred_renderer/step_17_documentation.md) — cumulative documentation through M01.
- [M02-09](milestone_02_manipulation/step_09_documentation.md) — cumulative documentation through M02.
- [M03-11](milestone_03_ray_tracing/step_11_documentation.md) — cumulative documentation through M03.
- [M04-08](milestone_04_lightmaps/step_08_documentation.md) — cumulative documentation through M04.
- [M05-07](milestone_05_offscreen/step_07_documentation.md) — cumulative documentation through M05.
- [M06-08](milestone_06_model_edit/step_08_documentation.md) — cumulative documentation through M06.
- [M07-08](milestone_07_bg2e_composer/step_08_documentation.md) — cumulative documentation through M07.
- [M08-07](milestone_08_lightmap_generator/step_07_documentation.md) — cumulative documentation through M08.
- [M09-08](milestone_09_cleanup/step_08_documentation.md) — cumulative documentation through M09.

## Preservation of independently delivered engine work

[Current-source reconciliation R01–R14](code-reconciliation-2026-10-21.md) is a
cross-cutting requirement for M08–M10. Current defaults/classification, caches,
native ABI/completion/pools, temporal/region/update behavior, profiling and save
failure semantics are implemented source contracts to preserve, not new migration
features to schedule or revert. Their functional/performance evidence is kept
separate from implementation existence and migration acceptance. The concrete
pending owning steps contain the corresponding preservation and validation work.

## M08 reviewed implementation mapping

The [M08 review](milestone_08_lightmap_generator/review.md) maps delivered actual
standalone owner/type/header/CLI/settings/output contracts to pending M08-07,
M09 and M10 tasks. [Retirement impact](milestone_08_lightmap_generator/removal-impact.md)
assigns current shared/native/product/template dependencies to concrete cleanup
steps. Required source/data/optimization preservation is separate from obsolete
old-app API compatibility; typed native HDR/complete-generation publication and
strict required-target success are explicit new contracts. P01–P11 and R01–R14
carry real evidence/provenance without inventing named backend results. M08-07
owns missing maintained standalone authoring; M10 only audits final coverage.

## M09-07 final production review — 2026-10-21

M09-06 and its scene reference-count, Metal GUI declaration and VMA implementation
corrections are accepted by the project lead's continuation instruction. Draw is
the production API; no retired render compatibility is retained. See the
[final contract review](milestone_09_cleanup/review.md), original-manifest
reconciliation and complete maintained-page handoff linked there. M09-07 is
awaiting lead acceptance; M09-08 must author cumulative documentation before
M09 closure. M10 audits that delivery. R01–R14 optimization/profiling source and
manual probes remain preserved; no new native coverage or performance result is
claimed by this review. Historical planning assumptions above remain historical.

## M10-06 final audit mapping — 2026-10-22

[Final requirement ledger](milestone_10_documentation/final-requirement-coverage.tsv)
reconciles all 43 decisions above to current contract/page/source evidence.
[R01–R14 ledger](milestone_10_documentation/final-preservation-coverage.tsv) retains
independent optimization/profiling evidence limits. The [final review](milestone_10_documentation/review.md)
awaits lead acceptance; M10-05 accepted by continuation. No next implementation
milestone is scheduled. The later explicitly requested source corrections implement canonical standalone
shader preflight and serialized shared UBO writes in GPU 11/13; their validation
record below supersedes the initial final-review findings.

## Requested source corrections delivered — 2026-10-22

[Source correction evidence](milestone_10_documentation/source-corrections.md)
supersedes the previously open shared-UBO and standalone-preflight findings.
Both source corrections and maintained guidance are delivered; no open finding
remains from them. Final lead validation/acceptance is pending. Earlier findings
are historical; no build/native execution or successor milestone is implied.
