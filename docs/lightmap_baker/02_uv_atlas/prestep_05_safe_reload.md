# Handoff for Plan safe editor mesh replacement

Next implementation step: [step-05_safe_reload.md](step-05_safe_reload.md). Complete this file **after** finishing step 04; this template records no implementation results.

- Changed files and relevant API decisions: added `lib/include/bg2e/ui/UvMapPreview.hpp` + `lib/src/bg2e/ui/UvMapPreview.cpp` (embeddable widget, exported through `bg2e/ui/all.hpp`). `init(engine, resolution=256)`, `setMesh(std::shared_ptr<geo::Mesh>)`, `setUvSet(0|1, clamped)`, `setResolution`, `setDisplaySize`, `refresh()`, `draw()`, `cleanup()`. It owns a `render::UvMapPreviewRenderer` and shows validity via `geo::UvAtlasValidator`; the sampled image goes through `TextureWidgets::setDeferredTexture`/`drawImage` using a non-owning `shared_ptr<render::Texture>` wrapper (the renderer outlives the widget texture reference; `cleanup()` clears the widget before destroying the renderer). The widget retains only the CPU mesh shared_ptr, never a Drawable.
- Build command, platform and result: Not run by the agent; repository instructions prohibit compiling unless explicitly requested. Project-lead compile verification of step 04 remains outstanding.
- Runtime/fixture evidence: none; no tests in step 04.
- Remaining limitations or regressions: `draw()` triggers a synchronous blocking `UvMapPreviewRenderer::render()` only when dirty (mesh/uvSet/resolution change or explicit `refresh()`), not per frame. Validation also runs only on refresh.
- Resources, ownership and synchronization cautions for the next agent: after an in-place CPU UV2 regeneration through `GenerateUv2AtlasModifier::apply()`, the preview's retained `shared_ptr<geo::Mesh>` still points at the same (modified) mesh, so call `UvMapPreview::refresh()` (not `setMesh`) to re-render; use `setMesh` only for model/scene swaps. `setResolution()` and `refresh()` internally call `TextureWidgets::clearTexture()`, which waits on `device().waitIdle()` before releasing the ImGui descriptor, so they are safe during safe-reload flows. Applications must call `UvMapPreview::cleanup()` before the engine is destroyed. After `Drawable::reload()`, call `refresh()` so the preview never displays a stale UV layout.
- Exact next action: Implement the scope in step-05_safe_reload.md.

## Next-step instructions

Add or reuse a narrow application-facing helper for invoking the CPU modifier and Drawable::reload inside MainLoop::safeUpdateScene. The callback runs after Engine::device().waitIdle(); use SafeUpdateToken to cancel work after a scene/window swap. Keep the geo API CPU-only and no automatic reload in it. Preserve material/submesh state and invalidate relevant bake histories after reload.

## Next-step acceptance gate

Loaded meshes can receive UV2 updates without modifying GPU resources in flight. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- In each editor's application layer, invoke `MainLoop::current()->safeUpdateScene(..., token)` for a loaded mesh. The method waits for `Engine::device().waitIdle()` before executing the callback. Inside it, create a stack `GenerateUv2AtlasModifier`, call `apply()` on the CPU mesh, then call `Drawable::reload()` once to rebuild the raster mesh, materials and BLAS.
- Capture a weak node/Drawable reference and a `SafeUpdateToken`; recheck scene root and selection after the callback begins. Cancel pending work on scene swap/window destruction. Preserve submesh count and material attributes; mark the document dirty only after success. Invalidate active baker accumulation after reload so it cannot use a stale UV surface.
- Do not call existing `Drawable::applyModifier(new ...)` from the UI because it hides reload timing and passes an owning-looking raw pointer. Keep the modifier's public `apply()` CPU-only; no engine reload is added to `geo`.
- After the safe reload, the next integrated `prepareFrame` must rebuild its context-owned bake TLAS from the root using the refreshed BLAS; do not rely on the ordinary renderer's frame TLAS.
