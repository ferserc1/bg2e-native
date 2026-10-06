# Handoff for Plan safe editor mesh replacement

Next implementation step: [step-05_safe_reload.md](step-05_safe_reload.md). Complete this file **after** finishing step 04; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
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
