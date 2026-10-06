# Step 05: Plan safe editor mesh replacement

## Scope

Add or reuse a narrow application-facing helper for invoking the CPU modifier and Drawable::reload inside MainLoop::safeUpdateScene. The callback runs after Engine::device().waitIdle(); use SafeUpdateToken to cancel work after a scene/window swap. Keep the geo API CPU-only and no automatic reload in it. Preserve material/submesh state and invalidate relevant bake histories after reload.

## Acceptance and compile gate

Loaded meshes can receive UV2 updates without modifying GPU resources in flight. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- In each editor's application layer, invoke `MainLoop::current()->safeUpdateScene(..., token)` for a loaded mesh. The method waits for `Engine::device().waitIdle()` before executing the callback. Inside it, create a stack `GenerateUv2AtlasModifier`, call `apply()` on the CPU mesh, then call `Drawable::reload()` once to rebuild the raster mesh, materials and BLAS.
- Capture a weak node/Drawable reference and a `SafeUpdateToken`; recheck scene root and selection after the callback begins. Cancel pending work on scene swap/window destruction. Preserve submesh count and material attributes; mark the document dirty only after success. Invalidate active baker accumulation after reload so it cannot use a stale UV surface.
- Do not call existing `Drawable::applyModifier(new ...)` from the UI because it hides reload timing and passes an owning-looking raw pointer. Keep the modifier's public `apply()` CPU-only; no engine reload is added to `geo`.

## Safety fixture

Queue UV regeneration while the previous frame uses the mesh; verify the callback runs only after waitIdle and the new BLAS participates in the next context-owned bake TLAS build (and in the ordinary renderer's next TLAS build where applicable).

## Handoff

After this step, complete [prestep_06_application_integration.md](prestep_06_application_integration.md) for the next step (Integrate UV2 generation and previews).
