# Handoff for Integrate UV2 generation and previews

Next implementation step: [step-06_application_integration.md](step-06_application_integration.md). Complete this file **after** finishing step 05; this template records no implementation results.

- Changed files and relevant API decisions: added `lib/include/bg2e/app/Uv2SafeReload.hpp` + `lib/src/bg2e/app/Uv2SafeReload.cpp` (exported through `bg2e/app/all.hpp`). `app::Uv2SafeReload::regenerate(std::weak_ptr<scene::Node>, const geo::Uv2AtlasOptions&, CompletionCallback)` schedules the CPU modifier + `Drawable::reload()` inside `MainLoop::safeUpdateScene` (runs after `device().waitIdle()`) and returns the `SafeUpdateToken` — editors must store it and destroy/reset it on scene swap or window destruction to cancel pending work. The callback receives `app::Uv2RegenerationResult { success, message, atlasWidth, atlasHeight, chartCount, utilization }` on the main thread; it runs for success and failure (node expired, wrong scene root, no standard Drawable, mesh not loaded, modifier exception, submesh-count change). The scene root captured at schedule time is rechecked inside the callback. Bake accumulation reset and `ui::UvMapPreview::refresh()` must be triggered from the completion callback on success.
- Build command, platform and result: Not run by the agent; repository instructions prohibit compiling unless explicitly requested. Project-lead compile verification of step 05 remains outstanding.
- Runtime/fixture evidence: none; no tests in step 05.
- Remaining limitations or regressions: if `reload()` succeeds but the submesh count differs (contract violation by the modifier), the helper reports failure after the reload already happened — treated as impossible in practice since the modifier preserves submesh ranges. The helper is windowed-lifecycle only (`MainLoop::current()`); the phase-3 CLI must keep using the CPU modifier directly, as planned.
- Resources, ownership and synchronization cautions for the next agent: the helper holds only a `weak_ptr<Node>` and a stack modifier; the modifier leaves the mesh untouched on failure, so failed regeneration never disturbs the AO assignment. `Drawable::reload()` preserves `_submeshAttributes` (materials, names, transforms, visibility) and rebuilds the raster mesh, materials and BLAS. After success, the next integrated `prepareFrame` rebuilds the context-owned bake TLAS with the new BLAS automatically.
- Exact next action: Implement the scope in step-06_application_integration.md.

## Next-step instructions

In model_edit expose Generate UV2 and UV1/UV2 preview for the active model; in bg2e_composer expose them for chosen Drawable nodes from the baker window. Use safe GPU reloads transparently to users. Let both editors bake the newly generated atlas and keep all submesh AO materials on one texture. The phase-3 CLI will use the already implemented modifier before GPU load.

## Next-step acceptance gate

Both editors compile; atlas generation, preview and integrated baking work end to end. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add Generate UV2 to `ModelLightmapWindow` for the active model and to `SceneLightmapWindow` for selected Drawable nodes; pass the square bake resolution and padding to the modifier. Display `UvMapPreview` with UV1/UV2 toggle in both apps. Perform transparent safe reload through step 05, then allow a new bake with reset history. Handle modifier failure without changing the model or its AO assignment.
- In Composer, generate one atlas per selected Drawable, each covering **all** its submeshes; never pack multiple Drawables into one lightmap. The window list refreshes on scene changes and clears stale target references. In model_edit, retain existing names/group metadata and mark the document dirty on success.
- Leave the CPU-only modifier and public validator usable by a future headless caller without a MainLoop. Do not create or reference a `lightmap_generator` executable in this phase; phase 3 implements its output matrix.
- Verify visual UV1 equality before/after, UV2 island layout, two-submesh shared texture and bake result in AO in both editors.
