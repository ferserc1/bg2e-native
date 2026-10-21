# M08 review — Concrete retirement impact snapshot

[Technical review](review.md) · [Per-file TSV](retirement-inventory.tsv) ·
[Inventory generator](inventory_sources.py) · [M09 owner](../milestone_09_cleanup/step_01_retirement-manifest.md)

This is an M08 source snapshot, **not a deletion command or an accepted M09
manifest**. M09-01 refreshes it against its actual starting checkout and incoming
users before performing any retirement. Native Vulkan in gpu is retained.
Shared neutral data/IO/scene behavior is not classified obsolete just because its
current file has a production overload. No old source/ABI compatibility shim is
required after M09; file/data readability is a separate requirement.

## Snapshot method and coverage

HEAD at review: 78943d5820e69363c942d71136f479a0df87128e plus all inherited M08 and
earlier uncommitted changes. Candidate inventory covers lib/include, lib/src,
apps, examples, tests, cmake, scripts and shaders, plus root CMakeLists. It scans
C/C++/Objective-C/shader/build/script/JSON/template files (including Windows batch helpers), direct render/scene::vk text,
native API tokens and product/resource identities; it also includes complete
legacy and surviving graphics/product trees and build/script registrations.
Author assets, vendored third-party code, generated binaries/build trees and
historical/maintained prose are excluded. Maintained documentation is separately
owned by M08-07/M09-08/M10 and must not be deleted by a source scanner.

There are **1338 candidate files**, including **316 direct textual legacy
matches** (comments and string literals included). Whole-tree inclusion makes
this larger than an include dependency list. It is not a transitive closure,
semantic symbol analysis or proof every candidate needs editing. Paths and
dispositions are individually listed in the TSV with owner steps and a reason.

| Candidate disposition | Files | Meaning |
|---|---:|---|
| remove_after_gates | 189 | Production render tree or scene native bindings; remove after incoming consumers migrate. |
| retire_specialized_legacy | 2 | DB production LightmapOutputWriter pair; bake owns its replacement. |
| retire_product | 53 | Old model editor/Composer/lightmap app files; preserve separate apps/draw replacements. |
| retire_or_port_check | 81 | Old examples; retain specifically required demonstrations via draw/gpu products. |
| audit_shared_before_remove | 73 | Legacy shader tree, subject to include/resource dependency review. |
| migrate_shared_api | 79 | Dual scene/DB/UI/manipulation/app interfaces; keep neutral and live draw behavior. |
| migrate_build_or_template | 20 | CMake/package/create/update scripts and standalone template/native generator references. |
| inspect_port_or_keep | 8 | Registered original test files; neutral atlas/input checks may survive. |
| audit_native_boundary | 5 | Remaining native mentions outside the main graphics trees; classify rather than blindly move/delete. |
| keep_current_implementation | 608 | Public draw/bake, gpu backends and paired shaders, including optimized/private helpers. |
| keep_product_migrate_seams | 215 | Surviving apps/draw, examples/draw and examples/gpu; canonical names/transitive includes still need checking. |
| keep_verify_consumer | 5 | Retained probe/output consumers caught by the text scan. |

Refresh from repository root:

```sh
python3 docs/render_to_draw/plan/milestone_08_lightmap_generator/inventory_sources.py
```

The generator reads source and replaces only the accompanying TSV. No source is
removed, rewritten, compiled or executed. Counts are this snapshot's counts, not
expected final deletion totals. TSV SHA256 at delivery:
8320dcb153df5f6564539e08afd06abd538d62f5ebce043bd8a4d3f1970b166e.

## Incoming dependency / replacement table

| Actual paths/family | Retire or migrate | Retain/replacement and prerequisite | Owner |
|---|---|---|---|
| lib/include/bg2e/render and lib/src/bg2e/render | Remove production Engine/renderers/forward/MSAA/deferred/native wrappers, old bake and utilities after users migrate | draw Engine/RendererDeferred/public pipeline, bake contexts/output, utils BlueNoise/ViewpointAnalyzer and gpu backends remain; no hidden render alias. | M09-03/04 |
| scene/vk headers and sources | Remove old descriptor/uniform data binding layer | draw DataBinding/ShaderData and GPU resource-set primitives; preserve packing/revisions/residency. | M09-03 |
| scene Drawable.hpp/cpp, Mesh.hpp, DrawVisitor and scene RenderQueueVisitor | Remove production Drawable/native draw mesh/visitors when callers retire | draw Drawable/RenderQueueVisitor, scene Update/Input/Transform/Find visitors and shared components remain. Inspect PickSelectionVisitor/ResizeViewportVisitor rather than deleting all visitors. | M09-03 |
| scene Node/Scene/Component, factories, component subclasses and DrawableRegistry | Remove render-Engine overloads/mode adapters only | CPU schema/hooks, detached root transfer, custom subclass/exact clone contracts, independent projection/GPU clones, helper exclusion, chains/joints, draw resource loading. | M09-03 |
| app Application/MainLoop/detail RenderGraphicsExecution/OffscreenApplication/OffscreenExecution | Remove production initialization, default tags/delegates/native callbacks | DrawGraphicsExecution and typed draw offscreen path become ordinary entry points; preserve backend choice, no-camera bake finish and camera exact-frame offscreen behavior separately. | M09-02 |
| app GPUSelectionDialog and UI UserInterface/ImGuiBackend/ImGuiVulkanBackend/ImGuiMetalBackend; DrawGraphicsExecution SDL drawable-size call | Remove production UI paths; move retained native graphics operations behind GPU primitive/native bridges | UI coordination/wrappers and draw UI stay; backend bridges must not depend on ui/scene/draw, and public headers must not expose imgui. | M09-03/04 |
| DB scene/scene_bg2/mesh_bg2/mesh_obj/scene_gltf, material/texture caches/modifiers and SceneCameraUtils | Remove production realization overloads/maps/candidate types | Shared CPU parsers, OBJ/MTL source inventory, chain/material JSON, draw complete bool-noexcept resource save, native archive, generic publishFile, stopped draw realization remain. | M09-03 |
| DB LightmapOutputWriter.hpp/cpp | Remove this production specialized writer after old CLI/users retire | bake LightmapOutputWriter is the actual batch transaction; keep db image/texture_archive/file helpers. No namespace-forwarded legacy writer. | M09-03 |
| UI old material/texture/settings/UV preview variants and manipulation old selection/gizmo methods | Retire old concrete resource variants/overloads | Draw widgets/preview registrations, selection table/gizmos/overlays and bool SelectableList lifecycle remain. | M09-03 |
| lib/include/bg2e/all.hpp, app/scene/DB/UI umbrellas; lib/include/bg2e.hpp | Remove transitive legacy exports/includes | Reconcile bake/all.hpp as the delivered bake include. Root all.hpp currently does not include bake/all.hpp; decide/add its canonical export in shared cleanup, do not document it as already exported. | M09-02/03/06 |
| apps/{model_edit,bg2e_composer,lightmap_generator}, apps/CMakeLists | Retire old source/targets | apps/draw/{model_edit,bg2e_composer,lightmap_generator}; current targets draw_model_edit/draw_bg2e_composer/draw_lightmap_generator. Any final rename must update all real consumers. | M09-02/05 |
| examples legacy debug-app/01..18 and examples/CMakeLists | Retire unsupported legacy demos or port a required check | examples/draw/01..13 and examples/gpu/01..13 are surviving products; keep deferred/manipulation/lightmaps/exact camera offscreen examples. | M09-05 |
| tests/test_01_key_and_mouse, test_02_uv_surface, test_03_integrated_baker_context, test_04_uv2_atlas and tests/CMakeLists | Inspect individually, port/remove legacy dependency | Input/CPU atlas behavior is not obsolete; keep final draw/bake checks and all optimization/profiling baselines. | M09-05 |
| tests/bake/{standalone,assembly,batch,cli,product}; tests/draw/composer_* and other draw/UI probes | Reconcile final accessors/target/header identities | Preserve probe contracts and evidence split; material-prefix inspector is not geometry reload/native proof. | M09-05/06 |
| shaders/src/glsl, shared lib includes, root/lib shader CMake | Remove only proven legacy-only stages/helpers | Paired shaders/src/draw and shaders/src/bake, shared includes, current v3/v5 stage names/capacity, SDK/runtime/FSR needs and RuntimeShaders install remain. | M09-05 |
| cmake/utils, standalone_utils, package_apps; scripts/package/create_standalone/update_libraries/templates | Remove old target/default app references, regenerate standalone template against final API | Recursive app collection, copied include tree, native runtime dependencies and paired shader refresh remain. Packaging source presence is not relocated runtime proof. | M09-05/06 |
| scripts/generate_blue_noise.c | Update the old render/blue_noise_data.h usage example if needed | Retain generation utility and utils::BlueNoise data; a comment match is not a reason to delete a graphics-neutral generator. | M09-05 |

## Final-name/data-contract reconciliation

The review records actual names, not a speculative rename. bake owns
StandaloneBakerContext/StandaloneLightmapBaker/StandaloneBakeSceneAssembler/
StandaloneBakeBatch/LightmapOutputWriter; db owns generic file/image/archive IO.
DrawableComponent currently exposes drawDrawable for draw and drawable for
production; M09-03 must choose one consistent supported accessor and update
actual apps, probes, reflection/factories and maintained snippets together.
A transitional typedef/forwarding namespace is not a final compatibility policy.

Current CLI configuration is SettingsCodec schema bg2e.draw.settings/version 1,
renderer bg2e.bake.standalone.v1, target bake/type bg2e.bake.lightmapSettings.v1.
This identity is independent of executable spelling. Preserve data semantics and
explicit mismatch diagnostics unless an intentional data migration is recorded;
renaming a binary alone is not a reason to silently relabel profiles. Final
executable/preference/import-server identities are verified in M09-02/05, with
all registered sources/resources/templates updated together.

Complete-generation lightmaps.json, relative paths, native HDR archives,
bakedAO/bakedGI/linear UV2 and source-preserving model snapshot remain the new
standalone product contract through retirement. Legacy flat-file/skip behavior
is historical reference, not a hidden final fallback. Native and performance
coverage follows P01–P11 and R01–R14, with unspecified details left unspecified.
